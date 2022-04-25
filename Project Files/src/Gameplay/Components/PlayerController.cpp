#include "Gameplay/Components/PlayerController.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"
#include "Application/Timing.h"

#include "Gameplay/Material.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Graphics/ShaderProgram.h"

void PlayerController::Awake()
{
	Timing& t = Timing::Current();
	t.SetTimeScale(0.0f);
}

void PlayerController::RenderImGui() {
}

nlohmann::json PlayerController::ToJson() const {
	return {

	};
}

PlayerController::PlayerController() :
	IComponent()
{ }

PlayerController::~PlayerController() = default;

PlayerController::Sptr PlayerController::FromJson(const nlohmann::json & blob) {
	PlayerController::Sptr result = std::make_shared<PlayerController>();
	return result;
}

void PlayerController::Update(float deltaTime) {

	if (InputEngine::GetKeyState(GLFW_KEY_SPACE) == ButtonState::Pressed) {
		Timing& t = Timing::Current();
		t.SetTimeScale(1.0f);
	}

	glm::vec3 newPos = GetGameObject()->GetPosition() + glm::vec3(deltaTime/2, 0.0f, 0.0f);

	if (InputEngine::GetKeyState(GLFW_KEY_UP) == ButtonState::Down && GetGameObject()->GetPosition().y < 6.5f) {
		newPos += glm::vec3(0.0, deltaTime, 0.0f);
	}

	if (InputEngine::GetKeyState(GLFW_KEY_DOWN) == ButtonState::Down && GetGameObject()->GetPosition().y > -7.5f) {
		newPos += glm::vec3(0.0, -deltaTime, 0.0f);
	}
	
	GetGameObject()->SetPostion(newPos);

	if (damageReceivedTimer > 0) {
		damageReceivedTimer -= deltaTime;
	}

	if (damageReceivedTimer < 0) {
		GetGameObject()->Get<RenderComponent>()->GetMaterial()->GetShader()->SetUniform("u_Effect.Enabled", false);
		damageReceivedTimer = 0;
	}
}

void PlayerController::DealDamage() {
	health -= 2;

	if (health <= 0) {
		// LOSS CONDITION
		LOG_INFO("PLAYER LOSE");
		Timing& t = Timing::Current();
		t.SetTimeScale(0.0f);
	}
	else {
		GetGameObject()->Get<RenderComponent>()->GetMaterial()->GetShader()->SetUniform("u_Effect.Enabled", true);
		damageReceivedTimer = 1.0f;
	}
}