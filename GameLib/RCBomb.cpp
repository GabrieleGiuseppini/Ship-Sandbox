/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

using namespace std::chrono_literals;

namespace Physics {

static constexpr auto SlowPingOffInterval = 750ms;
static constexpr auto SlowPingOnInterval = 250ms;
static constexpr auto FastPingInterval = 125ms;
static constexpr auto DetonationLeadInToDetonationInterval = 2000ms;
static constexpr auto ExplosionProgressInterval = 20ms;

static constexpr uint8_t ExplosionPhaseCount = 8;

static constexpr int PingFramesCount = 4;

RCBomb::RCBomb(
    ElementContainer::ElementIndex pointIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    BlastHandler blastHandler,
    Points & shipPoints)
    : Bomb(
        BombType::RCBomb,
        pointIndex,
        parentWorld,
        std::move(gameEventHandler),
        blastHandler,
        shipPoints)
    , mCurrentState(State::Idle)
    , mCurrentFrameIndex(0)
    , mCurrentPingPhase(0) 
    , mNextPingProgressTimePoint(GameWallClock::GetInstance().Now() + SlowPingOffInterval)
    , mDetonationTimePoint(GameWallClock::time_point::min())
    , mCurrentShakeOffset(0.0f, 0.0f)
    , mCurrentShakeOffsetIndex(static_cast<uint8_t>(GameRandomEngine::GetInstance().Choose(4)))
    , mCurrentExplosionPhase(0)
    , mNextExplosionProgressTimePoint(GameWallClock::time_point::min())
{
}

bool RCBomb::Update(
    GameWallClock::time_point now,
    GameParameters const & gameParameters)
{
    switch (mCurrentState)
    {
        case State::Idle:
        {
            if (now > mNextPingProgressTimePoint)
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
                    mCurrentFrameIndex = 1 + ((mCurrentPingPhase % (2 * PingFramesCount)) / 2);

                    // Calculate next progress time
                    mNextPingProgressTimePoint = now + SlowPingOnInterval;
                }
                else
                {
                    //
                    // Ping off
                    //

                    // Set frame index
                    mCurrentFrameIndex = 0;

                    // Calculate next progress time
                    mNextPingProgressTimePoint = now + SlowPingOffInterval;
                }
            }

            return true;
        }

        case State::DetonationLeadIn:
        {
            // Check if time to explode
            if (now > mDetonationTimePoint)
            {
                //
                // Initiate explosion
                //

                // Reset shaking
                mCurrentShakeOffset = vec2f(0.0f, 0.0f);

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachFromPointIfAttached();

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);

                //
                // Initiate explosion state machine
                //

                // Transition state
                mCurrentState = State::Exploding;

                // Initialize explosion phase
                mCurrentExplosionPhase = 0;

                // Fall over to next case
            }
            else 
            {
                if (now > mNextPingProgressTimePoint)
                {
                    // Notify
                    mGameEventHandler->OnRCBombPing(
                        mParentWorld.IsUnderwater(GetPosition()),
                        1);

                    // Set frame index
                    mCurrentFrameIndex = 1 + (mCurrentPingPhase % PingFramesCount);

                    // Get to new ping phase
                    ++mCurrentPingPhase;

                    // Schedule next ping progress
                    mNextPingProgressTimePoint = now + FastPingInterval;
                }

                //
                // Update shaking
                //

                static const vec2f ShakeOffsets[] = {
                    { -0.25f, -0.25f },
                    { 0.25f, -0.25f },
                    { -0.25f, 0.25f },
                    { 0.25f, 0.25f }
                };

                // TODOTEST
                //mCurrentShakeOffset = ShakeOffsets[(mCurrentShakeOffsetIndex++) % 4];

                return true;
            }
            
            // Falling over to next case on purpose
        }

        case State::Exploding:
        {
            if (now > mNextExplosionProgressTimePoint)
            {
                // Invoke blast handler
                mBlastHandler(
                    GetPosition(),
                    GetConnectedComponentId(),
                    mCurrentExplosionPhase,
                    ExplosionPhaseCount,
                    gameParameters);

                // Set frame index
                mCurrentFrameIndex = 1 + PingFramesCount + mCurrentExplosionPhase;

                // Increment explosion phase
                ++mCurrentExplosionPhase;

                // Check whether we're done
                if (mCurrentExplosionPhase > ExplosionPhaseCount)
                {
                    // Transition to expired
                    mCurrentState = State::Expired;
                }
                else
                {
                    // Schedule next exposion progress
                    mNextExplosionProgressTimePoint = now + ExplosionProgressInterval;
                }
            }

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

float RCBomb::GetRenderScale() const 
{
    return 1.0f + static_cast<float>(mCurrentExplosionPhase + 1) / static_cast<float>(ExplosionPhaseCount);
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

        // Schedule next ping progress for right now
        mNextPingProgressTimePoint = now;

        // Set timeout for detonation
        mDetonationTimePoint = now + DetonationLeadInToDetonationInterval;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

}