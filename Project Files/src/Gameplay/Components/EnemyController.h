#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/GameObject.h"

#include "Gameplay/MeshResource.h"
#include "Gameplay/Material.h"

/// <summary>
/// A simple behaviour that applies an impulse along the Z axis to the 
/// rigidbody of the parent when the space key is pressed
/// </summary>
class EnemyController : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<EnemyController> Sptr;

	EnemyController();
	virtual ~EnemyController();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(EnemyController);
	virtual nlohmann::json ToJson() const override;
	static EnemyController::Sptr FromJson(const nlohmann::json& blob);

	inline void SetPlayerRef(Gameplay::GameObject::Sptr object) { _playerRef = object; }
	inline void SetBulletMesh(Gameplay::MeshResource::Sptr mesh) { bulletMesh = mesh; }
	inline void SetBulletMaterial(Gameplay::Material::Sptr mat) { bulletMat = mat; }

protected:

	Gameplay::GameObject::Sptr _playerRef;
	Gameplay::Physics::RigidBody::Sptr _body;
	float shotTimer = 0.0f;
	float shotCooldown = 5.0f;

	Gameplay::Material::Sptr bulletMat;
	Gameplay::MeshResource::Sptr bulletMesh;
};