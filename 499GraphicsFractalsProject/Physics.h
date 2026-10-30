#pragma once

#define ENABLE_PHYSICS 0 //physics is a work in progress
#if ENABLE_PHYSICS

#include <vector>

#include <stdexcept>


#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/btBulletCollisionCommon.h>
#include "SparseSet.h"

struct Transform
{
	btVector3 position{};
	btQuaternion rotation{};
};

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

class MotionState : public btMotionState
{
public:
	Transform* m_transform{ nullptr };

	void getWorldTransform(btTransform& worldTrans) const override
	{
		if (!m_transform) return;
		worldTrans.setOrigin(m_transform->position);
		worldTrans.setRotation(m_transform->rotation);
	}

	void setWorldTransform(const btTransform& worldTrans)
	{
		m_transform->position = worldTrans.getOrigin();
		m_transform->rotation = worldTrans.getRotation();
	}

};



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
	btVehicleRaycaster* m_vehicleRaycaster{ nullptr };

	static constexpr size_t MAX_COMPONENTS = 128;
	static constexpr size_t MAX_ENTITIES = 1000;
	SparseSet<btCollisionShape*, MAX_ENTITIES, MAX_ENTITIES> m_collisionShapes{};
	SparseSet<btRigidBody*, MAX_ENTITIES, MAX_ENTITIES> m_rigidbodies{};
	SparseSet<btRaycastVehicle*, MAX_ENTITIES, MAX_ENTITIES> m_vehicles{};
	SparseSet<Transform*, MAX_ENTITIES, MAX_ENTITIES> m_vehicles{};

	RigidBodyID m_nextRBID = 0;

	void destroy()
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

	void getTransform(glm::mat4* transformOut, RigidBodyID id)
	{

	}

	void runSimulation(float deltaTime)
	{
		m_dynamicsWorld->stepSimulation(deltaTime, 10);
	}

	RigidBodyID createRigidBodySphere(
		const float& radius
		, glm::vec3 startOrigin
		, glm::quat startRotation
		, const float& mass)
	{
		Transform* transform = new Transform();
		btVector3 momentOfInertia = { 1.f,1.f,1.f };
		RigidBodyID rbid = m_nextRBID;
		m_nextRBID++;

		transform->position = glmToBt(startOrigin);
		transform->rotation = glmToBt(startRotation);



		btVector3 startOrigin(transform->position);
		btCollisionShape* collisionShape = new btSphereShape(radius);

		collisionShape->calculateLocalInertia(mass, momentOfInertia);
		MotionState* motionState = new MotionState();
		motionState->m_transform = transform;

		if (command.isKinematic)
		{
			mass = 0.0f;
			momentOfInertia = { 0,0,0 };
		}

		btRigidBody::btRigidBodyConstructionInfo rbInfo{ mass, motionState, collisionShape, momentOfInertia };
		btRigidBody* rigidBody = new btRigidBody(rbInfo);

		if (command.isKinematic)
		{
			rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			rigidBody->setActivationState(DISABLE_DEACTIVATION);
		}


		m_rigidbodies.insert(command.entity, rigidBody);

		m_dynamicsWorld->addRigidBody(rigidBody);
	}


};

#endif