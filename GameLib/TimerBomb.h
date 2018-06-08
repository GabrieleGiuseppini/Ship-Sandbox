/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include <chrono>
#include <cstdint>

using namespace std::chrono_literals;

namespace Physics
{   

/*
 * Bomb specialization for bombs that explode after a time interval.
 */
class TimerBomb final : public Bomb
{
public:

    TimerBomb(
        ObjectId id,
        ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        BlastHandler blastHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual bool Update(
        GameWallClock::time_point now,
        GameParameters const & gameParameters) override;

    virtual void OnBombRemoved() override
    {
        // Stop fuse if it's burning
        if (State::SlowFuseBurning == mState
            || State::FastFuseBurning == mState)
        {
            mGameEventHandler->OnTimerBombFuseStop(mId);
        }

        // Notify removal
        mGameEventHandler->OnBombRemoved(
            mId,
            BombType::TimerBomb,
            mParentWorld.IsUnderwater(
                GetPosition()));

        // Detach ourselves, if we're s attached
        DetachIfAttached();
    }

    virtual void OnNeighborhoodDisturbed() override;

    virtual void Upload(
        int shipId,
        RenderContext & renderContext) const override;

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In this state the fuse burns slowly, and after a while the bomb moves
        // to detonation lead-in
        SlowFuseBurning,

        // In this state the fuse burns fast, and then the bomb moves to exploding
        FastFuseBurning,

        // In this state we are about to explode; we wait a little time and then
        // move to exploding
        DetonationLeadIn,

        // In this state we are exploding, and increment our counter to
        // match the explosion animation until the animation is over
        Exploding,

        // We enter this state once the bomb gets underwater; we play a short
        // smoke animation and then we expire
        Defused,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    static constexpr auto SlowFuseToDetonationLeadInInterval = 7000ms;
    static constexpr auto DetonationLeadInToExplosionInterval = 1500ms;
    static constexpr int FuseFramesPerLeveCount = 4;

    ////inline void TransitionToDetonationLeadIn(GameWallClock::time_point now)
    ////{
    ////    mState = State::DetonationLeadIn;

    ////    ++mPingOnStepCounter;

    ////    mGameEventHandler->OnRCBombPing(
    ////        mParentWorld.IsUnderwater(GetPosition()),
    ////        1);

    ////    // Schedule next transition
    ////    mNextStateTransitionTimePoint = now + FastPingInterval;
    ////}

    State mState;

    // The next timestamp at which we'll automatically transition state
    GameWallClock::time_point mNextStateTransitionTimePoint;

    // The fuse frame frame index, which is calculated during state transitions
    uint32_t mFuseFlameFrameIndex;
};

}
