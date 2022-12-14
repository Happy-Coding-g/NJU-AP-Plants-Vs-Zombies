/**
 *Copyright (c) 2020 LZ.All Right Reserved
 *Author : LZ
 *Date: 2020.1.28
 *Email: 2117610943@qq.com
 */

#include "GSControlLayer.h"
#include "GSDefine.h"
#include "GSData.h"
#include "GSGameResultJudgement.h"
#include "GSGameEndLayer.h"
#include "GSButtonLayer.h"
#include "GSInformationLayer.h"
#include "GSBackgroundLayer.h"
#include "GSAnimationLayer.h"
#include "GSZombiesAppearControl.h"

#include "Plants/Plants.h"
#include "Plants/Plants-Files.h"
#include "Zombies/Zombies.h"
#include "Based/LevelData.h"
#include "Based/GameType.h"
#include "Based/UserData.h"
#include "Based/PlayMusic.h"
#include "Scenes/EasterEggsScene/GameEasterEggs.h"

GameMapInformation::GameMapInformation():
   rowNumbers(5)
,  columnNumbers(9)
{
	MAP_INIT(plantsMap);
	MAP_CAN_NOT_PLANT(plantsMap);
}

GSControlLayer::GSControlLayer():
	_global(Global::getInstance())
,   _openLevelData(OpenLevelData::getInstance())
,	_gameMapInformation(nullptr)
,   _selectPlantsTag(PlantsType::None)
,   _plantPreviewImage(nullptr)
,   _animationLayer(nullptr)
,   _gameEndShieldLayer(nullptr)
,   _zombiesAppearControl(nullptr)
{
	srand(time(nullptr));
	_levelData = _openLevelData->readLevelData(_openLevelData->getLevelNumber())->getMunchZombiesFrequency();
	_gameMapInformation = new GameMapInformation();
	_zombiesAppearControl = new ZombiesAppearControl();
}

GSControlLayer::~GSControlLayer()
{
	delete _gameMapInformation;
	delete _zombiesAppearControl;
}

bool GSControlLayer::init()
{
	if(!Layer::init())return false;

	createSchedule();
	createPlantsCardListener();
	createMouseListener();

	return true;
}

void GSControlLayer::setPlantMapCanPlant(const unsigned int colum, const unsigned int row)
{
	controlLayerInformation->_gameMapInformation->plantsMap[colum][row] = NO_PLANTS;
}

void GSControlLayer::createSchedule()
{
	schedule([&](float){
			controlCardEnabled();
			calculatePlantPosition();
			createZombies();
			controlRefurbishMusicAndText();
			judgeLevelIsFinished();
		}, 0.1f, "mainUpdate");

	schedule([&](float) {
		zombiesComeTiming();
		}, 1.0f, "zombiesComing");
}

void GSControlLayer::controlCardEnabled()
{
	for (auto& card : _global->userInformation->getUserSelectCrads())
	{
		/* ???????????????????????????????? */
		if (buttonLayerInformation->plantsCards[card.cardTag].plantsNeedSunNumbers > _global->userInformation->getSunNumbers())
		{
			buttonLayerInformation->plantsCards[card.cardTag].plantsCardText->setColor(Color3B::RED);
		}
		else
		{
			buttonLayerInformation->plantsCards[card.cardTag].plantsCardText->setColor(Color3B::BLACK);
		}
		/* ?????????????????????? */
		if (buttonLayerInformation->plantsCards[card.cardTag].timeBarIsFinished)
		{
			buttonLayerInformation->plantsCards[card.cardTag].plantsCards->setEnabled(true);
			/* ???????????????????????????? */
			if (buttonLayerInformation->plantsCards[card.cardTag].plantsNeedSunNumbers <= _global->userInformation->getSunNumbers())
			{
				buttonLayerInformation->plantsCards[card.cardTag].plantsCards->setColor(Color3B::WHITE);
			}
			else
			{
				buttonLayerInformation->plantsCards[card.cardTag].plantsCards->setColor(Color3B::GRAY);
			}
		}
	}
}

void GSControlLayer::calculatePlantPosition()
{
	for (unsigned int i = 0; i < _gameMapInformation->rowNumbers; ++i)
	{
		for (unsigned int j = 0; j < _gameMapInformation->columnNumbers; ++j)
		{
			if (GRASS_INSIDE(_cur, i, j))
			{
				_plantsPosition.x = j;
				_plantsPosition.y = i;
			}
		}
	}

	/* ???????????????????????????? */
	if (GRASS_OUTSIDE(_cur))
	{
		_plantsPosition.x = 100;
		_plantsPosition.y = 100;
	}
}

void GSControlLayer::createMouseListener()
{
	/* ???????????? */
	auto Mouse = EventListenerMouse::create();

	/* ???????? */
	Mouse->onMouseMove = [&](Event* event)
	{
		/* ???????????? */
		_cur = ((EventMouse*)event)->getLocationInView();
		mouseMoveControl();
	};

	/* ???????? */
	Mouse->onMouseDown = [&](Event* event)
	{
		mouseDownControl((EventMouse*)event);
	};

	_director->getEventDispatcher()->addEventListenerWithSceneGraphPriority(Mouse, this);
}

void GSControlLayer::createPlantsCardListener()
{
	for (auto& card : _global->userInformation->getUserSelectCrads())
	{
		buttonLayerInformation->plantsCards[card.cardTag].plantsCards->addTouchEventListener([&, card](Ref* sender, ui::Widget::TouchEventType type)
			{
				switch (type)
				{
				case ui::Widget::TouchEventType::ENDED:
					if (!buttonLayerInformation->mouseSelectImage->isSelectPlants)
					{
						_selectPlantsTag = static_cast<PlantsType>(card.cardTag);
					}
					selectPlantsPreviewImage();
					break;
				}
			});
	}
}

void GSControlLayer::selectPlantsPreviewImage()
{
	switch (buttonLayerInformation->mouseSelectImage->isSelectPlants)
	{
	case true:
		PlayMusic::playMusic("tap2");
		buttonLayerInformation->plantsCards[static_cast<unsigned int>(_selectPlantsTag)].progressTimer->setPercentage(0);
		buttonLayerInformation->plantsCards[static_cast<unsigned int>(_selectPlantsTag)].plantsCards->setColor(Color3B::WHITE);

		/* ?????????????????????? */
		_global->userInformation->setSunNumbers(_global->userInformation->getSunNumbers() + 
			buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].plantsNeedSunNumbers);
		informationLayerInformation->updateSunNumbers();

		removePreviewPlant();

		/* ???????????? */
		backgroundLayerInformation->gameType->updateRequirementNumbers("????????????");
		break;
	case false:
		/* ???????????????????????????????????????? */
		if (buttonLayerInformation->plantsCards[static_cast<unsigned int>(_selectPlantsTag)].plantsNeedSunNumbers > _global->userInformation->getSunNumbers())
		{
			PlayMusic::playMusic("buzzer");
			informationLayerInformation->sunNumberTextWarning();
		}
		/* ????????????????????0 */
		else if (backgroundLayerInformation->gameType->getPlantsRequriement()->isHavePlantsRequriement && backgroundLayerInformation->gameType->getPlantsRequriement()->surPlusPlantsNumbers <= 0)
		{
			PlayMusic::playMusic("buzzer");
			backgroundLayerInformation->gameType->waringPlantsNull();
		}
		else
		{
			PlayMusic::playMusic("seedlift");
			
			/* ?????????????????????? */
			_global->userInformation->setSunNumbers(_global->userInformation->getSunNumbers() - buttonLayerInformation->plantsCards[static_cast<unsigned int>(_selectPlantsTag)].plantsNeedSunNumbers);
			informationLayerInformation->updateSunNumbers();

			/* ???????? */
			buttonLayerInformation->plantsCards[static_cast<unsigned int>(_selectPlantsTag)].plantsCards->setColor(Color3B::GRAY);
			buttonLayerInformation->plantsCards[static_cast<unsigned int>(_selectPlantsTag)].progressTimer->setPercentage(100);

			/* ???????????? */
			buttonLayerInformation->mouseSelectImage->isSelectPlants = true;
			buttonLayerInformation->mouseSelectImage->selectPlantsId = _selectPlantsTag;

			createPreviewPlants();

			/* ???????????? */
			backgroundLayerInformation->gameType->updateRequirementNumbers("????????????");
		}
		break;
	}
}

void GSControlLayer::createPreviewPlants()
{
	CURSOR_VISIBLE(false)

	Plants* previewPlants, * curPlants;/* ???????? */
	switch (_selectPlantsTag)
	{
	case PlantsType::SunFlower:          previewPlants = SunFlower::create(this);         curPlants = SunFlower::create(this);         break;
	case PlantsType::PeaShooter:         previewPlants = PeaShooter::create(this);        curPlants = PeaShooter::create(this);        break;
	case PlantsType::WallNut:            previewPlants = WallNut::create(this);           curPlants = WallNut::create(this);           break;
	case PlantsType::CherryBomb:         previewPlants = CherryBomb::create(this);        curPlants = CherryBomb::create(this);        break;
	case PlantsType::PotatoMine:         previewPlants = PotatoMine::create(this);        curPlants = PotatoMine::create(this);        break;
	case PlantsType::CabbagePult:        previewPlants = CabbagePult::create(this);       curPlants = CabbagePult::create(this);       break;
	case PlantsType::Torchwood:          previewPlants = Torchwood::create(this);         curPlants = Torchwood::create(this);         break;
	case PlantsType::Spikeweed:          previewPlants = Spikeweed::create(this);         curPlants = Spikeweed::create(this);         break;
	case PlantsType::Garlic:             previewPlants = Garlic::create(this);            curPlants = Garlic::create(this);            break;
	case PlantsType::FirePeaShooter:     previewPlants = FirePeaShooter::create(this);    curPlants = FirePeaShooter::create(this);    break;
	case PlantsType::Jalapeno:           previewPlants = Jalapeno::create(this);          curPlants = Jalapeno::create(this);          break;
	case PlantsType::AcidLemonShooter:   previewPlants = AcidLemonShooter::create(this);  curPlants = AcidLemonShooter::create(this);  break;
	case PlantsType::Citron:             previewPlants = Citron::create(this);            curPlants = Citron::create(this);            break;
	default: break;
	}
	
	_plantPreviewImage = previewPlants->createPlantImage();

	_plantCurImage = curPlants->createPlantImage();
	_plantCurImage->setOpacity(255);
	_plantCurImage->setPosition(_cur);
	_plantCurImage->setGlobalZOrder(10);
}

void GSControlLayer::zombiesComeTiming()
{
	if (!_zombiesAppearControl->getIsBegin())
	{
		_zombiesAppearControl->setTimeClear();
		_zombiesAppearControl->setIsBegin(true);
	}

	/* ???? */
	_zombiesAppearControl->setTimeAdd();
}

void GSControlLayer::createZombies()
{
	/* ???????? */
	if (_zombiesAppearControl->getLastFrequencyZombiesWasDeath())
	{
		_zombiesAppearControl->setLastFrequencyZombiesWasDeath(false);
		_zombiesAppearControl->setTimeClear(); /* ?????????????????????? */
		if (_zombiesAppearControl->getZombiesAppearFrequency() < _openLevelData->readLevelData(_openLevelData->getLevelNumber())->getZombiesFrequency())
		{
			unsigned int zombiesNumbers = _zombiesAppearControl->getZombiesNumbersForAppearFrequency(_zombiesAppearControl->getZombiesAppearFrequency());
			for (unsigned int i = 0; i < zombiesNumbers; ++i)
			{
				animationLayerInformation->createZombies();
			}
			/* ?????????????? */
			_zombiesAppearControl->setZombiesAppearFrequency();

			/* ?????????? */
			informationLayerInformation->updateProgressBar(_zombiesAppearControl->getZombiesAppearFrequency());
		}
	}
	informationLayerInformation->updateProgressBarHead();
	
	/* ?????????????? */
	if (controlRefurbishZombies())
	{
		_zombiesAppearControl->setLastFrequencyZombiesWasDeath(true);
		_zombiesAppearControl->setIsBegin(false);
	}
}

bool GSControlLayer::controlRefurbishZombies()
{
	if ((Zombies::getZombiesNumbers() <= 4 &&
		_zombiesAppearControl->getZombiesAppearFrequency() > 3)                    /* ???????????????????????????????????? */

		|| (Zombies::getZombiesNumbers() <= 0 &&                                   /* ?????????????????????????????? */
			_zombiesAppearControl->getZombiesAppearFrequency() > 1)

		|| (_zombiesAppearControl->getTime() >= 
			_openLevelData->readLevelData(
				_openLevelData->getLevelNumber())->getFirstFrequencyTime() &&
			_zombiesAppearControl->getZombiesAppearFrequency() == 0)               /* ?????????????? */

		|| (_zombiesAppearControl->getTime() >= 32 + rand() % 10 &&
			(_zombiesAppearControl->getZombiesAppearFrequency() == 1 || 
		     _zombiesAppearControl->getZombiesAppearFrequency() == 2))             /* ???????????????? */

		|| (_zombiesAppearControl->getTime() >= 45 &&
			_zombiesAppearControl->getZombiesAppearFrequency() > 2)                /* ????????45???????????? */
		)
	{
		return true;
	}
	return false;
}

void GSControlLayer::controlRefurbishMusicAndText()
{
	/* ???????????????????????????? */
	auto level = _openLevelData->readLevelData(_openLevelData->getLevelNumber());
	if (_zombiesAppearControl->getTime() >= level->getFirstFrequencyTime() && _zombiesAppearControl->getZombiesAppearFrequency() == 0)
	{
		PlayMusic::playMusic("awooga");
	}

	/* ???????????????????????????????? */
	if (_zombiesAppearControl->getZombiesAppearFrequency() == level->getZombiesFrequency() && !_zombiesAppearControl->getIsShowWords())
	{
		if (informationLayerInformation->updateProgressBarFlag())
		{
			_zombiesAppearControl->setIsShowWords(true);
		}
	}

	/* ???????????????????????? */
	for (auto data = _levelData.begin(); data != _levelData.end();)
	{
		if (_zombiesAppearControl->getZombiesAppearFrequency() == *data)
		{
			if (informationLayerInformation->updateProgressBarFlag(-1) && informationLayerInformation->updateProgressBarFlag(*data))
			{
				data = _levelData.erase(data);
			}
			else
			{
				++data;
			}
		}
		else 
		{
			++data;
		}
	}
}

void GSControlLayer::updateFlag()
{
	for (auto data : _levelData)
	{
		if (_zombiesAppearControl->getZombiesAppearFrequency() > data)
		{
			informationLayerInformation->updateProgressBarFlag(data);
		}
	}
	if (_zombiesAppearControl->getIsShowWords())
		informationLayerInformation->updateProgressBarFinalFlag();
}

bool GSControlLayer::judgeMousePositionIsInMap()
{
	return (_plantsPosition.x >= 0 && _plantsPosition.x <= 8 && _plantsPosition.y >= 0 && _plantsPosition.y <= 4) ? true : false;
}

bool GSControlLayer::judgeMousePositionIsCanPlant()
{
	return (_gameMapInformation->plantsMap[static_cast<unsigned int>(_plantsPosition.y)][static_cast<unsigned int>(_plantsPosition.x)] != CAN_NOT_PLANT /* ?????????????? ??????????????????????*/
		&& _gameMapInformation->plantsMap[static_cast<unsigned int>(_plantsPosition.y)][static_cast<unsigned int>(_plantsPosition.x)] == NO_PLANTS)     /* ???????????????????????? */
		? true : false;
}

bool GSControlLayer::judgeMousePositionHavePlant()
{
	return (_gameMapInformation->plantsMap[static_cast<unsigned int>(_plantsPosition.y)][static_cast<unsigned int>(_plantsPosition.x)] != CAN_NOT_PLANT
		&& _gameMapInformation->plantsMap[static_cast<unsigned int>(_plantsPosition.y)][static_cast<unsigned int>(_plantsPosition.x)] != NO_PLANTS)
		? true : false;
}

void GSControlLayer::removePreviewPlant()
{
	/* ???????????? */
	_plantPreviewImage->removeFromParent();
	_plantCurImage->removeFromParent();
	buttonLayerInformation->mouseSelectImage->isSelectPlants = false;
	CURSOR_VISIBLE(true)
}

void GSControlLayer::removeShovel()
{
	buttonLayerInformation->mouseSelectImage->isSelectShovel = false;
	_director->getOpenGLView()->setCursor("resources/images/System/cursor.png", Point::ANCHOR_TOP_LEFT);
}

void GSControlLayer::recoveryPlantsColor()
{
	for (unsigned int i = 0; i < _gameMapInformation->rowNumbers; ++i)
	{
		for (unsigned int j = 0; j < _gameMapInformation->columnNumbers; ++j)
		{
			if (_gameMapInformation->plantsMap[i][j] != CAN_NOT_PLANT && _gameMapInformation->plantsMap[i][j] != NO_PLANTS)
			{
				auto plant = _animationLayer->getChildByTag(SET_TAG(Vec2(j, i)));
				if (plant)
				{
					plant->setColor(Color3B::WHITE);
				}
			}
		}
	}
}

void GSControlLayer::judgeLevelIsFinished()
{
	/* ???????? */
	if (ZombiesGroup.size() <= 0 && _zombiesAppearControl->getZombiesAppearFrequency() >=
		_openLevelData->readLevelData(_openLevelData->getLevelNumber())->getZombiesFrequency())
	{
		CURSOR_VISIBLE(true)

		setGameEnd();

		auto judgeUserWin = new GSGameResultJudgement();
		auto winOrLose = judgeUserWin->judgeUserIsWin();
		if (winOrLose == GameTypes::None)
		{
			if (_global->userInformation->getCurrentPlayLevels() >= 52)
			{
				_director->getInstance()->pushScene(TransitionFade::create(0.5f, GameEasterEggs::createScene()));
			}
			_gameEndShieldLayer->successfullEntry();
		}
		else
		{
			_gameEndShieldLayer->breakThrough(winOrLose); /* ???????? */
		}
		delete judgeUserWin;
	}
}

void GSControlLayer::setGameEnd()
{
	unschedule("mainUpdate");
	unschedule("zombiesComing");
	SunFlower::stopSun();
	Plants::stopPlantsAllAction();
	animationLayerInformation->stopRandomSun();

	_gameEndShieldLayer = GSGameEndLayer::create();
	_director->getRunningScene()->addChild(_gameEndShieldLayer, 10, "gameEndShieldLayer");
}

void GSControlLayer::mouseMoveControl()
{
	/* ?????????????????? */
	if (buttonLayerInformation->mouseSelectImage->isSelectPlants)
	{
		int posX = static_cast<int>(_plantsPosition.x);
		int posY = static_cast<int>(_plantsPosition.y);
		if (posX >= 0 && posY >= 0 && posX < 9 && posY < 5)
		{
			if (_gameMapInformation->plantsMap[posY][posX] != NO_PLANTS)
			{
				_plantPreviewImage->setPosition(Vec2(-1000, -1000));
			}
			else
			{
				_plantPreviewImage->setPosition(Vec2(GRASS_POSITION_LEFT + 122 * 
					_plantsPosition.x + 60, 110 + 138 * (_plantsPosition.y + 1) - 60));
			}
		}
		else
		{
			_plantPreviewImage->setPosition(Vec2(-1000, -1000));
		}
		_plantCurImage->setPosition(_cur + Vec2(0, 30));
	}

	/* ???????????? */
	if (buttonLayerInformation->mouseSelectImage->isSelectShovel)
	{
		if (!_animationLayer)
		{
			_animationLayer = _director->getRunningScene()->getChildByName("animationLayer");
		}

		/* ?????????????????????????? */
		recoveryPlantsColor();

		if (judgeMousePositionIsInMap() && judgeMousePositionHavePlant())  /* ???????????????? && ???????? */
		{
			auto plant = _animationLayer->getChildByTag(SET_TAG(_plantsPosition));
			if (plant)
			{
				plant->setColor(Color3B(100, 100, 100));
			}
		}
	}
}

void GSControlLayer::mouseDownControl(EventMouse* eventmouse)
{
	if (eventmouse->getMouseButton() == EventMouse::MouseButton::BUTTON_RIGHT) /* ?????????????? */
	{	
		if (buttonLayerInformation->mouseSelectImage->isSelectPlants)/* ???????????? */
		{
			PlayMusic::playMusic("tap2");
			buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].progressTimer->setPercentage(0);
			buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].plantsCards->setColor(Color3B::WHITE);

			/* ?????????????????????? */
			_global->userInformation->setSunNumbers(_global->userInformation->getSunNumbers() +
				buttonLayerInformation->plantsInformation->PlantsNeedSunNumbers[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)]);
			informationLayerInformation->updateSunNumbers();

			/* ???????????? */
			backgroundLayerInformation->gameType->updateRequirementNumbers("????????????");

			removePreviewPlant();
		}

		if (buttonLayerInformation->mouseSelectImage->isSelectShovel) /* ???????????? */
		{
			removeShovel();
			recoveryPlantsColor();
		}
	}

	if (eventmouse->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) /* ?????????????? */
	{
		if (buttonLayerInformation->mouseSelectImage->isSelectPlants)
		{
			if (judgeMousePositionIsInMap() && judgeMousePositionIsCanPlant()) /* ???????????????? && ???????????? */
			{
				/* ???????????????? */
				UserData::getInstance()->caveUserData("USEPLANTSNUMBERS", ++_global->userInformation->getUsePlantsNumbers());

				/* ???????? */
				animationLayerInformation->plantPlants();

				/* ?????????????????? */
				_gameMapInformation->plantsMap[static_cast<unsigned int>(_plantsPosition.y)][static_cast<unsigned int>(_plantsPosition.x)] =
					static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId);

				/* ???????????????????????? */
				unsigned int plantsTag = static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId);
				buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].timeBarIsFinished = false;
				buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].plantsCards->setEnabled(false);
				buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].plantsCards->setColor(Color3B::GRAY);
				buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].progressTimer->runAction(
					Sequence::create(ProgressFromTo::create(buttonLayerInformation->plantsInformation->PlantsCoolTime[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)], 100, 0),
						CallFunc::create([=]() { buttonLayerInformation->plantsCards[plantsTag].timeBarIsFinished = true; }), nullptr)
				);
				removePreviewPlant();
			}
			else
			{
				if (_cur.x > CARD_BAR_RIGHT)
				{
					PlayMusic::playMusic("buzzer");
					/* ???????????? */
					buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].progressTimer->setPercentage(0);
					buttonLayerInformation->plantsCards[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)].plantsCards->setColor(Color3B::WHITE);

					/* ???????? */
					informationLayerInformation->createPromptText();

					removePreviewPlant();

					/* ?????????????????????? */
					_global->userInformation->setSunNumbers(_global->userInformation->getSunNumbers() +
						buttonLayerInformation->plantsInformation->PlantsNeedSunNumbers[static_cast<unsigned int>(buttonLayerInformation->mouseSelectImage->selectPlantsId)]);
					informationLayerInformation->updateSunNumbers();

					/* ???????????? */
					backgroundLayerInformation->gameType->updateRequirementNumbers("????????????");
				}
			}
		}
		if (buttonLayerInformation->mouseSelectImage->isSelectShovel) /* ???????????? */
		{
			PlayMusic::playMusic("plant2");
			if (judgeMousePositionIsInMap() && judgeMousePositionHavePlant())    /* ???????????????? && ???????? */
			{
				animationLayerInformation->deletePlants();/* ???????? */
			}
			removeShovel();
			recoveryPlantsColor();
		}
	}
}