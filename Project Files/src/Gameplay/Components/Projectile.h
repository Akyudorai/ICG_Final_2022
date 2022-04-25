#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"

/// <summary>
/// A simple behaviour that applies an impulse along the Z axis to the 
/// rigidbody of the parent when the space key is pressed
/// </summary>
class Projectile : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<Projectile> Sptr;

	Projectile();
	virtual ~Projectile();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(Projectile);
	virtual nlohmann::json ToJson() const override;
	static Projectile::Sptr FromJson(const nlohmann::json& blob);

protected:
	
	Gameplay::Physics::RigidBody::Sptr _body;
};