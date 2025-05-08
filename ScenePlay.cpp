#include "ScenePlay.h"
#include "GameEngine.h"
#include "SceneMenu.h"
#include "Physics.h";

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

void ScenePlay::init(const std::string& levelPath)
{
	std::cout << "Game started" << std::endl;
	registerAction(sf::Keyboard::W, "UP");
	registerAction(sf::Keyboard::A, "LEFT");
	registerAction(sf::Keyboard::D, "RIGHT");
	registerAction(sf::Keyboard::S, "DOWN");
	registerAction(sf::Keyboard::Escape, "MENU");
	registerAction(sf::Keyboard::Space, "PAUSE");
	registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
	registerAction(sf::Keyboard::C, "TOGGLE_COLLISION");
	registerAction(sf::Keyboard::G, "TOGGLE_GRID");
	registerAction(sf::Keyboard::B, "SHOOT");
	registerAction(sf::Keyboard::V, "CHANGE_VIEW");

	m_gridText.setCharacterSize(16);
	m_gridText.setFont(m_game->getAssets().getFont("Algerian"));

	m_playText.setFont(m_game->getAssets().getFont("Arial"));
	m_playText.setFillColor(sf::Color::Black);
	m_playText.setStyle(sf::Text::Bold);

	for (auto& entity : m_entities.getEntities())
	{
		entity->destroy();
	}

	loadLevel(levelPath);
	srand(time(NULL));

	spawnPlayer();
}

void ScenePlay::setView()
{
	if (m_follow)
	{
		m_view.setCenter(sf::Vector2f(m_player->getComponent<CTransform>().pos.x, m_player->getComponent<CTransform>().pos.y));
		m_view.setSize(sf::Vector2f(m_game->m_widthW, m_game->m_heightW));

		//pm_view.move(sf::Vector2f(m_player->getComponent<CTransform>().velocity.x, m_player->getComponent<CTransform>().velocity.y));
	}
	else
	{
		int x = static_cast<int>(m_player->getComponent<CTransform>().pos.x);
		int y = static_cast<int>(m_player->getComponent<CTransform>().pos.y);

		while (x >= static_cast<int>(-m_game->m_worldWidth))
		{
			if (x % static_cast<int>(m_game->m_widthW) == 0)
			{
				break;
			}
			x--;
		}

		while (y >= static_cast<int>(-m_game->m_worldHeight))
		{
			if (y % static_cast<int>(m_game->m_heightW) == 0)
			{
				break;
			}
			y--;
		}

		m_view.reset(sf::FloatRect(x, y, m_game->m_widthW, m_game->m_heightW));
	}

	m_game->m_window.setView(m_view);
}

Vec2 ScenePlay::windowToWorld(const Vec2& windowPos) const
{
	auto& view = m_game->m_window.getView();

	float wx = view.getCenter().x - (m_game->m_window.getSize().x / 2);
	float wy = view.getCenter().y - (m_game->m_window.getSize().y / 2);

	return Vec2(windowPos.x + wx, windowPos.y + wy);
}

void ScenePlay::spawnPlayer()
{
	m_sound.setBuffer(m_game->getAssets().getSound("Start"));
	m_sound.play();

	m_player = m_entities.addEntity("Player");

	m_player->addComponent<CAnimation>(m_game->getAssets().getAnimation("StandSide"), false);

	m_player->addComponent<CState>("StandSide");

	m_player->addComponent<CTransform>(Vec2(gridtoMidPixel(0, 0, 11, 8, m_player)));

	m_player->addComponent<CBoundingBox>(Vec2(m_player->getComponent<CAnimation>().animation.getSize().x - 10, m_player->getComponent<CAnimation>().animation.getSize().y), sf::Color::Black);

	m_player->addComponent<CHealthBar>(5);
}

void ScenePlay::loadLevel(const std::string& level_path)
{
	std::cout << level_path << std::endl;
	std::fstream myFiles(level_path);
	if (!myFiles.is_open())
	{
		std::cerr << "Failed to open the file: " << level_path << std::endl;
		return;
	}

	std::string line;
	while (std::getline(myFiles, line))
	{
		if (line.empty())
		{
			continue;
		}

		std::istringstream lineStream(line);
		std::string assetType, nameAsset, patrolType, chaseType, color, xAsset, yAsset;
		size_t healthBarSize;
		int damage, xWindow, yWindow;
		float speed;

		if (lineStream >> assetType >> nameAsset >> patrolType >> xWindow >> yWindow >> xAsset >> yAsset >> chaseType >> healthBarSize >> color >> damage >> speed)
		{
			if (assetType == "Enemy")
			{
				spawnEnemy(nameAsset, patrolType, xWindow, yWindow, xAsset, yAsset, chaseType, healthBarSize, color, damage, speed);
			}
			else
			{
				auto entity = m_entities.addEntity(assetType);
				entity->addComponent<CAnimation>(m_game->getAssets().getAnimation(nameAsset), false);

				entity->addComponent<CTransform>(gridtoMidPixel(xWindow, yWindow, std::stof(xAsset), std::stof(yAsset), entity));

				sf::Color colorName;
				selectColor(color, colorName);
				entity->addComponent<CBoundingBox>(Vec2(entity->getComponent<CAnimation>().animation.getSize().x, entity->getComponent<CAnimation>().animation.getSize().y), colorName);

				if (entity->getComponent<CAnimation>().animation.getName() == "Black")
				{
					m_teleportPoints.push_back(entity->getComponent<CTransform>().pos);
				}
			}

		}
		else
		{
			std::cerr << "Malformed line" << std::endl;
		}
	}
}

void ScenePlay::selectColor(const std::string& color, sf::Color& colorName)
{
	if (color == "Black")
	{
		colorName = sf::Color::Black;
	}

	if (color == "Red")
	{
		colorName = sf::Color::Red;
	}

	if (color == "Blue")
	{
		colorName = sf::Color::Blue;
	}

	if (color == "White")
	{
		colorName = sf::Color::White;
	}
}

std::vector<float> ScenePlay::parseValues(const std::string& str) {
	std::vector<float> values;
	std::stringstream ss(str);
	std::string temp;

	while (std::getline(ss, temp, ',')) {
		if (!temp.empty()) {
			values.push_back(std::stof(temp));
		}
	}
	return values;
}

void ScenePlay::spawnEnemy(const std::string& name, const std::string& patrol, float xWindow, float yWindow, const std::string& xPos, const std::string& yPos, const std::string& chase, size_t health, const std::string& color, int damage, float speed)
{
	std::shared_ptr<Entity> entity = m_entities.addEntity("Enemy");
	entity->addComponent<CAnimation>(m_game->getAssets().getAnimation(name), false);

	if (patrol == "NonPatrol")
	{
		std::vector<Vec2> patrolPoints;
		float x = std::stof(xPos);
		float y = std::stof(yPos);

		patrolPoints.push_back(Vec2(x, y));
		entity->addComponent<CTransform>(gridtoMidPixel(xWindow, yWindow, patrolPoints[0].x, patrolPoints[0].y, entity));

		if (xWindow != 0)
		{
			x = x + ((xWindow * m_game->m_widthW) / m_gridSize.x);
		}

		if (yWindow != 0)
		{
			y = y + ((yWindow * m_game->m_heightW) / m_gridSize.y);
		}
		patrolPoints[0] = Vec2(x, y);
		entity->addComponent<CPatrol>(patrolPoints);
	}
	else
	{
		std::vector<float> xValues = parseValues(xPos);
		std::vector<float> yValues = parseValues(yPos);

		if (xValues.size() != yValues.size()) {
			std::cerr << "Error: Mismatched number of X and Y coordinates!" << std::endl;
		}

		std::vector<Vec2> patrolPoints;
		for (size_t i = 0; i < xValues.size(); i++) {
			patrolPoints.push_back({ xValues[i], yValues[i] });
		}
		entity->addComponent<CTransform>(gridtoMidPixel(xWindow, yWindow, patrolPoints[0].x, patrolPoints[0].y, entity));

		for (size_t i = 0; i < xValues.size(); i++) {
			if (xWindow != 0)
			{
				patrolPoints[i].x = patrolPoints[i].x + ((xWindow * m_game->m_widthW) / m_gridSize.x);
			}

			if (yWindow != 0)
			{
				patrolPoints[i].y = patrolPoints[i].y + ((yWindow * m_game->m_heightW) / m_gridSize.y);
			}
		}
		entity->addComponent<CPatrol>(patrolPoints);
	}

	if (chase == "Chase")
	{
		entity->addComponent<CChase>(true);
	}

	entity->addComponent<CHealthBar>(health);

	sf::Color colorName;
	selectColor(color, colorName);
	entity->addComponent<CBoundingBox>(Vec2(entity->getComponent<CAnimation>().animation.getSize().x, entity->getComponent<CAnimation>().animation.getSize().y), colorName);

	entity->addComponent<CDamage>(damage);

	entity->getComponent<CTransform>().speed = speed;
}

Vec2 ScenePlay::gridtoMidPixel(float windowX, float windowY, float gridX, float gridY, std::shared_ptr<Entity> entity)
{
	if (!entity->hasComponent<CAnimation>()) {
		throw std::runtime_error("Entity missing required CAnimation component.");
	}

	auto& animationSize = entity->getComponent<CAnimation>().animation.getSize();

	float centerX = gridX + (animationSize.x / (2.f * m_gridSize.x));
	float centerY = (gridY + 1) - (animationSize.y / (2.f * m_gridSize.y));

	float offsetX = 0;
	float offsetY = 0;

	if (windowX != 0)
	{
		offsetX = windowX * m_game->m_widthW;
		//return Vec2(centerX * m_gridSize.x + (windowX * m_game->m_widthW), centerY * m_gridSize.y);
	}

	if (windowY != 0)
	{
		offsetY = windowY * m_game->m_heightW;
		//return Vec2(centerX * m_gridSize.x, centerY * m_gridSize.y + (windowY * m_game->m_heightW));
	}

	//return Vec2(centerX * m_gridSize.x, centerY * m_gridSize.y);
	return Vec2((centerX * m_gridSize.x) + offsetX, (centerY * m_gridSize.y) + offsetY);
}

ScenePlay::ScenePlay(std::shared_ptr<GameEngine> game, const std::string& level_path)
	: Scene(std::move(game))
	, m_levelPath(level_path)
{
	ScenePlay::init(m_levelPath);
}

void ScenePlay::sAnimation()
{
	auto& animComponent = m_player->getComponent<CAnimation>().animation;
	const auto& stateComponent = m_player->getComponent<CState>();

	if (stateComponent.state == "RunUp")
	{
		animComponent = m_game->getAssets().getAnimation("RunUp");
	}
	if (stateComponent.state == "RunSide")
	{
		animComponent = m_game->getAssets().getAnimation("RunSide");
	}
	if (stateComponent.state == "RunDown")
	{
		animComponent = m_game->getAssets().getAnimation("RunDown");
	}
	if (stateComponent.state == "StandUp")
	{
		animComponent = m_game->getAssets().getAnimation("StandUp");
	}
	if (stateComponent.state == "StandSide")
	{
		animComponent = m_game->getAssets().getAnimation("StandSide");
	}
	if (stateComponent.state == "StandDown")
	{
		animComponent = m_game->getAssets().getAnimation("StandDown");
	}

	animComponent.m_sprite.setScale(m_player->getComponent<CTransform>().angle, 1.0f);

	for (auto& entity : m_entities.getEntities())
	{
		if (entity->tag() == "Player")
		{
			entity->getComponent<CAnimation>().animation.m_gameFrame = m_currentFrame;
		}
		entity->getComponent<CAnimation>().animation.update();
		if (entity->getComponent<CAnimation>().animation.hasEnded() && entity->getComponent<CAnimation>().destroy)
		{
			entity->destroy();
		}
	}
}

void ScenePlay::sDebug()
{
	setView();
	if (m_drawTextures)
	{
		std::string state = m_player->getComponent<CState>().state;
		for (auto& entity : m_entities.getEntities("Sword"))
		{
			if (state != "StandDown" && state != "RunDown")
			{
				entity->getComponent<CAnimation>().animation.getSprite().setPosition(entity->getComponent<CTransform>().pos.x, entity->getComponent<CTransform>().pos.y);
				m_game->m_window.draw(entity->getComponent<CAnimation>().animation.getSprite());
			}
		}

		for (auto& entity : m_entities.getEntities())
		{
			if (entity->tag() != "Sword")
			{
				if (entity->hasComponent<CHealthBar>())
				{
					auto& healthBarComp = entity->getComponent<CHealthBar>();
					healthBarComp.healthBar.setPosition(entity->getComponent<CTransform>().pos.x, entity->getComponent<CTransform>().pos.y - 38.0f);

					Vec2 refPos(healthBarComp.healthBar.getPosition().x, healthBarComp.healthBar.getPosition().y);
					float begin = refPos.x - (healthBarComp.healthBar.getSize().x / 2.0f);

					for (int i = 0; i < healthBarComp.size; i++)
					{
						begin += (healthBarComp.healthBox[i].getSize().x / 2.0f);
						healthBarComp.healthBox[i].setPosition(begin, refPos.y);
						if (healthBarComp.remaining > i)
						{
							healthBarComp.healthBox[i].setFillColor(sf::Color::Red);
						}
						else
						{
							healthBarComp.healthBox[i].setFillColor(sf::Color::Transparent);
						}
						m_game->m_window.draw(healthBarComp.healthBox[i]);
						begin += (healthBarComp.healthBox[i].getSize().x / 2.0f);
					}

				}

				entity->getComponent<CAnimation>().animation.getSprite().setPosition(entity->getComponent<CTransform>().pos.x, entity->getComponent<CTransform>().pos.y);
				m_game->m_window.draw(entity->getComponent<CAnimation>().animation.getSprite());
			}
			else
			{
				if (state == "StandDown" || state == "RunDown")
				{
					entity->getComponent<CAnimation>().animation.getSprite().setPosition(entity->getComponent<CTransform>().pos.x, entity->getComponent<CTransform>().pos.y);
					m_game->m_window.draw(entity->getComponent<CAnimation>().animation.getSprite());
				}
			}
		}
	}

	if (m_drawGrid)
	{
		sf::VertexArray gridLines(sf::Lines);

		for (int x = -m_game->m_worldWidth; x <= m_game->m_worldWidth; x += m_gridSize.x)
		{
			gridLines.append(sf::Vertex(sf::Vector2f(x, -m_game->m_worldHeight), sf::Color::White));
			gridLines.append(sf::Vertex(sf::Vector2f(x, m_game->m_worldHeight), sf::Color::White));
		}

		for (int y = -m_game->m_worldHeight; y <= m_game->m_worldHeight; y += m_gridSize.y)
		{
			gridLines.append(sf::Vertex(sf::Vector2f(-m_game->m_worldWidth, y), sf::Color::White));
			gridLines.append(sf::Vertex(sf::Vector2f(m_game->m_worldWidth, y), sf::Color::White));
		}

		m_game->m_window.draw(gridLines);
		m_gridText.setFillColor(sf::Color::White);

		for (int y = -m_game->m_worldHeight / m_gridSize.y; y <= m_game->m_worldHeight / m_gridSize.y; ++y)
		{
			for (int x = -m_game->m_worldWidth / m_gridSize.x; x <= m_game->m_worldWidth / m_gridSize.x; ++x)
			{
				std::ostringstream label;
				label << x << "," << y;
				m_gridText.setString(label.str());

				m_gridText.setPosition(x * m_gridSize.x + 5, y * m_gridSize.y + 5);
				m_game->m_window.draw(m_gridText);
			}
		}
	}


	if (m_drawCollision)
	{
		for (auto& entity : m_entities.getEntities())
		{
			if (entity->hasComponent<CBoundingBox>())
			{
				m_game->m_window.draw(entity->getComponent<CBoundingBox>().rectangle);
			}

			if (entity->hasComponent<CPatrol>())
			{
				auto& patrolComponent = entity->getComponent<CPatrol>();
				for (size_t i = 0; i < patrolComponent.patrolReference.size(); i++)
				{
					m_game->m_window.draw(patrolComponent.patrolReference[i]);
				}
			}

			if (entity->hasComponent<CChase>())
			{
				m_game->m_window.draw(entity->getComponent<CChase>().line, 2, sf::Lines);
			}
		}
	}
}

void ScenePlay::spawnSword()
{
	m_sound.setBuffer(m_game->getAssets().getSound("Bullet"));
	m_sound.play();

	std::shared_ptr<Entity> entity = m_entities.addEntity("Sword");

	entity->addComponent<CAnimation>(m_game->getAssets().getAnimation("Sword"), false);
	Vec2 refPos(m_player->getComponent<CTransform>().pos);
	Vec2 boundingBoxSize;

	if (m_player->getComponent<CState>().state == "StandSide" || m_player->getComponent<CState>().state == "RunSide")
	{
		boundingBoxSize = Vec2(entity->getComponent<CAnimation>().animation.getSprite().getTexture()->getSize().x, entity->getComponent<CAnimation>().animation.getSprite().getTexture()->getSize().y);
		if (m_player->getComponent<CTransform>().angle == 1.0f)
		{
			entity->getComponent<CAnimation>().animation.getSprite().setRotation(90.0f);
			entity->addComponent<CTransform>(Vec2(refPos.x + 51.0f, refPos.y + 10.0f));
		}
		else if (m_player->getComponent<CTransform>().angle == -1.0f)
		{
			entity->getComponent<CAnimation>().animation.getSprite().setRotation(-90.0f);
			entity->addComponent<CTransform>(Vec2(refPos.x - 51.0f, refPos.y + 10.0f));
		}
		entity->addComponent<CBoundingBox>(Vec2(boundingBoxSize.y, boundingBoxSize.x), sf::Color::Blue);
	}
	else
	{
		boundingBoxSize = Vec2(entity->getComponent<CAnimation>().animation.getSize().x, entity->getComponent<CAnimation>().animation.getSize().y);
		if (m_player->getComponent<CState>().state == "StandUp" || m_player->getComponent<CState>().state == "RunUp")
		{
			entity->addComponent<CTransform>(Vec2(refPos.x + 12.0f, refPos.y - 28.0f));
		}
		else if (m_player->getComponent<CState>().state == "StandDown" || m_player->getComponent<CState>().state == "RunDown")
		{
			entity->getComponent<CAnimation>().animation.getSprite().setRotation(180.0f);
			entity->addComponent<CTransform>(Vec2(refPos.x + 16.0f, refPos.y + 52.0f));
		}
		entity->addComponent<CBoundingBox>(boundingBoxSize, sf::Color::Blue);
	}

	entity->addComponent<CLifespan>(0.25);
	entity->addComponent<CDamage>(1);

	m_player->getComponent<CInput>().cancut = false;
}

void ScenePlay::sLifeSpan()
{
	for (auto& entity : m_entities.getEntities())
	{
		if (entity->hasComponent<CLifespan>())
		{
			entity->getComponent<CLifespan>().remaining--;
			if (entity->getComponent<CLifespan>().remaining <= 0.0f)
			{
				if (entity->tag() == "Sword")
				{
					m_player->getComponent<CInput>().cancut = true;
				}
				entity->destroy();
			}
		}
	}
}

bool IsInside(Vec2 pos, std::shared_ptr<Entity> e)
{
	auto ePos = e->getComponent<CTransform>().pos;
	auto size = e->getComponent<CAnimation>().animation.getSize();

	float dx = fabs(pos.x - ePos.x);
	float dy = fabs(pos.y - ePos.y);

	return (dx <= size.x / 2) && (dy <= size.y / 2);
}

void ScenePlay::sDoAction(const Action& action)
{
	if (action.type() == "START")
	{
		if (action.name() == "MENU")
		{
			onEnd();
		}
		if (action.name() == "TOGGLE_TEXTURE")
		{
			m_drawTextures = !m_drawTextures;
		}
		if (action.name() == "TOGGLE_COLLISION")
		{
			m_drawCollision = !m_drawCollision;
		}
		if (action.name() == "TOGGLE_GRID")
		{
			m_drawGrid = !m_drawGrid;
		}
		if (action.name() == "PAUSE")
		{
			m_paused = !m_paused;
		}
		if (action.name() == "CHANGE_VIEW")
		{
			m_follow = !m_follow;
		}

		if (action.name() == "MOUSE_MOVE")
		{
			m_mPos = action.pos();
			m_mShape.setPosition(m_mPos.x, m_mPos.y);
		}

		if (action.name() == "RIGHT")
		{
			m_player->getComponent<CInput>().right = true;
		}

		else if (action.name() == "LEFT")
		{
			m_player->getComponent<CInput>().left = true;
		}

		else if (action.name() == "UP")
		{
			m_player->getComponent<CInput>().up = true;
		}

		else if (action.name() == "DOWN")
		{
			m_player->getComponent<CInput>().down = true;
		}

		if (action.name() == "LEFT_CLICK")
		{
			if (m_player->getComponent<CInput>().cancut)
			{
				spawnSword();
			}
		}
	}

	if (action.type() == "END")
	{
		if (action.name() == "RIGHT")
		{
			m_player->getComponent<CState>().state = "StandSide";
			m_player->getComponent<CInput>().right = false;
		}

		else if (action.name() == "LEFT")
		{
			m_player->getComponent<CState>().state = "StandSide";
			m_player->getComponent<CInput>().left = false;
		}

		else if (action.name() == "UP")
		{
			m_player->getComponent<CState>().state = "StandUp";
			m_player->getComponent<CInput>().up = false;
		}

		else if (action.name() == "DOWN")
		{
			m_player->getComponent<CState>().state = "StandDown";
			m_player->getComponent<CInput>().down = false;
		}
	}
}

const ActionMap& ScenePlay::getActionMap() const
{
	return m_actionMap;
}

void ScenePlay::damage(std::shared_ptr<Entity> entity, int safe, int damage = 1)
{
	if (m_currentFrame - entity->getComponent<CHealthBar>().lastHurt >= safe)
	{
		entity->getComponent<CHealthBar>().lastHurt = m_currentFrame;
		entity->getComponent<CHealthBar>().remaining -= damage;
		if (entity->getComponent<CHealthBar>().remaining <= 0)
		{
			if (entity->tag() != "Player")
			{
				m_sound.setBuffer(m_game->getAssets().getSound("Explosion"));
				m_sound.play();
				entity->getComponent<CBoundingBox>().has = false;
				entity->getComponent<CHealthBar>().has = false;
				entity->getComponent<CChase>().has = false;
				entity->getComponent<CPatrol>().has = false;
				entity->getComponent<CAnimation>().destroy = true;
				entity->getComponent<CAnimation>().animation = m_game->getAssets().getAnimation("Explosion");
			}
			else
			{
				init(m_levelPath);
			}
		}
	}
}

void ScenePlay::solveCollision(std::shared_ptr<Entity>entity1, std::shared_ptr<Entity>entity2, const Vec2& overlap)
{
	Vec2 prevOverlap = Physics::GetPreviousOverlap(entity1, entity2);
	auto& entity1Transform = entity1->getComponent<CTransform>();
	auto& entity2Transform = entity2->getComponent<CTransform>();

	if (prevOverlap.x > 0) {
		if (entity1Transform.pos.y > entity2Transform.pos.y) {
			if (entity2->getComponent<CAnimation>().animation.getName() != "Black")
			{
				entity1Transform.pos.y += overlap.y;
			}
			else
			{
				size_t randomPoint = 0 + rand() % m_teleportPoints.size();
				if (m_teleportPoints[randomPoint] == entity2Transform.pos)
				{
					randomPoint = (randomPoint + 1) % m_teleportPoints.size();
				}
				entity1Transform.pos = Vec2(m_teleportPoints[randomPoint].x, m_teleportPoints[randomPoint].y + m_gridSize.y + 32.0f);
			}
		}
		else {
			entity1Transform.pos.y -= overlap.y;
		}
	}

	if (prevOverlap.y > 0) {
		if (entity1Transform.pos.x > entity2Transform.pos.x) {
			entity1Transform.pos.x += overlap.x;
		}
		else {
			entity1Transform.pos.x -= overlap.x;
		}
	}
}

void ScenePlay::sCollision() {
	const float EPSILON = 1e-5f;
	Vec2 totalAdjustment(0.0f, 0.0f);

	for (auto& sword : m_entities.getEntities("Sword"))
	{
		for (auto& enemy : m_entities.getEntities("Enemy"))
		{
			Vec2 overlap = Physics::GetOverlap(sword, enemy);
			if (overlap != Vec2(0.0f, 0.0f))
			{
				damage(enemy, 15, sword->getComponent<CDamage>().damage);
				break;
			}
		}
	}

	for (auto& enemy : m_entities.getEntities("Enemy"))
	{
		Vec2 overlap = Physics::GetOverlap(m_player, enemy);
		if (overlap != Vec2(0.0f, 0.0f))
		{
			damage(m_player, 60, enemy->getComponent<CDamage>().damage);
		}
		for (auto& tile : m_entities.getEntities("Tile"))
		{
			overlap = Physics::GetOverlap(tile, enemy);
			if (overlap != Vec2(0.0f, 0.0f))
			{
				if (tile->getComponent<CBoundingBox>().boxColor == sf::Color::Black || tile->getComponent<CBoundingBox>().boxColor == sf::Color::Blue)
				{
					if (tile->getComponent<CAnimation>().animation.getName() == "Heart")
					{
						enemy->getComponent<CHealthBar>().remaining = enemy->getComponent<CHealthBar>().size;
						tile->destroy();
					}
					else
					{
						solveCollision(enemy, tile, overlap);
					}
				}
			}
		}
	}

	for (auto& tile : m_entities.getEntities("Tile"))
	{
		Vec2 overlap = Physics::GetOverlap(m_player, tile);
		if (overlap != Vec2(0.0f, 0.0f))
		{
			if (tile->getComponent<CBoundingBox>().boxColor == sf::Color::Black || tile->getComponent<CBoundingBox>().boxColor == sf::Color::Blue)
			{
				if (tile->getComponent<CAnimation>().animation.getName() == "Heart")
				{
					m_player->getComponent<CHealthBar>().remaining = m_player->getComponent<CHealthBar>().size;
					tile->destroy();
				}
				else
				{
					solveCollision(m_player, tile, overlap);
				}
			}
		}
	}
}

void ScenePlay::update() {
	m_entities.update();
	if (!m_paused) {
		sMovement();
		sCollision();
		sAnimation();
		sLifeSpan();
	}
}


void ScenePlay::onEnd()
{
	for (auto& entity : m_entities.getEntities())
	{
		entity->destroy();
	}
	m_game->changeScene("MENU", std::make_shared<SceneMenu>(m_game));
}

ScenePlay::Intersect ScenePlay::intersection(const Vec2& a, const Vec2& b)
{
	Vec2 r = b - a;
	Vec2 c, d;

	std::vector<Vec2> points(4);
	for (auto& entity : m_entities.getEntities())
	{
		if (entity->tag() == "Player")
		{
			continue;
		}
		else
		{
			if (entity->hasComponent<CBoundingBox>())
			{
				auto& boundComponent = entity->getComponent<CBoundingBox>();
				if (boundComponent.boxColor == sf::Color::Black || boundComponent.boxColor == sf::Color::Red)
				{

					points[0] = Vec2(boundComponent.rectangle.getPosition().x - boundComponent.halfSize.x, boundComponent.rectangle.getPosition().y - boundComponent.halfSize.y);
					points[1] = Vec2(boundComponent.rectangle.getPosition().x + boundComponent.halfSize.x, boundComponent.rectangle.getPosition().y - boundComponent.halfSize.y);
					points[2] = Vec2(boundComponent.rectangle.getPosition().x + boundComponent.halfSize.x, boundComponent.rectangle.getPosition().y + boundComponent.halfSize.y);
					points[3] = Vec2(boundComponent.rectangle.getPosition().x - boundComponent.halfSize.x, boundComponent.rectangle.getPosition().y + boundComponent.halfSize.y);

					for (size_t i = 0; i < 4; i++)
					{
						c = points[i];
						d = points[(i + 1) % 4];
						Vec2 s = d - c;
						float rxs = r.x * s.y - r.y * s.x;

						if (rxs == 0) continue; // Parallel lines

						Vec2 cma = c - a;
						float t = (cma.x * s.y - cma.y * s.x) / rxs;
						float u = (cma.x * r.y - cma.y * r.x) / rxs;
						if ((t > 0 && t < 1) && (u > 0 && u < 1))
						{
							return { true };
						}
					}
				}
			}
			else
			{
				continue;
			}
		}
	}
	return { false };
}

void ScenePlay::sMovement()
{
	Vec2 playerVelocity(0, 0);
	if (m_player->getComponent<CInput>().right)
	{
		playerVelocity.x = 4.0;
		m_player->getComponent<CState>().state = "RunSide";
		m_player->getComponent<CTransform>().angle = 1.0f;
	}

	else if (m_player->getComponent<CInput>().left)
	{
		playerVelocity.x = -4.0;
		m_player->getComponent<CState>().state = "RunSide";
		m_player->getComponent<CTransform>().angle = -1.0f;
	}

	else if (m_player->getComponent<CInput>().up)
	{
		playerVelocity.y = -4.0;
		m_player->getComponent<CState>().state = "RunUp";
	}

	else if (m_player->getComponent<CInput>().down)
	{
		playerVelocity.y = 4.0;
		m_player->getComponent<CState>().state = "RunDown";
	}

	/*
	if (m_player->getComponent<CTransform>().pos.x - m_player->getComponent<CBoundingBox>().halfSize.x <= 0)
	{
		playerVelocity.x = 0.1f;
	}

	if (m_player->getComponent<CTransform>().pos.x + m_player->getComponent<CBoundingBox>().halfSize.x >= m_game->m_widthW)
	{
		playerVelocity.x = -0.1f;
	}
	*/

	m_player->getComponent<CTransform>().velocity = playerVelocity;

	for (auto& e : m_entities.getEntities())
	{
		if (e->getComponent<CTransform>().velocity.y > 10)
		{
			e->getComponent<CTransform>().velocity.y = 10;
		}
		if (e->getComponent<CTransform>().velocity.x > 10)
		{
			e->getComponent<CTransform>().velocity.x = 10;
		}
		if (e->tag() == "Sword")
		{
			e->getComponent<CTransform>().velocity = playerVelocity;
		}

		if (e->tag() == "Enemy")
		{
			auto& patrolComponent = e->getComponent<CPatrol>();
			size_t size = patrolComponent.patrolPoints.size();
			if (patrolComponent.patrolling)
			{
				if (patrolComponent.patrolPoints.size() > 1)
				{
					float theta;
					Vec2 first(patrolComponent.patrolReference[patrolComponent.current].getPosition().x, patrolComponent.patrolReference[patrolComponent.current].getPosition().y);
					Vec2 second(patrolComponent.patrolReference[(patrolComponent.current + 1) % size].getPosition().x, patrolComponent.patrolReference[(patrolComponent.current + 1) % size].getPosition().y);
					theta = second.angle(first);
					if (second.distq(e->getComponent<CTransform>().pos) <= 5.0f)
					{
						patrolComponent.current = (patrolComponent.current + 1) % size;
						first = Vec2(patrolComponent.patrolReference[patrolComponent.current].getPosition().x, patrolComponent.patrolReference[patrolComponent.current].getPosition().y);
						second = Vec2(patrolComponent.patrolReference[(patrolComponent.current + 1) % size].getPosition().x, patrolComponent.patrolReference[(patrolComponent.current + 1) % size].getPosition().y);
						theta = second.angle(first);
					}
					e->getComponent<CTransform>().velocity = Vec2(e->getComponent<CTransform>().speed * cos(theta), e->getComponent<CTransform>().speed * sin(theta));
				}
			}
			if (e->hasComponent<CChase>())
			{
				auto& chaseComponent = e->getComponent<CChase>();
				chaseComponent.line[0] = sf::Vertex(sf::Vector2f(e->getComponent<CTransform>().pos.x, e->getComponent<CTransform>().pos.y), sf::Color::Black);
				chaseComponent.line[1] = sf::Vertex(sf::Vector2f(m_player->getComponent<CTransform>().pos.x, m_player->getComponent<CTransform>().pos.y), sf::Color::Black);
				ScenePlay::Intersect intersectResult = intersection(e->getComponent<CTransform>().pos, m_player->getComponent<CTransform>().pos);
				if (intersectResult.result)
				{
					if (Vec2(e->getComponent<CPatrol>().patrolReference[0].getPosition().x, e->getComponent<CPatrol>().patrolReference[0].getPosition().y).distq(e->getComponent<CTransform>().pos) >= 2.5f && !patrolComponent.patrolling)
					{
						float theta = Vec2(e->getComponent<CPatrol>().patrolReference[0].getPosition().x, e->getComponent<CPatrol>().patrolReference[0].getPosition().y).angle(e->getComponent<CTransform>().pos);
						e->getComponent<CTransform>().velocity = Vec2(e->getComponent<CTransform>().speed * cos(theta), e->getComponent<CTransform>().speed * sin(theta));
						patrolComponent.patrolling = false;
					}
					else
					{
						//e->getComponent<CTransform>().velocity = Vec2(0, 0);
						if (!patrolComponent.patrolling)
						{
							if (patrolComponent.patrolPoints.size() > 1)
							{
								patrolComponent.current = 0;
							}
							else
							{
								e->getComponent<CTransform>().velocity = Vec2(0, 0);

							}
							patrolComponent.patrolling = true;
						}
					}
					//patrolComponent.patrolling = true;
				}
				else
				{
					patrolComponent.patrolling = false;
					float theta = m_player->getComponent<CTransform>().pos.angle(e->getComponent<CTransform>().pos);
					e->getComponent<CTransform>().velocity = Vec2(e->getComponent<CTransform>().speed * cos(theta), e->getComponent<CTransform>().speed * sin(theta));
				}
			}
		}

		e->getComponent<CTransform>().prevPos = e->getComponent<CTransform>().pos;
		e->getComponent<CTransform>().pos += e->getComponent<CTransform>().velocity;
		if (e->hasComponent<CBoundingBox>())
		{
			e->getComponent<CBoundingBox>().rectangle.setPosition(e->getComponent<CTransform>().pos.x, e->getComponent<CTransform>().pos.y);
		}
	}
}

void ScenePlay::sRender()
{
	m_game->m_window.clear(sf::Color(220, 222, 217));
	sDebug();

	m_mShape.setFillColor(sf::Color::Red);
	m_mShape.setRadius(4);
	m_mShape.setOrigin(2, 2);

	Vec2 worldPos = windowToWorld(m_mPos);
	m_mShape.setPosition(worldPos.x, worldPos.y);
	m_game->m_window.draw(m_mShape);

	if (m_paused)
	{
		m_playText.setString("PAUSED");
		m_playText.setCharacterSize(100);
		m_playText.setOrigin(200, 50);
		m_playText.setPosition(m_view.getCenter().x, m_view.getCenter().y);
		m_game->m_window.draw(m_playText);
	}

	m_game->m_window.display();
}