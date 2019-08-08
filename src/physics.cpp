
#include "log.h"
#include "physics.h"
#include "LinearMath/btDefaultMotionState.h"
#include "BulletCollision/CollisionShapes/btStaticPlaneShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
#include "LinearMath/btIDebugDraw.h"
#include <mat4x4.hpp>
#include <vec3.hpp>
#include "graphics/platform_gl.h"
#include "renderer/progs_manager.h"

extern bool noclip;

PhysicsSystem g_physics;

class MyDebugDraw: public btIDebugDraw
{
	int mode;

	// btIDebugDraw interface
public:
	void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
	{
		g_progsManager.SetMtx(glm::mat4(1.0));

		float verts[6] = {from.x(),from.y(),from.z(), to.x(), to.y(), to.z()};
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, verts);
		g_progsManager.SetColor(glm::vec4(color.x(),color.y(),color.z(),1));
		glDrawArrays(GL_LINES,0,2);
	}
	void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)
	{
	}
	void reportErrorWarning(const char *warningString)
	{
		LOG("Physics: %s\n",warningString);
	}
	void draw3dText(const btVector3 &location, const char *textString)
	{
		//TODO
		LOG("Physics 3dtext: %s\n",textString);
	}
	void setDebugMode(int debugMode)
	{
		mode = debugMode;
	}
	int getDebugMode() const
	{
		return mode;
	}
};

void PhysicsSystem::Init()
{
	LOG("Init physics system\n");
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfig);
	broadphase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver();
	world = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfig);
	//TODO flip gravity axis
	world->setGravity(btVector3(0, -10, 0));

	debugDraw = new MyDebugDraw();
	world->setDebugDrawer(debugDraw);

	//test
/*	btCollisionShape *groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);

	btDefaultMotionState *groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
	mass = 0;
	rigidBodyCI = btRigidBody::btRigidBodyConstructionInfo (mass, groundMotionState, groundShape);
	btRigidBody *groundRigidBody = new btRigidBody(rigidBodyCI);
	groundRigidBody->setRestitution(0.5f);
	world->addRigidBody(groundRigidBody);
*/
	btCollisionShape *fallShape = new btBoxShape(btVector3(0.5,0.5,0.5));
	btDefaultMotionState* fallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(3, 3, -2)));
	float mass = 1;
	btVector3 fallInertia(0, 0, 0);
	fallShape->calculateLocalInertia(mass, fallInertia);
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, fallMotionState, fallShape, fallInertia);
	fallRigidBody = new btRigidBody(rigidBodyCI);
	fallRigidBody->setRestitution(1.0f);
	world->addRigidBody(fallRigidBody);
}

void PhysicsSystem::InitPlayer(glm::vec3 pos)
{
	btCollisionShape* shape = new btCapsuleShape(0.5, 0.8);//1.8);

	//btTransform startTransform;
	//startTransform.setFromOpenGLMatrix((float*)&mtx);
	//btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
	btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), *(btVector3*)&pos));
	btScalar mass = 50;
	btVector3 inertia(0, 0, 0);
	shape->calculateLocalInertia(mass, inertia);

	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
	playerRigidBody = new btRigidBody(rigidBodyCI);
	playerRigidBody->setRestitution(0.2f);
	//playerRigidBody->setFriction(0.8f);
	playerRigidBody->setAngularFactor(0);
	world->addRigidBody(playerRigidBody);
}

void PhysicsSystem::SetPlayerPos(glm::vec3 pos)
{
	playerRigidBody->setWorldTransform(btTransform(btQuaternion(0, 0, 0, 1), *(btVector3*)&pos));
	playerRigidBody->setLinearVelocity(btVector3(0,0,0));
}

extern Camera camera;
void PhysicsSystem::Update(float deltaTime)
{
	world->stepSimulation(deltaTime, 10);

	if(!noclip)
	{
		btTransform trans;
		playerRigidBody->getMotionState()->getWorldTransform(trans);
		camera.pos = *(glm::vec3*)&trans.getOrigin();
		camera.pos.y+=0.6;
	}

	/*for(int i=0;i<curRigidBodies;i++)
	{
		rigidBodies[i]->getMotionState()->getWorldTransform(trans);
		trans.getOpenGLMatrix((GLfloat*)&renderObjects[i].modelMtx);
		DrawBoxShape(renderObjects[i].modelMtx, renderObjects[i].boxSize, glm::vec4(1.0,1.0,1.0,1.0));
	}*/
}

void DrawBoxLines(glm::mat4 mtx, glm::vec3 mins, glm::vec3 maxs, glm::vec4 color);
void PhysicsSystem::Draw()
{
	/*btTransform trans;
	glm::mat4 mtx;
	fallRigidBody->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix((float*)&mtx);
	DrawBoxLines(mtx, glm::vec3(-0.5),glm::vec3(0.5), glm::vec4(1.0,1.0,1.0,1.0));

	playerRigidBody->getMotionState()->getWorldTransform(trans);
	trans.getOpenGLMatrix((float*)&mtx);
	DrawBoxLines(mtx, glm::vec3(-0.25,-0.9,-0.25),glm::vec3(0.25,0.9,0.25), glm::vec4(1.0,1.0,1.0,1.0));
*/
	world->debugDrawWorld();
}

float maxVelChange = 10;
void PhysicsSystem::Move(float x, float z)
{
#if 0
	playerRigidBody->activate(true);
	//playerRigidBody->applyCentralImpulse(btVector3(x*force,0,y*force));
	float y = playerRigidBody->getLinearVelocity().getY();
	playerRigidBody->setLinearVelocity(btVector3(x,y,z));
#else
	btVector3 vel = playerRigidBody->getLinearVelocity();
	btVector3 velChange(glm::clamp(x-vel.x(),-maxVelChange,maxVelChange),0,glm::clamp(z-vel.z(),-maxVelChange,maxVelChange));
	playerRigidBody->activate(true);
	playerRigidBody->setLinearVelocity(playerRigidBody->getLinearVelocity()+velChange);
#endif
}

void PhysicsSystem::Jump()
{
	playerRigidBody->activate(true);
	playerRigidBody->applyCentralImpulse(btVector3(0,300,0));
}

btRigidBody *PhysicsSystem::CreateRigidBody(float mass, btTransform tr, btCollisionShape *shape)
{
	LOG("Physics: CreateRigidBody (mass %f, shape %p)\n",mass,shape);
	btDefaultMotionState* motionState = new btDefaultMotionState(tr);
	btVector3 inertia(0, 0, 0);
	//if(mass>0)
		shape->calculateLocalInertia(mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
	btRigidBody *body = new btRigidBody(rigidBodyCI);

	world->addRigidBody(body);
	return body;
}

void PhysicsSystem::SetDebugDrawMode(int m)
{
	debugDraw->setDebugMode(m);
}

bool PhysicsSystem::RayCast(glm::vec3 from, glm::vec3 to, glm::vec3 *out)
{
	btCollisionWorld::ClosestRayResultCallback result(*(btVector3*)&from,*(btVector3*)&to);
	result.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;

	world->rayTest(*(btVector3*)&from,*(btVector3*)&to,result);

	if(result.hasHit())
	{
		*out = glm::mix(from,to,result.m_closestHitFraction);
		return true;
	}
	return false;
}
