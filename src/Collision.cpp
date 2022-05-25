#include "stdafx.h"
#include "Collision.h"
#include "Box2D_Helpers.h"
#include "Collider.h"
#include "PhysicsBody.h"

bool Contact::GetRelativeVelocity(int contactPointIndex, glm::vec2& outputVelocity) const
{
    if ((mContactPointsCount == 0) || (contactPointIndex < 0) || (contactPointIndex >= mContactPointsCount))
    {
        cxx_assert(false);
        return false;
    }

    PhysicsBody* thisBody = mThisCollider->mPhysicsBody;
    PhysicsBody* thatBody = mThatCollider->mPhysicsBody;
    cxx_assert(thisBody);
    cxx_assert(thatBody);

    glm::vec2 relativeVelocity;
    if (thisBody && thisBody)
    {
        glm::vec2 velocityA = thisBody->GetLinearVelocityFromWorldPoint(mContactPoints[contactPointIndex].mPosition);
        glm::vec2 velocityB = thatBody->GetLinearVelocityFromWorldPoint(mContactPoints[contactPointIndex].mPosition);
        outputVelocity = velocityA - velocityB;
        return true;
    }
    return false;
}

bool Contact::GetRelativeVelocity(glm::vec2& outputVelocity) const
{
    if (mContactPointsCount > 0)
        return GetRelativeVelocity(0, outputVelocity);

    return false;
}

bool Contact::HasContactPoints() const
{
    return mContactPointsCount > 0;
}

void Contact::SetupWithBox2Data(b2Contact* box2Contact, b2Fixture* thisFixture, b2Fixture* thatFixture)
{
    cxx_assert(box2Contact);
    cxx_assert(thisFixture);
    cxx_assert(thatFixture);

    mThisObject = nullptr;
    mThatObject = nullptr;
    mThisCollider = b2Fixture_get_collider(thisFixture);
    mThatCollider = b2Fixture_get_collider(thatFixture);

    cxx_assert(mThisCollider);
    cxx_assert(mThatCollider);

    mThisObject = mThisCollider->mGameObject;
    mThatObject = mThatCollider->mGameObject;
    cxx_assert(mThisObject);
    cxx_assert(mThatObject);
    cxx_assert(mThisCollider->mGameObject != mThatCollider->mGameObject);

    mContactPointsCount = box2Contact->GetManifold()->pointCount;
    if (mContactPointsCount > 0)
    {
        mContactPointsCount = std::min(mContactPointsCount, MaxCollisionContactPoints);

        b2WorldManifold wmanifold;
        box2Contact->GetWorldManifold(&wmanifold);

        for (int icurrPoint = 0; icurrPoint < mContactPointsCount; ++ icurrPoint)
        {
            mContactPoints[icurrPoint].mPosition = convert_vec2(wmanifold.points[icurrPoint]);
            mContactPoints[icurrPoint].mNormal = convert_vec2(wmanifold.normal);
            mContactPoints[icurrPoint].mPositionY = mThisCollider->mPhysicsBody->mPositionY;
            mContactPoints[icurrPoint].mSeparation = wmanifold.separations[icurrPoint];
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void Collision::SetupWithBox2Data(b2Contact* box2Contact, b2Fixture* thisFixture, b2Fixture* thatFixture, const b2ContactImpulse* contactImpulse)
{
    mContactInfo.SetupWithBox2Data(box2Contact, thisFixture, thatFixture);

    cxx_assert(contactImpulse);

    mContactImpulse = 0.0f;
    for (int icurr = 0; icurr < contactImpulse->count; ++icurr)
    {
        mContactImpulse += contactImpulse->normalImpulses[icurr];
    }
}

//////////////////////////////////////////////////////////////////////////

bool MapCollision::HasContactPoints() const
{
    return mContactPointsCount > 0;
}

void MapCollision::SetupWithBox2Data(b2Contact* box2Contact, b2Fixture* objectFixture, const MapBlockInfo* mapBlockInfo, const b2ContactImpulse* contactImpulse)
{
    cxx_assert(box2Contact);
    cxx_assert(objectFixture);
    cxx_assert(mapBlockInfo);
    cxx_assert(contactImpulse);

    mMapBlockInfo = mapBlockInfo;
    mThisObject = nullptr;
    mThisCollider = b2Fixture_get_collider(objectFixture);

    cxx_assert(mThisCollider);

    mThisObject = mThisCollider->mGameObject;
    cxx_assert(mThisObject);

    mContactPointsCount = box2Contact->GetManifold()->pointCount;
    if (mContactPointsCount > 0)
    {
        mContactPointsCount = std::min(mContactPointsCount, MaxCollisionContactPoints);

        b2WorldManifold wmanifold;
        box2Contact->GetWorldManifold(&wmanifold);

        for (int icurrPoint = 0; icurrPoint < mContactPointsCount; ++ icurrPoint)
        {
            mContactPoints[icurrPoint].mPosition = convert_vec2(wmanifold.points[icurrPoint]);
            mContactPoints[icurrPoint].mNormal = convert_vec2(wmanifold.normal);
            mContactPoints[icurrPoint].mPositionY = mThisCollider->mPhysicsBody->mPositionY;
            mContactPoints[icurrPoint].mSeparation = wmanifold.separations[icurrPoint];
        }
    }

    mContactImpulse = 0.0f;
    for (int icurr = 0; icurr < contactImpulse->count; ++icurr)
    {
        mContactImpulse += contactImpulse->normalImpulses[icurr];
    }
}
