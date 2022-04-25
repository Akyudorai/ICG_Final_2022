#include "Gameplay/Components/Projectile.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"

void Projectile::Awake()
{
	
}

void Projectile::RenderImGui() {
}

nlohmann::json Projectile::ToJson() const {
	return {

	};
}

Projectile::Projectile() :
	IComponent()
{ }

Projectile::~Projectile() = default;

Projectile::Sptr Projectile::FromJson(const nlohmann::json & blob) {
	Projectile::Sptr result = std::make_shared<Projectile>();
	return result;
}

void Projectile::Update(float deltaTime) 
{
	glm::vec3 newPos = GetGameObject()->GetPosition() + glm::vec3(-deltaTime, 0.0f, 0.0f);
	GetGameObject()->SetPostion(newPos);
}