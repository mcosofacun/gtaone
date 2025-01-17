#include "stdafx.h"
#include "TrafficManager.h"
#include "GtaOneGame.h"
#include "GameCheatsWindow.h"
#include "AiCharacterController.h"

TrafficManager::TrafficManager()
{
}

void TrafficManager::StartupTraffic()
{   
    mLastGenHareKrishnasTime = gGame.mTimeMng.mGameTime;

    mLastGenPedsTime = 0.0f;
    GeneratePeds();

    mLastGenCarsTime = 0.0f;
    GenerateCars();

}

void TrafficManager::CleanupTraffic()
{
    for (Vehicle* currCar: gGame.mObjectsMng.mVehicles)
    {
        TryRemoveTrafficCar(currCar);
    }

    for (Pedestrian* currPedestrian: gGame.mObjectsMng.mPedestrians)
    {
        TryRemoveTrafficPed(currPedestrian);
    }
}

void TrafficManager::UpdateFrame()
{
    GeneratePeds();
    GenerateCars();
}

void TrafficManager::DebugDraw(DebugRenderer& debugRender)
{
}

void TrafficManager::GeneratePeds()
{
    if ((mLastGenPedsTime > 0.0f) && 
        (mLastGenPedsTime + gGame.mParams.mTrafficGenPedsCooldownTime) > gGame.mTimeMng.mGameTime)
    {
        return;
    }

    mLastGenPedsTime = gGame.mTimeMng.mGameTime;

    RemoveOffscreenPeds();

    if (!gGameCheatsWindow.mEnableTrafficPedsGeneration)
        return;

    int generatePedsCount = GetPedsToGenerateCount(gGame.mCamera);
    if (generatePedsCount > 0)
    {
        GenerateTrafficPeds(generatePedsCount, gGame.mCamera);
    }
}

void TrafficManager::RemoveOffscreenPeds()
{
    float offscreenDistance = Convert::MapUnitsToMeters(gGame.mParams.mTrafficGenPedsMaxDistance + 1.0f);

    cxx::aabbox2d_t onScreenArea = gGame.mCamera.mOnScreenMapArea;
    onScreenArea.mMax.x += offscreenDistance;
    onScreenArea.mMax.y += offscreenDistance;
    onScreenArea.mMin.x -= offscreenDistance;
    onScreenArea.mMin.y -= offscreenDistance;

    for (Pedestrian* pedestrian: gGame.mObjectsMng.mPedestrians)
    {
        if (pedestrian->IsMarkedForDeletion() || !pedestrian->IsTrafficFlag())
            continue;

        // skip vehicle passengers
        if (pedestrian->IsCarPassenger())
            continue;

        if (pedestrian->IsOnScreen(onScreenArea))
            continue;

        // remove ped
        pedestrian->MarkForDeletion();
    }
}

void TrafficManager::GenerateTrafficPeds(int pedsCount, GameCamera& view)
{
    cxx::randomizer& random = gGame.mRandom;

    int numPedsGenerated = 0;

    Rect innerRect;
    Rect outerRect;
    // get map area on screen
    {
        Point minBlock;
        minBlock.x = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMin.x);
        minBlock.y = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMin.y);

        Point maxBlock;
        maxBlock.x = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMax.x) + 1;
        maxBlock.y = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMax.y) + 1;

        innerRect.x = minBlock.x;
        innerRect.y = minBlock.y;
        innerRect.w = (maxBlock.x - minBlock.x);
        innerRect.h = (maxBlock.y - minBlock.y);

        // expand
        int expandSize = gGame.mParams.mTrafficGenPedsMaxDistance;
        outerRect = innerRect;
        outerRect.x -= expandSize;
        outerRect.y -= expandSize;
        outerRect.w += expandSize * 2;
        outerRect.h += expandSize * 2;
    }
    
    mCandidatePosArray.clear();

    for (int iy = 0; iy < outerRect.h; ++iy)
    for (int ix = 0; ix < outerRect.w; ++ix)
    {
        Point pos (ix + outerRect.x, iy + outerRect.y);
        if (innerRect.PointWithin(pos))
            continue;

        // scan candidate from top
        for (int iz = (MAP_LAYERS_COUNT - 1); iz > 0; --iz)
        {
            const MapBlockInfo* mapBlock = gGame.mMap.GetBlockInfo(pos.x, pos.y, iz);

            if (mapBlock->mGroundType == eGroundType_Air)
                continue;

            if (mapBlock->mGroundType == eGroundType_Pawement)
            {
                if (mapBlock->mIsRailway)
                    continue;

                CandidatePos candidatePos;
                candidatePos.mMapX = pos.x;
                candidatePos.mMapY = pos.y;
                candidatePos.mMapLayer = iz;
                mCandidatePosArray.push_back(candidatePos);
            }
            break;
        }
    }

    if (mCandidatePosArray.empty())
        return;

    // shuffle candidates
    random.shuffle(mCandidatePosArray);

    for (; (numPedsGenerated < pedsCount) && !mCandidatePosArray.empty(); ++numPedsGenerated)
    {
        if (!random.random_chance(gGame.mParams.mTrafficGenPedsChance))
            continue;

        CandidatePos candidate = mCandidatePosArray.back();
        mCandidatePosArray.pop_back();

        if ((mLastGenHareKrishnasTime + gGame.mParams.mTrafficGenHareKrishnasTime) < gGame.mTimeMng.mGameTime)
        {
            // generate hare krishnas
            GenerateHareKrishnas(candidate.mMapX, candidate.mMapLayer, candidate.mMapY);
            mLastGenHareKrishnasTime = gGame.mTimeMng.mGameTime;
        }
        else
        {
            // generate normal pedestrian
            GenerateRandomTrafficPedestrian(candidate.mMapX, candidate.mMapLayer, candidate.mMapY);
        }
    }
}

int TrafficManager::GetPedsToGenerateCount(GameCamera& view) const
{
    int pedestriansCounter = 0;

    float offscreenDistance = Convert::MapUnitsToMeters(gGame.mParams.mTrafficGenPedsMaxDistance * 1.0f);

    cxx::aabbox2d_t onScreenArea = view.mOnScreenMapArea;
    onScreenArea.mMax.x += offscreenDistance;
    onScreenArea.mMax.y += offscreenDistance;
    onScreenArea.mMin.x -= offscreenDistance;
    onScreenArea.mMin.y -= offscreenDistance;

    for (Pedestrian* pedestrian: gGame.mObjectsMng.mPedestrians)
    {
        if (!pedestrian->IsTrafficFlag() || pedestrian->IsMarkedForDeletion() || pedestrian->IsCarPassenger())
            continue;

        if (pedestrian->IsOnScreen(onScreenArea))
        {
            ++pedestriansCounter;
        }
    }

    return std::max(0, (gGame.mParams.mTrafficGenMaxPeds - pedestriansCounter));
}

int TrafficManager::CountTrafficPedestrians() const
{
    int counter = 0;
    for (Pedestrian* pedestrian: gGame.mObjectsMng.mPedestrians)
    {
        if (!pedestrian->IsTrafficFlag() || pedestrian->IsMarkedForDeletion() || pedestrian->IsCarPassenger())
            continue;

        ++counter;
    }
    return counter;
}

int TrafficManager::CountTrafficCars() const
{
    int counter = 0;
    for (Vehicle* currVehicle: gGame.mObjectsMng.mVehicles)
    {
        if (currVehicle->IsMarkedForDeletion() || !currVehicle->IsTrafficFlag())
            continue;

        ++counter;
    }
    return counter;
}

int TrafficManager::GetCarsToGenerateCount(GameCamera& view) const
{
    int carsCounter = 0;

    float offscreenDistance = Convert::MapUnitsToMeters(gGame.mParams.mTrafficGenCarsMaxDistance * 1.0f);

    cxx::aabbox2d_t onScreenArea = view.mOnScreenMapArea;
    onScreenArea.mMax.x += offscreenDistance;
    onScreenArea.mMax.y += offscreenDistance;
    onScreenArea.mMin.x -= offscreenDistance;
    onScreenArea.mMin.y -= offscreenDistance;

    for (Vehicle* car: gGame.mObjectsMng.mVehicles)
    {
        if (!car->IsTrafficFlag() || car->IsMarkedForDeletion())
            continue;

        if (car->IsOnScreen(onScreenArea))
        {
            ++carsCounter;
        }
    }

    return std::max(0, (gGame.mParams.mTrafficGenMaxCars - carsCounter));
}

void TrafficManager::GenerateCars()
{
    if ((mLastGenCarsTime > 0.0f) && 
        (mLastGenCarsTime + gGame.mParams.mTrafficGenCarsCooldownTime) > gGame.mTimeMng.mGameTime)
    {
        return;
    }

    mLastGenCarsTime = gGame.mTimeMng.mGameTime;

    RemoveOffscreenCars();

    if (!gGameCheatsWindow.mEnableTrafficCarsGeneration)
        return;

    int generateCarsCount = GetCarsToGenerateCount(gGame.mCamera);
    if (generateCarsCount > 0)
    {
        GenerateTrafficCars(generateCarsCount, gGame.mCamera);
    }
}

void TrafficManager::GenerateTrafficCars(int carsCount, GameCamera& view)
{
    cxx::randomizer& random = gGame.mRandom;

    int numCarsGenerated = 0;

    Rect innerRect;
    Rect outerRect;
    // get map area on screen
    {
        Point minBlock;
        minBlock.x = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMin.x);
        minBlock.y = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMin.y);

        Point maxBlock;
        maxBlock.x = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMax.x) + 1;
        maxBlock.y = (int) Convert::MetersToMapUnits(view.mOnScreenMapArea.mMax.y) + 1;

        innerRect.x = minBlock.x;
        innerRect.y = minBlock.y;
        innerRect.w = (maxBlock.x - minBlock.x);
        innerRect.h = (maxBlock.y - minBlock.y);

        // expand
        int expandSize = gGame.mParams.mTrafficGenCarsMaxDistance;
        outerRect = innerRect;
        outerRect.x -= expandSize;
        outerRect.y -= expandSize;
        outerRect.w += expandSize * 2;
        outerRect.h += expandSize * 2;
    }
    
    mCandidatePosArray.clear();

    for (int iy = 0; iy < outerRect.h; ++iy)
    for (int ix = 0; ix < outerRect.w; ++ix)
    {
        Point pos (ix + outerRect.x, iy + outerRect.y);
        if (innerRect.PointWithin(pos))
            continue;

        // scan candidate from top
        for (int iz = (MAP_LAYERS_COUNT - 1); iz > 0; --iz)
        {
            const MapBlockInfo* mapBlock = gGame.mMap.GetBlockInfo(pos.x, pos.y, iz);

            if (mapBlock->mGroundType == eGroundType_Air)
                continue;

            if (mapBlock->mGroundType == eGroundType_Road)
            {
                int bits = (int) (mapBlock->mDownDirection) + 
                    (int) (mapBlock->mUpDirection) +
                    (int) (mapBlock->mLeftDirection) + 
                    (int) (mapBlock->mRightDirection);

                if ((bits == 0 || bits > 1) || mapBlock->mIsRailway)
                    continue;

                CandidatePos candidatePos;
                candidatePos.mMapX = pos.x;
                candidatePos.mMapY = pos.y;
                candidatePos.mMapLayer = iz;
                mCandidatePosArray.push_back(candidatePos);
            }
            break;
        }
    }

    if (mCandidatePosArray.empty())
        return;

    // shuffle candidates
    random.shuffle(mCandidatePosArray);

    for (; (numCarsGenerated < carsCount) && !mCandidatePosArray.empty(); ++numCarsGenerated)
    {
        if (!random.random_chance(gGame.mParams.mTrafficGenCarsChance))
            continue;

        CandidatePos candidate = mCandidatePosArray.back();
        mCandidatePosArray.pop_back();

        GenerateRandomTrafficCar(candidate.mMapX, candidate.mMapLayer, candidate.mMapY);
    }
}

void TrafficManager::RemoveOffscreenCars()
{
    float offscreenDistance = Convert::MapUnitsToMeters(gGame.mParams.mTrafficGenCarsMaxDistance + 1.0f);

    cxx::aabbox2d_t onScreenArea = gGame.mCamera.mOnScreenMapArea;
    onScreenArea.mMax.x += offscreenDistance;
    onScreenArea.mMax.y += offscreenDistance;
    onScreenArea.mMin.x -= offscreenDistance;
    onScreenArea.mMin.y -= offscreenDistance;

    for (Vehicle* currentCar: gGame.mObjectsMng.mVehicles)
    {
        if (currentCar->IsMarkedForDeletion() || !currentCar->IsTrafficFlag())
            continue;

        if (currentCar->IsOnScreen(onScreenArea))
            continue;

        TryRemoveTrafficCar(currentCar);
    }
}

Vehicle* TrafficManager::GenerateRandomTrafficCar(int posx, int posy, int posz)
{
    const MapBlockInfo* mapBlock = gGame.mMap.GetBlockInfo(posx, posz, posy);
  
    glm::vec3 positions(
        Convert::MapUnitsToMeters(posx + 0.5f),
        Convert::MapUnitsToMeters(posy * 1.0f),
        Convert::MapUnitsToMeters(posz + 0.5f)
    );

    float turnAngle = 0.0f;
    if (mapBlock->mUpDirection)
    {
        turnAngle = -90.0f;
    }
    else if (mapBlock->mDownDirection)
    {
        turnAngle = 90.0f;
    }
    else if (mapBlock->mLeftDirection)
    {
        turnAngle = 180.0f;
    }

    // generate car
    cxx::angle_t carHeading(turnAngle, cxx::angle_t::units::degrees);
    positions.y = gGame.mMap.GetHeightAtPosition(positions);

    // choose car model
    std::vector<VehicleInfo*> models;
    for(VehicleInfo& currModel: gGame.mStyleData.mVehicles)
    {
        // filter classes
        if ((currModel.mClassID == eVehicleClass_Tank) || (currModel.mClassID == eVehicleClass_Boat) ||
            (currModel.mClassID == eVehicleClass_Tram) || (currModel.mClassID == eVehicleClass_Train) ||
            (currModel.mClassID == eVehicleClass_BackOfJuggernaut))
        {
            continue;
        }
        // filter models
        if ((currModel.mModelID == eVehicle_Helicopter) || (currModel.mModelID == eVehicle_Ambulance) ||
            (currModel.mModelID == eVehicle_FireTruck) || (currModel.mModelID == eVehicle_ModelCar))
        {
            continue;
        }
        models.push_back(&currModel);
    }
    
    if (models.empty())
        return nullptr;

    // shuffle candidates
    gGame.mRandom.shuffle(models);

    Vehicle* vehicle = gGame.mObjectsMng.CreateVehicle(positions, carHeading, models.front());
    cxx_assert(vehicle);
    if (vehicle)
    {
        vehicle->mObjectFlags = (vehicle->mObjectFlags | GameObjectFlags_Traffic);
        // todo: remap

        Pedestrian* carDriver = GenerateRandomTrafficCarDriver(vehicle);
        cxx_assert(carDriver);
    }

    return vehicle;
}

Pedestrian* TrafficManager::GenerateRandomTrafficPedestrian(int posx, int posy, int posz)
{
    cxx::randomizer& random = gGame.mRandom;

    // generate pedestrian
    glm::vec2 positionOffset(
        Convert::MapUnitsToMeters(random.generate_float() - 0.5f),
        Convert::MapUnitsToMeters(random.generate_float() - 0.5f)
    );
    cxx::angle_t pedestrianHeading(360.0f * random.generate_float(), cxx::angle_t::units::degrees);
    glm::vec3 pedestrianPosition(
        Convert::MapUnitsToMeters(posx + 0.5f) + positionOffset.x,
        Convert::MapUnitsToMeters(posy * 1.0f),
        Convert::MapUnitsToMeters(posz + 0.5f) + positionOffset.y
    );
    // fix height
    pedestrianPosition.y = gGame.mMap.GetHeightAtPosition(pedestrianPosition);

    Pedestrian* pedestrian = gGame.mObjectsMng.CreatePedestrian(pedestrianPosition, pedestrianHeading, ePedestrianType_Civilian);
    cxx_assert(pedestrian);
    if (pedestrian)
    {
        pedestrian->mObjectFlags = (pedestrian->mObjectFlags | GameObjectFlags_Traffic);

        AiCharacterController* controller = gGame.mAiMng.CreateAiController(pedestrian);
        cxx_assert(controller);
    }
    return pedestrian;
}

Pedestrian* TrafficManager::GenerateHareKrishnas(int posx, int posy, int posz)
{
    cxx::randomizer& random = gGame.mRandom;

    glm::vec2 positionOffset(
        Convert::MapUnitsToMeters(random.generate_float() - 0.5f),
        Convert::MapUnitsToMeters(random.generate_float() - 0.5f)
    );
    cxx::angle_t pedestrianHeading(360.0f * random.generate_float(), cxx::angle_t::units::degrees);
    glm::vec3 pedestrianPosition(
        Convert::MapUnitsToMeters(posx + 0.5f) + positionOffset.x,
        Convert::MapUnitsToMeters(posy * 1.0f),
        Convert::MapUnitsToMeters(posz + 0.5f) + positionOffset.y
    );
    // fix height
    pedestrianPosition.y = gGame.mMap.GetHeightAtPosition(pedestrianPosition);

    Pedestrian* characterLeader = nullptr;
    Pedestrian* characterPrev = nullptr;
    for (int i = 0, PedsCount = 7; i < PedsCount; ++i)
    {
        Pedestrian* character = gGame.mObjectsMng.CreatePedestrian(pedestrianPosition, pedestrianHeading, 
            (i == 0) ? ePedestrianType_GangLeader : ePedestrianType_Gang);
        cxx_assert(character);

        character->mObjectFlags = (character->mObjectFlags | GameObjectFlags_Traffic);
        AiCharacterController* controller = gGame.mAiMng.CreateAiController(character);
        cxx_assert(controller);
        if (controller && characterLeader == nullptr)
        {
            characterLeader = character;
        }
        if (controller && characterPrev)
        {
            controller->FollowPedestrian(characterPrev);
        }
        characterPrev = character;
    }
    return characterLeader;
}

Pedestrian* TrafficManager::GenerateRandomTrafficCarDriver(Vehicle* car)
{
    cxx::randomizer& random = gGame.mRandom;

    cxx_assert(car);

    int remapIndex = random.generate_int(0, MAX_PED_REMAPS - 1); // todo: find out correct list of traffic peds skins
    Pedestrian* pedestrian = gGame.mObjectsMng.CreatePedestrian(car->mTransform.mPosition, car->mTransform.mOrientation, ePedestrianType_Civilian);
    cxx_assert(pedestrian);

    if (pedestrian)
    {
        pedestrian->mObjectFlags = (pedestrian->mObjectFlags | GameObjectFlags_Traffic);

        AiCharacterController* controller = gGame.mAiMng.CreateAiController(pedestrian);
        cxx_assert(controller);
        if (controller)
        {
            pedestrian->PutInsideCar(car, eCarSeat_Driver);
        }
    }
    return pedestrian;
}

bool TrafficManager::TryRemoveTrafficCar(Vehicle* car)
{
    if (car->IsMarkedForDeletion())
        return true;

    if (!car->IsTrafficFlag())
        return false;

    bool hasNonTrafficPassengers = cxx::contains_if(car->mPassengers, [](Pedestrian* carDriver)
    {
        return !carDriver->IsTrafficFlag();
    });

    if (hasNonTrafficPassengers)
        return false;

    for (Pedestrian* currDriver: car->mPassengers)
    {
        currDriver->MarkForDeletion();
    }

    car->MarkForDeletion();
    return true;
}

bool TrafficManager::TryRemoveTrafficPed(Pedestrian* pedestrian)
{
    if (pedestrian->IsMarkedForDeletion())
        return true;

    if (!pedestrian->IsTrafficFlag())
        return false;
        
    pedestrian->MarkForDeletion();
    return true;
}