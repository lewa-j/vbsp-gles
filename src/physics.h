
#pragma once

#include "BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include <vec3.hpp>

class PhysicsSystem
{
	btCollisionConfiguration *collisionConfig;
	btDispatcher *dispatcher;
	btBroadphaseInterface *broadphase;
	btConstraintSolver *solver;
	btDynamicsWorld *world;
	btIDebugDraw *debugDraw;

	btRigidBody *playerRigidBody;
	btRigidBody *fallRigidBody;

public:
	void Init();
	void Update(float deltaTime);
	void Draw();
	void InitPlayer(glm::vec3 pos);
	void SetPlayerPos(glm::vec3 pos);
	void Move(float x, float z);
	void Jump();
	btRigidBody *CreateRigidBody(float mass, btTransform tr, btCollisionShape *shape);
	void SetDebugDrawMode(int m);
	bool RayCast(glm::vec3 from, glm::vec3 to, glm::vec3 *out);
};

extern PhysicsSystem g_physics;
