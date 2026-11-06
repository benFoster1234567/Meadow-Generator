#pragma once

#include <stdexcept>

#include <vector>
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/btBulletCollisionCommon.h>
#include <glm/ext/quaternion_float.hpp>

#include "SparseSet.h"
#include "Transform.h"
#include "Collider.h"

//-----------------------------------------------------------------
//helpers
static btVector3 glmToBt(const glm::vec3& v)
{
	return btVector3{ v.x, v.y, v.z };
}

static btQuaternion glmToBt(const glm::quat& q)
{
	return btQuaternion{ q.x, q.y, q.z, q.w };
}

static glm::vec3 btToGlm(const btVector3& v)
{
	return glm::vec3{ v.x(), v.y(), v.z() };
}

static glm::quat btToGlm(const btQuaternion& q)
{
	return glm::quat{ q.w(), q.x(), q.y(), q.z() };
}

static glm::vec3 extractVec3FromMat(const glm::mat4& m)
{
	return { m[3][0], m[3][1], m[3][2] };
}


//-----------------------------------------------------------------
class MotionState : public btMotionState
{
	btVector3 m_position{ 0.f,0.f,0.f };
	btQuaternion m_rotation{ 0.f,0.f,0.f, 1.f };

public:
	void getWorldTransform(btTransform& worldTrans) const override
	{
		worldTrans.setOrigin(m_position);
		worldTrans.setRotation(m_rotation);
	}

	void setWorldTransform(const btTransform& worldTrans)
	{
		m_position = worldTrans.getOrigin();
		m_rotation = worldTrans.getRotation();
	}

	glm::vec3 getPositionGlm() const { return btToGlm(m_position); }
	glm::quat getRotationGlm() const { return btToGlm(m_rotation); }

	btVector3 getPositionBt() const { return m_position; }
	btQuaternion getRotationBt() const { return m_rotation; }

	void setPosition(const glm::vec3& p) { m_position = glmToBt(p); }
	void setRotation(const glm::quat& r) { m_rotation = glmToBt(r); }
};

//-----------------------------------------------------------------
class BtPhysicsEngine
{
public:
	using RigidBodyID = size_t;
private:
	btDefaultCollisionConfiguration* m_collisionConfiguration{ nullptr };
	btCollisionDispatcher* m_dispatcher{ nullptr };
	btDbvtBroadphase* m_broadPhase{ nullptr };
	btSequentialImpulseConstraintSolver* m_solver{ nullptr };
	btDiscreteDynamicsWorld* m_dynamicsWorld{ nullptr };

	static constexpr size_t MAX_COMPONENTS = 128;
	static constexpr size_t MAX_ENTITIES = 1000;

	SparseSet<btCollisionShape*, MAX_ENTITIES, MAX_ENTITIES>
		m_collisionShapes{};

	SparseSet<btRigidBody*, MAX_ENTITIES, MAX_ENTITIES>
		m_rigidbodies{};

	RigidBodyID m_nextRBID = 1;

	RigidBodyID createRigidBody
	(
		btCollisionShape* collisionShape
		, const Transform& transform
		, float mass
		, const bool& isKinematic
		, const bool& isActive
	)
	{
		btVector3 momentOfInertia = { 1.f,1.f,1.f };
		RigidBodyID rbid = m_nextRBID;
		m_nextRBID++;

		collisionShape->calculateLocalInertia(mass, momentOfInertia);
		MotionState* motionState = new MotionState();
		motionState->setPosition(transform.getPosition());
		motionState->setRotation(transform.getRotation());

		if (isKinematic)
		{
			mass = 0.0f;
			momentOfInertia = { 0,0,0 };
		}

		btRigidBody::btRigidBodyConstructionInfo rbInfo{ mass, motionState, collisionShape, momentOfInertia };
		btRigidBody* rigidBody = new btRigidBody(rbInfo);

		if (isKinematic)
		{
			rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			rigidBody->setActivationState(DISABLE_DEACTIVATION);
		}

		rigidBody->setActivationState(isActive ? ACTIVE_TAG : ISLAND_SLEEPING);

		m_rigidbodies.insert(rbid, rigidBody);
		m_dynamicsWorld->addRigidBody(rigidBody);

		return rbid;
	}

	void destroy()
	{
		clear();

		delete m_dynamicsWorld;
		delete m_solver;
		delete m_broadPhase;
		delete m_dispatcher;
		delete m_collisionConfiguration;

		m_dynamicsWorld = nullptr;
		m_solver = nullptr;
		m_broadPhase = nullptr;
		m_dispatcher = nullptr;
		m_collisionConfiguration = nullptr;
	}

	MotionState* getRigidbodyMotionState(const RigidBodyID& id)
	{
		if (!m_rigidbodies.contains(id)) return nullptr;
		return static_cast<MotionState*>(m_rigidbodies.get(id)->getMotionState());
	}

public:

	BtPhysicsEngine()
	{
		m_broadPhase = new btDbvtBroadphase();
		m_collisionConfiguration = new btDefaultCollisionConfiguration();
		m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
		m_solver = new btSequentialImpulseConstraintSolver();
		m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadPhase, m_solver, m_collisionConfiguration);
		m_dynamicsWorld->setGravity(btVector3(0.0f, -9.81f, 0.0f));
	}

	~BtPhysicsEngine()
	{
		destroy();
	}

	BtPhysicsEngine(const BtPhysicsEngine&) = delete;
	BtPhysicsEngine& operator=(const BtPhysicsEngine&) = delete;

	BtPhysicsEngine(BtPhysicsEngine&& other) noexcept
		: m_collisionConfiguration(std::exchange(other.m_collisionConfiguration, nullptr)),
		m_dispatcher(std::exchange(other.m_dispatcher, nullptr)),
		m_broadPhase(std::exchange(other.m_broadPhase, nullptr)),
		m_solver(std::exchange(other.m_solver, nullptr)),
		m_dynamicsWorld(std::exchange(other.m_dynamicsWorld, nullptr)),
		m_collisionShapes(std::move(other.m_collisionShapes))
	{
	}

	void clear()
	{
		if (m_dynamicsWorld)
		{
			for (int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
			{
				btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
				btRigidBody* body = btRigidBody::upcast(obj);

				if (body && body->getMotionState())
				{
					delete body->getMotionState();
				}

				m_dynamicsWorld->removeCollisionObject(obj);
				delete obj;
			}
		}

		for (auto& collisionShape : m_collisionShapes)
		{
			delete collisionShape;
		}
		m_nextRBID = 1;
		m_rigidbodies.clear();
		m_collisionShapes.clear();

	}

	void runSimulation(float deltaTime)
	{
		m_dynamicsWorld->stepSimulation(deltaTime, 10);
	}

	bool getRigidbodyTransform(Transform* transformOut, const RigidBodyID& id)
	{
		MotionState* ms = getRigidbodyMotionState(id);
		transformOut->setPosition(ms->getPositionGlm());
		transformOut->setRotation(ms->getRotationGlm());
		return true;
	}

	bool setRigidbodyTransform(const Transform& transform, const RigidBodyID& id)
	{
		MotionState* ms = getRigidbodyMotionState(id);
		ms->setPosition(transform.getPosition());
		ms->setRotation(transform.getRotation());
		return true;
	}

	bool activateRigidbody(const RigidBodyID& id, const Transform& t)
	{
		if (!m_rigidbodies.contains(id)) return false;
		btRigidBody* rb = m_rigidbodies.get(id);

		btTransform bt;
		bt.setIdentity();
		bt.setOrigin(glmToBt(t.getPosition()));
		bt.setRotation(glmToBt(t.getRotation()));

		rb->setWorldTransform(bt);
		rb->setInterpolationWorldTransform(bt);
		if (auto* ms = rb->getMotionState()) ms->setWorldTransform(bt);

		rb->setLinearVelocity(btVector3(0, 0, 0));
		rb->setAngularVelocity(btVector3(0, 0, 0));
		rb->clearForces();

		rb->forceActivationState(ACTIVE_TAG);
		rb->activate(true);
		return true;
	}

	bool deactivateRigidbody(const RigidBodyID& id)
	{
		if (m_rigidbodies.contains(id))
		{
			auto* rb = m_rigidbodies.get(id);
			rb->setActivationState(DISABLE_SIMULATION);
			return true;
		}
		return false;
	}

	RigidBodyID createRigidBodyBox
	(
		const glm::vec3& halfExtents
		, const Transform& transform
		, const float& mass
		, const bool& isKinematic
		, const bool& isActive
	)
	{
		btBoxShape* collider = new btBoxShape(glmToBt(halfExtents));
		RigidBodyID rid = createRigidBody(collider, transform, mass, isKinematic, isActive);
		m_collisionShapes.insert(rid, collider);

		return rid;
	}

	RigidBodyID createRigidBodySphere
	(
		const float& radius
		, const Transform& transform
		, const float& mass
		, const bool& isKinematic
		, const bool& isActive
	)
	{
		btSphereShape* collider = new btSphereShape(radius);
		RigidBodyID rid = createRigidBody(collider, transform, mass, isKinematic, isActive);
		m_collisionShapes.insert(rid, collider);

		return rid;
	}

	RigidBodyID createRigidBody(
		const Collider& collider
		, const Transform& transform
		, const float& mass
		, const bool& isKinematic
		, const bool& isActive
	)
	{
		return std::visit([&](const auto& shape) -> RigidBodyID
			{
				if constexpr (std::is_same_v<std::decay_t<decltype(shape)>, SphereCollider>)
				{
					return createRigidBodySphere(shape.radius, transform, mass, isKinematic, isActive);
				}

				else if constexpr (std::is_same_v<std::decay_t<decltype(shape)>, BoxCollider>)
				{
					return createRigidBodyBox(shape.halfExtents, transform, mass, isKinematic, isActive);
				}
			}, collider);
	}
};
