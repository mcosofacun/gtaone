#include "stdafx.h"
#include "BroadcastEvent.h"
#include "GtaOneGame.h"

void BroadcastEventsIterator::Reset()
{
    mCurrentIndex = -1;
}

bool BroadcastEventsIterator::NextEvent(eBroadcastEvent eventType, BroadcastEvent& outputEventData)
{
    int currIndex = 0;
    if (mCurrentIndex != -1)
    {
        currIndex = mCurrentIndex;
    }
    const int eventsCount = (int) gGame.mBroadcastEventsList.size();
    for (;currIndex < eventsCount; ++currIndex)
    {
        const BroadcastEvent& currEvent = gGame.mBroadcastEventsList[currIndex];
        if (currEvent.mEventType == eventType)
        {
            mCurrentIndex = currIndex;
            outputEventData = currEvent;
            return true;
        }
    }
    mCurrentIndex = eventsCount;
    return false;
}

bool BroadcastEventsIterator::NextEventInDistance(eBroadcastEvent eventType, const glm::vec2& position, float maxDistance, BroadcastEvent& outputEventData)
{
    int currIndex = 0;
    if (mCurrentIndex != -1)
    {
        currIndex = mCurrentIndex;
    }
    const float maxDistance2 = (maxDistance * maxDistance);
    const int eventsCount = (int) gGame.mBroadcastEventsList.size();
    for (;currIndex < eventsCount; ++currIndex)
    {
        const BroadcastEvent& currEvent = gGame.mBroadcastEventsList[currIndex];
        if (currEvent.mEventType == eventType)
        {
            float currDistance2 = glm::distance2(position, currEvent.mPosition);
            if (currDistance2 < maxDistance2)
            {
                mCurrentIndex = currIndex;
                outputEventData = currEvent;
                return true;
            }
        }
    }
    mCurrentIndex = eventsCount;
    return false;
}

void BroadcastEventsIterator::DeleteCurrentEvent()
{
    if ((mCurrentIndex == -1) || (mCurrentIndex >= (int)gGame.mBroadcastEventsList.size()))
    {
        cxx_assert(false);
        return;
    }
    gGame.mBroadcastEventsList[mCurrentIndex].mStatus = BroadcastEvent::Status_CanDelete;
}
