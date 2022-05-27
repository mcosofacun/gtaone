#pragma once

#include "Pedestrian.h"
#include "Vehicle.h"
#include "Projectile.h"
#include "Decoration.h"
#include "Obstacle.h"
#include "Explosion.h"

// define game objects manager class
class GameObjectsManager final: public cxx::noncopyable
{
public:
    // readonly
    std::vector<GameObject*> mAllObjects;
    std::vector<Pedestrian*> mPedestrians;
    std::vector<Vehicle*> mVehicles;

public:
    ~GameObjectsManager();

    void EnterWorld();
    void ClearWorld();
    void UpdateFrame();
    void DebugDraw(DebugRenderer& debugRender);

    // Add new pedestrian instance to map at specific location
    // @param position: Real world position
    // @param heading: Initial rotation
    // @param pedestrianType: Pedestrian type
    // @param remap: Remap
    Pedestrian* CreatePedestrian(const glm::vec3& position, cxx::angle_t heading, ePedestrianType pedestrianType, int remap = NO_REMAP);

    // Add new car instance to map at specific location
    // @param position: Initial world position
    // @param heading: Initial rotation
    // @param desc: Car style
    // @param carModel: Car model identifier
    Vehicle* CreateVehicle(const glm::vec3& position, cxx::angle_t heading, VehicleInfo* desc);
    Vehicle* CreateVehicle(const glm::vec3& position, cxx::angle_t heading, eVehicleModel carModel);

    // Add new projectile instance to map at specific location
    Projectile* CreateProjectile(const glm::vec3& position, cxx::angle_t heading, Pedestrian* shooter);
    Projectile* CreateProjectile(const glm::vec3& position, cxx::angle_t heading, WeaponInfo* weaponInfo, Pedestrian* shooter);

    // Add new decoration instance to map at specific location
    Decoration* CreateDecoration(const glm::vec3& position, cxx::angle_t heading, GameObjectInfo* desc);
    Decoration* CreateFirstBlood(const glm::vec3& position);
    Decoration* CreateWaterSplash(const glm::vec3& position);
    Decoration* CreateBigSmoke(const glm::vec3& position);

    // Add explosion instance to map at specific location 
    Explosion* CreateExplosion(GameObject* explodingObject, Pedestrian* causer, eExplosionType explosionType, const glm::vec3& position);

    // Add new obstacle instance to map at specific location
    Obstacle* CreateObstacle(const glm::vec3& position, cxx::angle_t heading, GameObjectInfo* desc);

    // Find gameobject by its unique identifier
    // @param objectID: Unique identifier
    Vehicle* GetVehicleByID(GameObjectID objectID) const;
    Obstacle* GetObstacleByID(GameObjectID objectID) const;
    Decoration* GetDecorationByID(GameObjectID objectID) const;
    Pedestrian* GetPedestrianByID(GameObjectID objectID) const;
    GameObject* GetGameObjectByID(GameObjectID objectID) const;

    // Will immediately destroy gameobject, don't call this mehod during UpdateFrame
    // @param object: Object to destroy
    void DestroyGameObject(GameObject* object);

private:
    bool CreateStartupObjects();
    void DestroyAllObjects();
    void DestroyMarkedForDeletionObjects();
    GameObjectID GenerateUniqueID();

private:
    GameObjectID mIDsCounter = 0;

    // objects pools
    cxx::object_pool<Pedestrian> mPedestriansPool;
    cxx::object_pool<Vehicle> mCarsPool;
    cxx::object_pool<Projectile> mProjectilesPool;
    cxx::object_pool<Decoration> mDecorationsPool;
    cxx::object_pool<Obstacle> mObstaclesPool;
    cxx::object_pool<Explosion> mExplosionsPool;
};