#include "stdafx.h"
#include "Decoration.h"
#include "GtaOneGame.h"

Decoration::Decoration(GameObjectID id, GameObjectInfo* gameObjectDesc) 
    : GameObject(eGameObjectClass_Decoration, id)
    , mMoveVelocity()
    , mAnimationState()
{
    if (gameObjectDesc)
    {
        mAnimationState.mAnimDesc = gameObjectDesc->mAnimationData;
        mDrawSprite.mDrawOrder = gameObjectDesc->mDrawOrder;
    }
}

void Decoration::UpdateFrame()
{
    float deltaTime = gGame.mTimeMng.mGameFrameDelta;
    if (mAnimationState.UpdateFrame(deltaTime))
    {
        SetSprite(mAnimationState.GetSpriteIndex(), 0);
    }

    glm::vec3 newPosition = mTransform.mPosition + (mMoveVelocity * deltaTime);
    if (newPosition != mTransform.mPosition)
    {
        SetTransform(newPosition, mTransform.mOrientation);
    }

    if (mLifeDuration > 0 && !mAnimationState.IsActive())
    {
        MarkForDeletion();
    }
}

void Decoration::DebugDraw(DebugRenderer& debugRender)
{
}

void Decoration::HandleSpawn()
{
    mRemapClut = 0;

    mAnimationState.ClearState();
    mAnimationState.PlayAnimation(eSpriteAnimLoop_FromStart);

    SetSprite(mAnimationState.GetSpriteIndex(), 0);
}

void Decoration::SetLifeDuration(int numCycles)
{
    mLifeDuration = numCycles;
    mAnimationState.SetMaxRepeatCycles(numCycles);
}

void Decoration::SetDrawOrder(eSpriteDrawOrder drawOrder)
{
    mDrawSprite.mDrawOrder = drawOrder;
}

void Decoration::SetScale(float scale)
{
    mDrawSprite.mScale = scale;
    RefreshDrawSprite();
}

void Decoration::SetMoveVelocity(const glm::vec3& moveVelocity)
{
    mMoveVelocity = moveVelocity;
}
