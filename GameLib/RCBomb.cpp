/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

static constexpr auto SlowPingOffInterval = 750ms;
static constexpr auto SlowPingOnInterval = 250ms;

static constexpr auto FastPingInterval = 125ms;

static constexpr auto DetonationLeadInToDetonationInterval = 2000ms;

RCBomb::RCBomb(
    ElementContainer::ElementIndex pointIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    BlastHandler blastHandler,
    Points & points)
    : Bomb(
        BombType::RCBomb,
        pointIndex,
        parentWorld,
        std::move(gameEventHandler),
        blastHandler,
        points)
    , mCurrentState(State::Idle)
    , mCurrentFrameIndex(0)
    , mCurrentPingPhase(0) 
    , mNextPingTimePoint(GameWallClock::GetInstance().Now() + SlowPingOffInterval)
    , mDetonationTimePoint(GameWallClock::time_point::min())
    , mCurrentExplosionPhase(0)
{
}


bool RCBomb::Update(GameWallClock::time_point now)
{
    switch (mCurrentState)
    {
        case State::Idle:
        {
            if (now > mNextPingTimePoint)
            {
                // Get to new ping phase
                ++mCurrentPingPhase;

                if (0 != (mCurrentPingPhase % 2))
                {
                    //
                    // Ping on
                    //

                    // Notify
                    mGameEventHandler->OnRCBombPing(
                        mParentWorld.IsUnderwater(GetPosition()),
                        1);

                    // Set frame index
                    mCurrentFrameIndex = 1 + ((mCurrentPingPhase % 8) / 2);

                    // Calculate next ping
                    mNextPingTimePoint = now + SlowPingOnInterval;
                }
                else
                {
                    //
                    // Ping off
                    //

                    // Set frame index
                    mCurrentFrameIndex = 0;

                    // Calculate next ping
                    mNextPingTimePoint = now + SlowPingOffInterval;
                }
            }

            return true;
        }

        case State::DetonationLeadIn:
        {
            // Check if time to explode
            if (now > mDetonationTimePoint)
            {
                // Time to explode

                // Transition state
                mCurrentState = State::Exploding;

                // Initialize explosion phase
                mCurrentExplosionPhase = 0;

                // Keep going, fall over to next switch
                // where we will perform the actual explosion
            }
            else if (now > mNextPingTimePoint)
            {
                // Get to new ping phase
                ++mCurrentPingPhase;

                // Notify
                mGameEventHandler->OnRCBombPing(
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                // Set frame index
                mCurrentFrameIndex = 1 + (mCurrentPingPhase % 4);

                // Calculate next ping
                mNextPingTimePoint = now + FastPingInterval;


                //
                // Update shaking
                //

                // TODO

                return true;
            }
            else
            {
                // Nothing to do
                return true;
            }

            // Fall over to next case on purpose
        }

        case State::Exploding:
        {
            // TODO: revisit, based off explosion frames
            if (0 == mCurrentExplosionPhase)
            {
                //
                // Trigger explosion
                //

                // Detach self first
                // TODOHERE
                //DetachFromPointIfAttached(gameParameters);

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                // Invoke blast handler
                mBlastHandler(
                    GetPosition(),
                    GetConnectedComponentId(),
                    1.0f);
            }

            // Increment explosion phase
            ++mCurrentExplosionPhase;

            // Schedule next phase transition
            // TODO

            return true;
        }

        case State::Expired:
        {
            return false;
        }
    }

    assert(false);
    return true;
}

void RCBomb::Detonate()
{
    if (State::Idle == mCurrentState)
    {
        // Transition state
        mCurrentState = State::DetonationLeadIn;

        // Reset ping phase
        mCurrentPingPhase = 0;
        
        // Reset frame index
        mCurrentFrameIndex = 0;

        auto now = GameWallClock::GetInstance().Now();

        // Schedule next ping for right now
        mNextPingTimePoint = now;

        // Set timeout for detonation
        mDetonationTimePoint = now + DetonationLeadInToDetonationInterval;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

}