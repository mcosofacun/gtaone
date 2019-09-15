#include "stdafx.h"
#include "PhysicsManager.h"
#include "GameMapData.h"

//////////////////////////////////////////////////////////////////////////

union b2FixtureData_map
{
    b2FixtureData_map(void* asPointer = nullptr)
        : mAsPointer(asPointer)
    {
    }

    struct
    {
        unsigned char mZ;
    };

    void* mAsPointer;
};

static_assert(sizeof(b2FixtureData_map) <= sizeof(void*), "Cannot pack data into pointer");

//////////////////////////////////////////////////////////////////////////

PhysicsManager gPhysics;

PhysicsManager::PhysicsManager()
    : mPhysicsWorld()
    , mMapPhysicsBody()
{
    mDebugDraw.SetFlags(0);
}

bool PhysicsManager::Initialize()
{
    b2Vec2 gravity {0.0f, 0.0f}; // default gravity shoild be disabled
    mPhysicsWorld = new b2World(gravity);
    mPhysicsWorld->SetDebugDraw(&mDebugDraw);
    mPhysicsWorld->SetContactFilter(this);

    // create collsition body for map
    mMapPhysicsBody = CreateMapBody();
    debug_assert(mMapPhysicsBody);

    return true;
}

void PhysicsManager::Deinit()
{
    if (mMapPhysicsBody)
    {
        DestroyPhysicsObject(mMapPhysicsBody);
        mMapPhysicsBody = nullptr;
    }
    SafeDelete(mPhysicsWorld);
}

void PhysicsManager::UpdateFrame(Timespan deltaTime)
{
    int maxSimulationStepsPerFrame = 3;
    int numSimulations = 0;

    const float simulationStepF = 1.0f / 60.0f;
    const int velocityIterations = 3; // recommended 8
    const int positionIterations = 3; // recommended 3

    mSimulationTimeAccumulator += deltaTime.ToSeconds();
    while (mSimulationTimeAccumulator > simulationStepF)
    {
        if (++numSimulations > maxSimulationStepsPerFrame)
        {
            mSimulationTimeAccumulator = 0.0f;
            break;
        }
        mSimulationTimeAccumulator -= simulationStepF;
        mPhysicsWorld->Step(simulationStepF, velocityIterations, positionIterations);
    }

    mPhysicsWorld->DrawDebugData();
}

void PhysicsManager::EnableDebugDraw(bool isEnabled)
{
    if (isEnabled)
    {
        mDebugDraw.SetFlags(b2Draw::e_shapeBit);
    }
    else
    {
        mDebugDraw.SetFlags(0);
    }
}

PhysicsObject* PhysicsManager::CreatePedestrianBody(const glm::vec3& position, float angleDegrees)
{
    PhysicsObject* physicsObject = mObjectsPool.create();
    physicsObject->mPhysicsWorld = mPhysicsWorld;

    // create body
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {position.x, position.y};
    bodyDef.angle = glm::radians(angleDegrees);
    bodyDef.fixedRotation = true;
    bodyDef.userData = physicsObject;

    physicsObject->mPhysicsBody = mPhysicsWorld->CreateBody(&bodyDef);
    debug_assert(physicsObject->mPhysicsBody);
    physicsObject->mDepth = 0.5f;
    physicsObject->mZCoord = position.z;
    
    b2CircleShape shapeDef;
    shapeDef.m_radius = PHYSICS_PED_BOUNDING_SPHERE_RADIUS;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shapeDef;
    fixtureDef.density = 10.0f;
    fixtureDef.filter.categoryBits = PHYSICS_OBJCAT_PED;

    b2Fixture* b2fixture = physicsObject->mPhysicsBody->CreateFixture(&fixtureDef);
    debug_assert(b2fixture);

    return physicsObject;
}

PhysicsObject* PhysicsManager::CreateMapBody()
{
    // build object for layer 1

    PhysicsObject* physicsObject = mObjectsPool.create();
    physicsObject->mPhysicsWorld = mPhysicsWorld;

    // create body
    b2BodyDef bodyDef;
    bodyDef.type = b2_staticBody;
    bodyDef.userData = physicsObject;

    physicsObject->mPhysicsBody = mPhysicsWorld->CreateBody(&bodyDef);
    debug_assert(physicsObject->mPhysicsBody);
    physicsObject->mDepth = 1.0f;
    physicsObject->mZCoord = 1.0f;

    // for each block create fixture
    for (int x = 0; x < MAP_DIMENSIONS; ++x)
    for (int y = 0; y < MAP_DIMENSIONS; ++y)
    for (int z = 1; z < MAP_LAYERS_COUNT; ++z)
    {
        MapCoord currentMapCoord { x, y, z };
        BlockStyleData* blockData = gGameMap.GetBlock(currentMapCoord);
        debug_assert(blockData);

        if (blockData->mGroundType != eGroundType_Building)
            continue;

        b2PolygonShape b2shapeDef;
        b2Vec2 center { 
            (x * MAP_BLOCK_LENGTH) + (MAP_BLOCK_LENGTH * 0.5f), 
            (y * MAP_BLOCK_LENGTH) + (MAP_BLOCK_LENGTH * 0.5f)
        };
        b2shapeDef.SetAsBox(MAP_BLOCK_LENGTH * 0.5f, MAP_BLOCK_LENGTH * 0.5f, center, 0.0f);

        b2FixtureData_map fixtureData;
        fixtureData.mZ = z;

        b2FixtureDef b2fixtureDef;
        b2fixtureDef.density = 1.0f;
        b2fixtureDef.shape = &b2shapeDef;
        b2fixtureDef.userData = fixtureData.mAsPointer;
        b2fixtureDef.filter.categoryBits = PHYSICS_OBJCAT_MAP;

        b2Fixture* b2fixture = physicsObject->mPhysicsBody->CreateFixture(&b2fixtureDef);
        debug_assert(b2fixture);
    }
    return physicsObject;
}

void PhysicsManager::DestroyPhysicsObject(PhysicsObject* object)
{
    debug_assert(object);

    mObjectsPool.destroy(object);
}

bool PhysicsManager::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
{
    if (fixtureA == nullptr || fixtureB == nullptr)
        return false;

    const b2Filter& filterA = fixtureA->GetFilterData();
    const b2Filter& filterB = fixtureB->GetFilterData();

    bool ped_vs_map = (filterA.categoryBits | filterB.categoryBits) == (PHYSICS_OBJCAT_PED | PHYSICS_OBJCAT_MAP);
    if (ped_vs_map)
    {
        return ShouldCollide_Ped_vs_Map((filterA.categoryBits & PHYSICS_OBJCAT_PED) ? fixtureA : fixtureB, 
            (filterA.categoryBits & PHYSICS_OBJCAT_MAP) ? fixtureA : fixtureB);
    }
    return false;
}

bool PhysicsManager::ShouldCollide_Ped_vs_Map(b2Fixture* fixturePed, b2Fixture* fixtureMap) const
{
    b2FixtureData_map blockData (fixtureMap->GetUserData());

    PhysicsObject* pedObject = (PhysicsObject*) fixturePed->GetBody()->GetUserData();
    return abs(pedObject->mZCoord - (blockData.mZ * MAP_BLOCK_LENGTH)) < MAP_BLOCK_LENGTH;
}
