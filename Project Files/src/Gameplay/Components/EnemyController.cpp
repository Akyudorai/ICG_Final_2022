#include "Gameplay/Components/EnemyController.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"

#include "RenderComponent.h"
#include "Projectile.h"
#include "TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Application/Timing.h"

void EnemyController::Awake()
{

}

void EnemyController::RenderImGui() {
}

nlohmann::json EnemyController::ToJson() const {
	return {
		
	};
}

EnemyController::EnemyController() :
	IComponent()
{ }

EnemyController::~EnemyController() = default;

EnemyController::Sptr EnemyController::FromJson(const nlohmann::json & blob) {
	EnemyController::Sptr result = std::make_shared<EnemyController>();
	return result;
}

void EnemyController::Update(float deltaTime) {
	
	glm::vec3 newPos = GetGameObject()->GetPosition();
	newPos.y = _playerRef->GetPosition().y;
	GetGameObject()->SetPostion(newPos);

	if (shotTimer > 0) {
		shotTimer -= deltaTime;
	}
	else {
		shotTimer = shotCooldown;

		Gameplay::GameObject::Sptr bullet = GetGameObject()->GetScene()->CreateGameObject("Bullet");
		{
			// Set position in the scene
			bullet->SetPostion(GetGameObject()->GetPosition());
			bullet->SetRotation({ 0, 0, 0 });
			bullet->SetScale({ 0.5f, 0.5f, 0.5f });

			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = bullet->Add<RenderComponent>();
			renderer->SetMesh(bulletMesh);
			renderer->SetMaterial(bulletMat);

			// Projectile!
			Projectile::Sptr projectile = bullet->Add<Projectile>();
			
			Gameplay::Physics::TriggerVolume::Sptr volume = bullet->Add<Gameplay::Physics::TriggerVolume>();
			Gameplay::Physics::BoxCollider::Sptr collider = Gameplay::Physics::BoxCollider::Create(glm::vec3(0.5f, 0.5f, 0.5f));
			collider->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			volume->AddCollider(collider);

			bullet->Add<TriggerVolumeEnterBehaviour>();
		}
	}

	// Win Condition
	if (glm::distance(newPos, _playerRef->GetPosition()) < 2.5f) {
		LOG_INFO("Player Wins!");	

		Timing& t = Timing::Current();
		t.SetTimeScale(0.0f);
	}

}