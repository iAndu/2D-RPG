#include "MainActor.h"
#include "Utils.h"
#include "Map.h"
#include "Hero.h"
#include "res.h"
#include "snd.h"
#include "Text.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>

using namespace oxygine;

spMainActor MainActor::mainActor = 0;
spText hero_level;
spActor layer;

spMainActor MainActor::getMainActor()
{
	if (mainActor == 0)
		mainActor = new MainActor();
	return mainActor;
}

MainActor::MainActor() : _world(0)
{
	snd::resources.loadXML("sounds.xml");
	snd::musicPlayer.play(snd::resources.get("music"));


	setSize(getStage()->getSize());

	layer = new Actor;
	layer->setSize(getSize());

	map = new Map("map.xml", "Sprites.png");
	addChild(map);
	addChild(layer);
	_world = new b2World(b2Vec2(0, 10));

	// The following 4 lines of code are used for Box2dDebug -- comment them to make the debug button to disappear 
	spButton btn = new Button();
	btn->addEventListener(TouchEvent::CLICK, CLOSURE(this, &MainActor::ShowHideDebug));
	btn->setPosition(Vector2(400, 10));
	btn->attachTo(this);
	btn->setSize(Vector2(20, 20));

	//healthBar bar
	health = new ProgressBar();
	health->setResAnim(res::resources.getResAnim("health"));
	health->setAnchor(Vector2(0.5f, 0.5f));
	health->setPosition(Vector2(900, 10));
	health->setSize(Vector2(300, 8));
	health->setDirection(ProgressBar::dir_0);
	addChild(health);

	//armor bar
	armor = new ProgressBar();
	armor->setResAnim(res::resources.getResAnim("armor"));
	armor->setAnchor(Vector2(0.5f, 0.5f));
	armor->setPosition(Vector2(900, 18));
	armor->setSize(Vector2(300, 8));
	armor->setDirection(ProgressBar::dir_0);
	addChild(armor);

	//xp bar
	xp = new ProgressBar();
	xp->setResAnim(res::resources.getResAnim("xp"));
	xp->setAnchor(Vector2(0.5f, 0.5f));
	xp->setPosition(Vector2(900, 26));
	xp->setSize(Vector2(300, 8));
	xp->setDirection(ProgressBar::dir_0);
	addChild(xp);
	

	hero = Hero::getHero(_world, getSize() / 2);
	//_mobs.push_back(hero);
	addChild(hero);
	std::string str = "LVL:  " + std::to_string(hero->GetLevel());
	hero_level = new Text(str, Color(0xFFFF00FF), Vector2(660, -10), false, 0, 0);
	addChild(hero_level);
	((b2Body*)(hero->getUserData()))->SetGravityScale(0);
	((b2Body*)(hero->getUserData()))->SetFixedRotation(true);
	this->addEventListener(TouchEvent::CLICK, CLOSURE(this, &MainActor::MoveHero));
	hero->addEventListener(TouchEvent::CLICK, CLOSURE(this, &MainActor::ClickOnHero));
	//_world->SetContactListener(&contactListener);
	hero->addEventListener(TouchEvent::OVER, CLOSURE(this, &MainActor::OverHero));
	hero->addEventListener(TouchEvent::OUT, CLOSURE(this, &MainActor::OutOfHero));
	RandomSpawn();
}

void MainActor::OverCharacter(Event * ev)
{
	TouchEvent* tev = safeCast<TouchEvent*>(ev);
	spCharacter mob = (Character*)tev->target.get();
	std::string str = "LVL: " + std::to_string(mob->GetLevel());
	spText tmob = new Text(str, Color(0xFFFF00FF), mob->getPosition() + Vector2(-25, -80), false, 0, 0);
	layer->addChild(tmob);
}

void MainActor::OutOfCharacter(Event * ev)
{
	layer->removeChildren();
}

void MainActor::OverHero(Event * ev)
{
	std::string str = "DMG: " + std::to_string(hero->GetDamage());
	spText thero = new Text(str, Color(0xFFFF00FF), hero->getPosition() + Vector2(-80, -60), false, 0, 0);
	layer->addChild(thero);
	str = "HP: " + std::to_string(hero->GetHealth());
	thero = new Text(str, Color(0xFFFF00FF), hero->getPosition() + Vector2(10, -60), false, 0, 0);
	layer->addChild(thero);
	str = "XP: " + std::to_string(hero->GetXp());
	thero = new Text(str, Color(0xFFFF00FF), hero->getPosition() + Vector2(10, 0), false, 0, 0);
	layer->addChild(thero);
	str = "ARM: " + std::to_string(hero->GetArmor());
	thero = new Text(str, Color(0xFFFF00FF), hero->getPosition() + Vector2(-80, 0), false, 0, 0);
	layer->addChild(thero);
}

void MainActor::OutOfHero(Event * ev)
{
	layer->removeChildren();
}

void MainActor::ClickOnHero(Event * ev)
{
	ev->stopImmediatePropagation();
}

void MainActor::doUpdate(const UpdateState& us)
{
	_world->Step(us.dt / 1000.0f, 6, 2);
	RandomSpawn();

	std::string str = "LVL:  " + std::to_string(hero->GetLevel());
	hero_level->SetText(str);

	SoundSystem::get()->update();
	snd::sfxPlayer.update();
	snd::musicPlayer.update();

	health->addTween(ProgressBar::TweenProgress(hero->GetHealth() / (float)hero->GetMaxHealth()), 20);
	armor->addTween(ProgressBar::TweenProgress(hero->GetArmor() / (float)hero->GetMaxArmor()), 20);
	xp->addTween(ProgressBar::TweenProgress(hero->GetXp() / (float)hero->GetNextLevelXp()), 20);

	//update each body position on display
	b2Body* body = (b2Body*)(hero->getUserData());
	b2Vec2 pos = body->GetPosition();
	if (std::abs(pos.Normalize() - Utils::convert(hero->getTargetPosition()).Normalize()) <= 0.05)
		body->SetLinearVelocity(b2Vec2(0, 0));
	hero->setPosition(Utils::convert(body->GetPosition()));
	if (body->GetLinearVelocity() == b2Vec2(0, 0))
		hero->removeTweens(true);
}

void MainActor::ShowHideDebug(Event* ev)
{
	TouchEvent* te = safeCast<TouchEvent*>(ev);
	te->stopsImmediatePropagation = true;
	if (_debugDraw)
	{
		_debugDraw->detach();
		_debugDraw = 0;
		return;
	}

	_debugDraw = new Box2DDraw;
	_debugDraw->SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit | b2Draw::e_pairBit | b2Draw::e_centerOfMassBit);
	_debugDraw->attachTo(this);
	_debugDraw->setWorld(Utils::scale, _world);
	_debugDraw->setPriority(1);
}

void MainActor::MoveHero(Event* ev)
{
	TouchEvent* tev = safeCast<TouchEvent*>(ev);
	if (tev->localPosition.x > 64 && tev->localPosition.y > 64 && tev->localPosition.y < 630 && tev->localPosition.x < 1080)
	{
		hero->setTargetPosition(tev->localPosition);
		auto body = (b2Body*)(hero->getUserData());
		b2Vec2 pos = (Utils::convert(tev->localPosition) - body->GetPosition());
		const float force = (2 / sqrt(pos.x * pos.x + pos.y * pos.y));
		pos = force * pos;
		body->SetLinearVelocity(pos);

		int x = hero->getPosition().x, y = hero->getPosition().y;
		int xTarget = tev->localPosition.x, yTarget = tev->localPosition.y;
		double tg = (x - xTarget != 0) ? (double)(yTarget - y) / (x - xTarget) : Utils::inf * ((yTarget - y) < 0 ? (-1) : 1);
		if (tg < 0)
		{
			if (yTarget - y <= 0)
			{
				if (tg > -1)
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_left")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_left")), 500, 1)));
				else
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_up")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_up")), 500, 1)));
			}
			else if (x - xTarget <= 0)
			{
				if (tg > -1)
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_right")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_right")), 500, 1)));
				else
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_down")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_down")), 500, 1)));
			}
		}
		else
		{
			if (x - xTarget >= 0 && yTarget - y >= 0)
			{
				if (tg < 1)
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_left")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_left")), 500, 1)));
				else
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_down")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_down")), 500, 1)));
			}
			else
			{
				if (tg < 1)
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_right")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_right")), 500, 1)));
				else
					hero->addTween(TweenQueue::create(
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_walk_up")), 1500, 3),
						createTween(Sprite::TweenAnim(res::resources.getResAnim("hero_idle_up")), 500, 1)));
			}
		}
	}
}

void MainActor::RandomSpawn()
{
	std::string mob_types[] = { "skeleton", "dwarf", "troll" };
	srand(time(0));
	for (int i = _mobs.size(); i < 10; ++i)
	{
		int type = rand() % 3;
		Vector2 pos;
		do {
			pos.x = rand() % (int)getSize().x;
			pos.y = rand() % (int)getSize().y;
		} while (pos.x < 64 || pos.x > 1080 || pos.y < 64 || pos.y > 630 || Overlaps(pos, 0));
        int mobLevel = rand() % hero->GetLevel() + 1;
        Character *mob = new Character(GetMobHealth(mobLevel), GetMobDamage(mobLevel), GetMobXp(mobLevel), mobLevel, mob_types[type], res::resources.getResAnim(mob_types[type] + "_idle"), _world, pos, b2_staticBody, 1);
		_mobs.push_back(mob);
		mob->addTween(TweenAnim(res::resources.getResAnim(mob_types[type] + "_spawn")), 700);
		mob->addEventListener(TouchEvent::CLICK, CLOSURE(this, &MainActor::ClickCharacter));
		mob->addEventListener(TouchEvent::OVER, CLOSURE(this, &MainActor::OverCharacter));
		mob->addEventListener(TouchEvent::OUT, CLOSURE(this, &MainActor::OutOfCharacter));
		addChild(mob);
	}

	std::string plants_types[] = { "red_flower", "blue_flower" };
	for (int i = _plants.size(); i < 8; ++i)
	{
		Vector2 pos;
		do {
			pos.x = rand() % (int)getSize().x;
			pos.y = rand() % (int)getSize().y;
		} while (pos.x < 64 || pos.x > 1080 || pos.y < 64 || pos.y > 630 || Overlaps(pos, 1));
		Environment* _plant = new Environment(res::resources.getResAnim("tree"), _world, pos, 0.2);
		_plants.push_back(_plant);
		_plant->addEventListener(TouchEvent::CLICK, CLOSURE(this, &MainActor::MoveHero));
		addChild(_plant);

	}
	for (int i = _spPlants.size(); i < 10; ++i)
	{
		int type = rand() % 2;
		Vector2 pos;
		do {
			pos.x = rand() % (int)getSize().x;
			pos.y = rand() % (int)getSize().y;
		} while (pos.x < 64 || pos.x > 1080 || pos.y < 64 || pos.y > 630 || Overlaps(pos, 1));
		SpecialEnvironment* _spPlant = new SpecialEnvironment(res::resources.getResAnim(plants_types[type]), pos);
		_spPlants.push_back(_spPlant);
		_spPlant->addEventListener(TouchEvent::CLICK, CLOSURE(this, &MainActor::ClickSpecialEnvironment));
		addChild(_spPlant);
	}
}

void MainActor::ClickCharacter(Event* _event)
{
	TouchEvent* _tevent = safeCast<TouchEvent*>(_event);

	MoveHero(_event);
	std::cout << "APP_LOG: CHARACTER CLICKED\n";
	Character* mob = (Character*)_event->target.get();
	if (Utils::distance(mob->getPosition(), hero->getPosition()) < 80)
	{
		int heroArmor = hero->GetArmor();
		int heroHealth = hero->GetHealth();
		int heroDamage = hero->DealDamage();
		int mobHealth = mob->GetHealth();
		int mobDamage = mob->DealDamage();

		std::string str = std::to_string(mobDamage);
		spText tmob = new Text(str, Color(0xFF0000FF), hero->getPosition());
		addChild(tmob);

		str = std::to_string(heroDamage);
		spText thero = new Text(str, Color(0xFFFF00FF), mob->getPosition());
		addChild(thero);

		if (heroArmor >= mobDamage)
			hero->AddArmor(-mobDamage);
		else
		{
			hero->SetArmor(0);
			hero->AddHealth(heroArmor - mobDamage);
		}
		mob->SetHealth(mobHealth - heroDamage);

		if (hero->GetHealth() <= 0)
		{
			hero->removeTweens(true);
			hero->removeAllEventListeners();
			this->removeAllEventListeners();
			for (auto it : _mobs)
				it->removeAllEventListeners();
			hero->Die(mob->GetType());
			layer->removeChildren();
	        GameOver();
			return;
		}

		if (mob->GetHealth() <= 0)
		{
			mob->removeTweens(true);
			mob->removeAllEventListeners();
			mob->Die();
			hero->AddXp(mob->GetXp());
			std::string str = "XP: +" + std::to_string(mob->GetXp());
			spText txp = new Text(str, Color(0xFFA500FF), hero->getPosition(), true, 1500, true);
			addChild(txp);
			RemoveActor(mob);
			layer->removeChildren();
			mob->addTween(TweenDummy(), 10000)->detachWhenDone();
			return;
		}
	}
}

void MainActor::ClickSpecialEnvironment(Event* _event)
{
	TouchEvent* _tevent = safeCast<TouchEvent*>(_event);
	std::cout << "APP_LOG: SPECIAL ENVIRONMENT CLICKED\n";

	SpecialEnvironment* env = (SpecialEnvironment*)_event->target.get();

	std::cout << "APP_LOG: DISTANCE - " << Utils::distance(hero->getPosition(), env->getPosition()) << '\n';
	if (Utils::distance(hero->getPosition(), env->getPosition()) < 50)
	{
		std::pair<int, int> _randomDrop = env->RandomDrop();
		std::string str;
		spText tcollect;

		switch (_randomDrop.first)
		{
			//health
		case 0: hero->AddHealth(_randomDrop.second);
			str = "HP: +" + std::to_string(_randomDrop.second);
			tcollect = new Text(str, Color(0xFFA500FF), hero->getPosition());
			addChild(tcollect);
			std::cout << "APP_LOG: ADDED " << _randomDrop.second << " HEALTH\n";
			break;
			//damage
		case 1: hero->AddDamage(_randomDrop.second);
			str = "DMG: +" + std::to_string(_randomDrop.second);
			tcollect = new Text(str, Color(0xFFA500FF), hero->getPosition());
			addChild(tcollect);
			std::cout << "APP_LOG: ADDED " << _randomDrop.second << " DAMAGE\n";
			break;
			//aromr
		case 2: hero->AddArmor(_randomDrop.second);
			str = "ARM: +" + std::to_string(_randomDrop.second);
			tcollect = new Text(str, Color(0xFFA500FF), hero->getPosition());
			addChild(tcollect);
			std::cout << "APP_LOG: ADDED " << _randomDrop.second << " ARMOR\n";
			break;
			//xp
		case 3: hero->AddXp(_randomDrop.second);
			str = "XP: +" + std::to_string(_randomDrop.second);
			tcollect = new Text(str, Color(0xFFA500FF), hero->getPosition());
			addChild(tcollect);
			std::cout << "APP_LOG: ADDED " << _randomDrop.second << " XP\n";
			break;
		}

		env->removeAllEventListeners();
		RemoveActor(env);
		env->detach();
	}
}

bool MainActor::Overlaps(const Vector2 _pos, int _type)
{
	int dist_1, dist_2;
	switch (_type)
	{
		// If mob
	case 0: dist_1 = 200; dist_2 = 30;
		break;
		// If plant
	case 1: dist_1 = 30; dist_2 = 100;
		break;
	}
	for (auto it = _mobs.begin(); it != _mobs.end(); ++it)
	{
		Vector2 mob_pos = (*it)->getPosition();
		if (sqrt((mob_pos.x - _pos.x) * (mob_pos.x - _pos.x) + (mob_pos.y - _pos.y) * (mob_pos.y - _pos.y)) < dist_1)
			return true;
	}
	for (auto it = _plants.begin(); it != _plants.end(); ++it)
	{
		Vector2 plant_pos = (*it)->getPosition();
		if (sqrt((plant_pos.x - _pos.x) * (plant_pos.x - _pos.x) + (plant_pos.y - _pos.y) * (plant_pos.y - _pos.y)) < dist_2)
			return true;
	}
	return false;
}

void MainActor::RemoveActor(Actor* _act)
{
	for (auto it = _spPlants.begin(); it != _spPlants.end(); ++it)
				if ((Actor*)*it == _act) {
					_spPlants.erase(it);
					return;
				}
	b2Body* body = _world->GetBodyList();
	while (body)
	{
		spActor actor = (Actor*)body->GetUserData();
		b2Body* next = body->GetNext();
		if (actor == _act)
		{
			body->SetUserData(0);
			_world->DestroyBody(body);
			for (auto it = _mobs.begin(); it != _mobs.end(); ++it)
				if ((Actor*)*it == _act) {
					_mobs.erase(it);
					return;
				}
			for (auto it = _plants.begin(); it != _plants.end(); ++it)
				if ((Actor*)*it == _act) {
					_plants.erase(it);
					return;
				}
		}

		body = next;
	}
}

int MainActor::GetMobHealth(int mobLevel)
{
	return hero->GetMaxHealth() - 10 * (hero->GetLevel() - mobLevel);
}

int MainActor::GetMobDamage(int mobLevel)
{
	return 10 + 3 * (mobLevel - 1);
}

int MainActor::GetMobXp(int mobLevel)
{
	return mobLevel * 50;
}

void MainActor::GameOver()
{
	spSprite img = new Sprite;
	img->setResAnim(res::resources.getResAnim("gameover"));
	img->setAnchor(0.5f, 0.5f);
	img->setSize(getSize() / 2);
	img->setPosition(Vector2(getSize().x / 2, getSize().y + img->getSize().y));
	img->addTween(TweenPosition(getSize() / 2), 3000);
	addChild(img);
}
