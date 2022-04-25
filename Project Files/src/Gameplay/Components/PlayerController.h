#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Material.h"

/// <summary>
/// A simple behaviour that applies an impulse along the Z axis to the 
/// rigidbody of the parent when the space key is pressed
/// </summary>
class PlayerController : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<PlayerController> Sptr;

	PlayerController();
	virtual ~PlayerController();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(PlayerController);
	virtual nlohmann::json ToJson() const override;
	static PlayerController::Sptr FromJson(const nlohmann::json& blob);

public:
	int health = 2;
	void DealDamage();
	inline void SetMatRef(Gameplay::Material::Sptr mat) { _matRef = mat; }

protected:
	Gameplay::Physics::RigidBody::Sptr _body;

	Gameplay::Material::Sptr _matRef;
	float damageReceivedTimer = 0.0f;
};