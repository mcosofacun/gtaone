#include "stdafx.h"
#include "GameObjectsManager.h"
#include "Vehicle.h"
#include "Pedestrian.h"
#include "PhysicsBody.h"
#include "GameMapManager.h"
#include "Projectile.h"
#include "RenderingManager.h"

GameObjectsManager gGameObjectsManager;

GameObjectsManager::~GameObjectsManager()
{
    mPedestriansPool.cleanup();
    mCarsPool.cleanup();
    mProjectilesPool.cleanup();
    mDecorationsPool.cleanup();
    mObstaclesPool.cleanup();
    mExplosionsPool.cleanup();

    cxx_assert(mPedestriansList.empty());
    cxx_assert(mVehiclesList.empty());
    cxx_assert(mAllObjects.empty());
}

void GameObjectsManager::EnterWorld()
{
    mIDsCounter = 0;

    if (!CreateStartupObjects())
    {
        gConsole.LogMessage(eLogMessage_Warning, "GameObjectsManager: Cannot create startup objects");
    }
}

void GameObjectsManager::ClearWorld()
{
    DestroyAllObjects();
}

void GameObjectsManager::UpdateFrame()
{
    bool hasDeadObjects = false;

    // if is safe to add new objects during loop by adding them to the end of the list

    for (size_t i = 0, NumElements = mAllObjects.size(); i < NumElements; ++i)
    {
        GameObject* currentObject = mAllObjects[i];
        if (currentObject->IsMarkedForDeletion())
        {
            hasDeadObjects = true;
            continue;
        }
        currentObject->UpdateFrame();
    }

    if (hasDeadObjects)
    {
        DestroyMarkedForDeletionObjects();
    }
}

void GameObjectsManager::DebugDraw(DebugRenderer& debugRender)
{
}

Pedestrian* GameObjectsManager::CreatePedestrian(const glm::vec3& position, cxx::angle_t heading, ePedestrianType pedestrianType, int remap)
{
    GameObjectID pedestrianID = GenerateUniqueID();

    Pedestrian* instance = mPedestriansPool.create(pedestrianID, pedestrianType);
    cxx_assert(instance);
    if (remap != NO_REMAP)
    {
        instance->mRemapIndex = remap;
    }
    mAllObjects.push_back(instance);
    mPedestriansList.push_back(instance);

    // init
    instance->SetTransform(position, heading);
    instance->HandleSpawn();
    return instance;
}

Vehicle* GameObjectsManager::CreateVehicle(const glm::vec3& position, cxx::angle_t heading, VehicleInfo* carStyle)
{
    cxx_assert(gGameMap.mStyleData.IsLoaded());
    cxx_assert(carStyle);
    GameObjectID carID = GenerateUniqueID();

    Vehicle* instance = mCarsPool.create(carID);
    cxx_assert(instance);

    mAllObjects.push_back(instance);
    mVehiclesList.push_back(instance);

    // init
    instance->mCarInfo = carStyle;
    instance->SetTransform(position, heading);
    instance->HandleSpawn();
    return instance;
}

Vehicle* GameObjectsManager::CreateVehicle(const glm::vec3& position, cxx::angle_t heading, eVehicleModel carModel)
{
    Vehicle* vehicle = nullptr;
    for (VehicleInfo& currStyle: gGameMap.mStyleData.mVehicles)
    {
        if (currStyle.mModelID == carModel)
        {
            vehicle = CreateVehicle(position, heading, &currStyle);
            break;
        }
    }
    cxx_assert(vehicle);
    return vehicle;
}

Projectile* GameObjectsManager::CreateProjectile(const glm::vec3& position, cxx::angle_t heading, Pedestrian* shooter)
{
    return CreateProjectile(position, heading, nullptr, shooter);
}

Projectile* GameObjectsManager::CreateProjectile(const glm::vec3& position, cxx::angle_t heading, WeaponInfo* weaponInfo, Pedestrian* shooter)
{
    Projectile* instance = mProjectilesPool.create(weaponInfo, shooter);
    cxx_assert(instance);

    mAllObjects.push_back(instance);
    // init
    instance->SetTransform(position, heading);
    instance->HandleSpawn();
    return instance;
}

Obstacle* GameObjectsManager::CreateObstacle(const glm::vec3& position, cxx::angle_t heading, GameObjectInfo* desc)
{
    Obstacle* instance = nullptr;
    cxx_assert(gGameMap.mStyleData.IsLoaded());
    cxx_assert(desc);
    cxx_assert(desc->mClassID == eGameObjectClass_Obstacle);
    if (desc->mClassID == eGameObjectClass_Obstacle)
    {
        GameObjectID objectID = GenerateUniqueID();

        instance = mObstaclesPool.create(objectID, desc);
        cxx_assert(instance);
        mAllObjects.push_back(instance);
        // init
        instance->SetTransform(position, heading);
        instance->HandleSpawn();
    }
    return instance;
}

Explosion* GameObjectsManager::CreateExplosion(GameObject* explodingObject, Pedestrian* causer, eExplosionType explosionType, const glm::vec3& position)
{
    Explosion* instance = mExplosionsPool.create(explodingObject, causer, explosionType);
    cxx_assert(instance);
    mAllObjects.push_back(instance);
    // init
    static const cxx::angle_t heading;
    instance->SetTransform(position, heading);
    instance->HandleSpawn();
    return instance;
}

Decoration* GameObjectsManager::CreateDecoration(const glm::vec3& position, cxx::angle_t heading, GameObjectInfo* desc)
{
    Decoration* instance = nullptr;
    cxx_assert(gGameMap.mStyleData.IsLoaded());
    cxx_assert(desc);
    cxx_assert(desc->mClassID == eGameObjectClass_Decoration);

    GameObjectID objectID = GenerateUniqueID();

    instance = mDecorationsPool.create(objectID, desc);
    cxx_assert(instance);
    mAllObjects.push_back(instance);
    // init
    instance->SetTransform(position, heading);
    instance->HandleSpawn();
    instance->SetLifeDuration(desc->mLifeDuration);
    return instance;
}

Decoration* GameObjectsManager::CreateFirstBlood(const glm::vec3& position)
{
    GameObjectInfo& objectInfo = gGameMap.mStyleData.mObjects[GameObjectType_FirstBlood];

    cxx::angle_t rotation;
    Decoration* decoration = CreateDecoration(position, rotation, &objectInfo);

    return decoration;
}

Decoration* GameObjectsManager::CreateWaterSplash(const glm::vec3& position)
{
    GameObjectInfo& objectInfo = gGameMap.mStyleData.mObjects[GameObjectType_Splash];

    cxx::angle_t rotation;
    Decoration* decoration = CreateDecoration(position, rotation, &objectInfo);

    return decoration;
}

Decoration* GameObjectsManager::CreateBigSmoke(const glm::vec3& position)
{
    GameObjectInfo& objectInfo = gGameMap.mStyleData.mObjects[GameObjectType_BigSmoke];

    cxx::angle_t rotation;
    Decoration* decoration = CreateDecoration(position, rotation, &objectInfo);
    
    if (decoration) // add some wind effect
    {
        const glm::vec3 velocity (1.0f, 0.0f, 0.0f); // todo: magic values
        decoration->SetMoveVelocity(velocity);
    }

    return decoration;
}

Obstacle* GameObjectsManager::GetObstacleByID(GameObjectID objectID) const
{
    for (GameObject* currentObject: mAllObjects)
    {
        if (currentObject->mObjectID != objectID)
            continue;

        if (currentObject->IsMarkedForDeletion() || !currentObject->IsObstacleClass())
            return nullptr;

        return static_cast<Obstacle*>(currentObject);
    }
    return nullptr;
}

Vehicle* GameObjectsManager::GetVehicleByID(GameObjectID objectID) const
{
    for (GameObject* currentObject: mAllObjects)
    {
        if (currentObject->mObjectID != objectID)
            continue;

        if (currentObject->IsMarkedForDeletion() || !currentObject->IsVehicleClass())
            return nullptr;

        return static_cast<Vehicle*>(currentObject);
    }
    return nullptr;
}

Decoration* GameObjectsManager::GetDecorationByID(GameObjectID objectID) const
{
    for (GameObject* currentObject: mAllObjects)
    {
        if (currentObject->mObjectID != objectID)
            continue;

        if (currentObject->IsMarkedForDeletion() || !currentObject->IsDecorationClass())
            return nullptr;

        return static_cast<Decoration*>(currentObject);
    }
    return nullptr;
}

Pedestrian* GameObjectsManager::GetPedestrianByID(GameObjectID objectID) const
{
    for (GameObject* currentObject: mAllObjects)
    {
        if (currentObject->mObjectID != objectID)
            continue;

        if (currentObject->IsMarkedForDeletion() || !currentObject->IsPedestrianClass())
            return nullptr;

        return static_cast<Pedestrian*>(currentObject);
    }
    return nullptr;
}

GameObject* GameObjectsManager::GetGameObjectByID(GameObjectID objectID) const
{
    for (GameObject* currentObject: mAllObjects)
    {
        if (currentObject->mObjectID != objectID)
            continue;

        if (currentObject->IsMarkedForDeletion())
            return nullptr;

        return currentObject;
    }
    return nullptr;
}

void GameObjectsManager::DestroyGameObject(GameObject* object)
{
    if (object == nullptr)
    {
        cxx_assert(false);
        return;
    }

    object->HandleDespawn();

    cxx::erase_elements(mAllObjects, object);

    switch (object->mClassID)
    {
        case eGameObjectClass_Pedestrian:
        {
            Pedestrian* pedestrian = static_cast<Pedestrian*>(object);
            mPedestriansPool.destroy(pedestrian);

            cxx::erase_elements(mPedestriansList, object);
        }
        break;

        case eGameObjectClass_Car:
        {
            Vehicle* vehicle = static_cast<Vehicle*>(object);
            mCarsPool.destroy(vehicle);

            cxx::erase_elements(mVehiclesList, object);
        }
        break;

        case eGameObjectClass_Projectile:
        {
            Projectile* projectile = static_cast<Projectile*>(object);
            mProjectilesPool.destroy(projectile);
        }
        break;

        case eGameObjectClass_Decoration:
        {
            Decoration* decoration = static_cast<Decoration*>(object);
            mDecorationsPool.destroy(decoration);
        }
        break;

        case eGameObjectClass_Obstacle:
        {
            Obstacle* obstacle = static_cast<Obstacle*>(object);
            mObstaclesPool.destroy(obstacle);
        }
        break;

        case eGameObjectClass_Explosion:
        {
            Explosion* explosion = static_cast<Explosion*>(object);
            mExplosionsPool.destroy(explosion);
        }
        break;

        case eGameObjectClass_Powerup:
        default:
        {
            cxx_assert(false);
        }
    }
}

void GameObjectsManager::DestroyAllObjects()
{
    for (GameObject* currObject: mAllObjects)
    {
        currObject->MarkForDeletion();
    }

    while (!mAllObjects.empty())
    {
        // destroy roots first
        GameObject* gameObject = get_element_if(mAllObjects, (GameObject*) nullptr, [](GameObject* currObject)
        {
            if (currObject->IsAttachedToObject())
                return false;

            return true;
        });
        cxx_assert(gameObject);
        DestroyGameObject(gameObject);
    }

    cxx_assert(mVehiclesList.empty());
    cxx_assert(mPedestriansList.empty());
}

void GameObjectsManager::DestroyMarkedForDeletionObjects()
{
    std::vector<GameObject*> toDeleteObjectsList;
    for (GameObject* currGameObject: mAllObjects)
    {
        if (currGameObject->IsMarkedForDeletion())
        {
            toDeleteObjectsList.push_back(currGameObject);
        }
    }

    for (GameObject* currGameObject: toDeleteObjectsList)
    {
        DestroyGameObject(currGameObject);
    }
}

GameObjectID GameObjectsManager::GenerateUniqueID()
{
    GameObjectID newID = ++mIDsCounter;
    if (newID == GAMEOBJECT_ID_NULL) // overflow
    {
        cxx_assert(false);
    }
    return newID;
}

bool GameObjectsManager::CreateStartupObjects()
{
    cxx_assert(gGameMap.IsLoaded());

    StyleData& styleData = gGameMap.mStyleData;
    for (const StartupObjectPosStruct& currObject: gGameMap.mStartupObjects)
    {
        int mapLevel = (int) Convert::PixelsToMapUnits(currObject.mZ);
        mapLevel = INVERT_MAP_LAYER(mapLevel);
        glm::vec3 start_position 
        { 
            Convert::PixelsToMeters(currObject.mX),
            Convert::MapUnitsToMeters(mapLevel * 1.0f),
            Convert::PixelsToMeters(currObject.mY) 
        };

        cxx::angle_t start_rotation = -Convert::Fix16ToAngle(currObject.mRotation);

        // create startup cars
        if (currObject.IsCarObject())
        {
            eVehicleModel carModel;
            if (!cxx::parse_enum_int(currObject.mType, carModel))
            {
                cxx_assert(false);
                continue;
            }
            if (Vehicle* startupCar = CreateVehicle(start_position, start_rotation, carModel))
            {
                startupCar->mObjectFlags = (startupCar->mObjectFlags | GameObjectFlags_Startup);
                continue;
            }
            cxx_assert(false);
            continue;
        }

        GameObjectInfo& objectType = styleData.mObjects[currObject.mType];
        switch (objectType.mClassID)
        {
            case eGameObjectClass_Decoration: 
            {
                Decoration* startupDecoration = CreateDecoration(start_position, start_rotation, &objectType);
                if (startupDecoration)
                {
                    startupDecoration->mObjectFlags = (startupDecoration->mObjectFlags | GameObjectFlags_Startup);
                }
                cxx_assert(startupDecoration);
            }
            break;

            case eGameObjectClass_Obstacle: 
            {
                Obstacle* startupObstacle = CreateObstacle(start_position, start_rotation, &objectType);
                if (startupObstacle)
                {
                    startupObstacle->mObjectFlags = (startupObstacle->mObjectFlags | GameObjectFlags_Startup);
                }
                cxx_assert(startupObstacle);
            }            
            break;

            default:
                cxx_assert(false);
            break;
        }
    }
    return true;
}