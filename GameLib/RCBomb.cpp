/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

namespace Physics {

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
    , mState(State::IdlePingOff)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + SlowPingOffInterval)
    , mExplosionTimePoint(GameWallClock::time_point::min())
    , mIdlePingOnStepCounter(0u)
    , mDetonationLeadInStepCounter(0u)
    , mExplodingStepCounter(0u)
{
}

bool RCBomb::Update(
    GameWallClock::time_point now,
    GameParameters const & gameParameters)
{
    switch (mState)
    {
        case State::IdlePingOff:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to PingOn state
                //

                mState = State::IdlePingOn;

                ++mIdlePingOnStepCounter;

                mGameEventHandler->OnRCBombPing(
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);
               
                // Schedule next transition
                mNextStateTransitionTimePoint = now + SlowPingOnInterval;
            }

            return true;
        }

        case State::IdlePingOn:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to PingOff state
                //

                mState = State::IdlePingOff;

                // Schedule next transition
                mNextStateTransitionTimePoint = now + SlowPingOffInterval;
            }

            return true;
        }

        case State::DetonationLeadIn:
        {
            // Check if time to explode
            if (now > mExplosionTimePoint)
            {
                //
                // Transition to Exploding state
                //

                TransitionToExploding(now, gameParameters);

                // Detach self (or else explosion will move along with ship performing
                // its blast)
                DetachFromPointIfAttached();

                // Notify explosion
                mGameEventHandler->OnBombExplosion(
                    mParentWorld.IsUnderwater(GetPosition()),
                    1);
            }
            else if (now > mNextStateTransitionTimePoint)
            {
                //
                // Transition to DetonationLeadIn state
                //

                TransitionToDetonationLeadIn(now);
            }

            return true;
        }

        case State::Exploding:
        {
            if (now > mNextStateTransitionTimePoint)
            {
                // Check whether we're done
                if (mExplodingStepCounter > ExplosionStepsCount)
                {
                    // Transition to expired
                    mState = State::Expired;
                }
                else
                {
                    //
                    // Transition to Exploding state
                    //

                    TransitionToExploding(now, gameParameters);
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

void RCBomb::Upload(
    int shipId,
    RenderContext & renderContext) const
{
    switch (mState)
    {
        case State::IdlePingOff:
        {
            renderContext.UploadShipElementBomb(
                shipId,
                BombType::RCBomb,
                RotatedTextureRenderInfo(
                    GetPosition(),
                    1.0f,
                    {1.0f, 1.0f}, // TODO: take from spring
                    { 1.0f, 1.0f }), // TODO: take from spring
                0,
                std::nullopt,
                GetConnectedComponentId());

            break;
        }

        case State::IdlePingOn:
        {
            renderContext.UploadShipElementBomb(
                shipId,
                BombType::RCBomb,
                RotatedTextureRenderInfo(
                    GetPosition(),
                    1.0f,
                    { 1.0f, 1.0f }, // TODO: take from spring
                    { 1.0f, 1.0f }), // TODO: take from spring
                0,
                1 + ((mIdlePingOnStepCounter - 1) % PingFramesCount),
                GetConnectedComponentId());

            break;
        }

        case State::DetonationLeadIn:
        {
            renderContext.UploadShipElementBomb(
                shipId,
                BombType::RCBomb,
                RotatedTextureRenderInfo(
                    GetPosition(),
                    1.0f,
                    { 1.0f, 1.0f }, // TODO: take from spring
                    { 1.0f, 1.0f }), // TODO: take from spring
                0,
                1 + ((mDetonationLeadInStepCounter - 1) % PingFramesCount),
                GetConnectedComponentId());

            break;
        }

        case State::Exploding:
        {
            assert(mExplodingStepCounter >= 1);
            assert(mExplodingStepCounter <= ExplosionStepsCount);
            
            renderContext.UploadShipElementBomb(
                shipId,
                BombType::RCBomb,
                RotatedTextureRenderInfo(                    
                    GetPosition(),
                    1.0f + static_cast<float>(mExplodingStepCounter) / static_cast<float>(ExplosionStepsCount),
                    { 1.0f, 1.0f }, // TODO: take from spring
                    { 1.0f, 1.0f }), // TODO: take from spring
                std::nullopt,
                1 + PingFramesCount + (mExplodingStepCounter - 1),
                GetConnectedComponentId());

            break;
        }

        case State::Expired:
        {
            // No drawing
            break;
        }
    }
}

void RCBomb::Detonate()
{
    if (State::IdlePingOff == mState
        || State::IdlePingOn == mState)
    {
        //
        // Transition to DetonationLeadIn state
        //

        auto now = GameWallClock::GetInstance().Now();

        TransitionToDetonationLeadIn(now);

        // Schedule explosion
        mExplosionTimePoint = now + DetonationLeadInToExplosionInterval;
    }
}


}