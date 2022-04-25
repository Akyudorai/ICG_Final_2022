#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"
#include "Application/Timing.h"
#include "Gameplay/Components/PlayerController.h"

#include "Gameplay/Scene.h"

TriggerVolumeEnterBehaviour::TriggerVolumeEnterBehaviour() :
	IComponent()
{ }
TriggerVolumeEnterBehaviour::~TriggerVolumeEnterBehaviour() = default;


void TriggerVolumeEnterBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	
	_playerInTrigger = true;

	body->GetComponent<PlayerController>()->DealDamage();

	// Destroy the projectile
	GetGameObject()->GetScene()->RemoveGameObject(GetGameObject()->SelfRef());
}

void TriggerVolumeEnterBehaviour::OnTriggerVolumeLeaving(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
	
	_playerInTrigger = false;
}

void TriggerVolumeEnterBehaviour::RenderImGui() { }

nlohmann::json TriggerVolumeEnterBehaviour::ToJson() const {
	return { };
}

TriggerVolumeEnterBehaviour::Sptr TriggerVolumeEnterBehaviour::FromJson(const nlohmann::json& blob) {
	TriggerVolumeEnterBehaviour::Sptr result = std::make_shared<TriggerVolumeEnterBehaviour>();
	return result;
}
