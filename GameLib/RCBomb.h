/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include <cstdint>

using namespace std::chrono_literals;

namespace Physics
{   

/*
 * Bomb specialization for bombs that explode when a remote control is triggered.
 */
class RCBomb final : public Bomb
{
public:

    RCBomb(
        ElementContainer::ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        BlastHandler blastHandler,
        Points & shipPoints,
        Springs & shipSprings);

    virtual bool Update(
        GameWallClock::time_point now,
        GameParameters const & gameParameters) override;

    virtual void OnNeighborhoodDisturbed() override
    {
        Detonate();
    }

    virtual void Upload(
        int shipId,
        RenderContext & renderContext) const override;

    void Detonate();

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In these states we wait for remote detonation or disturbance,
        // and ping regularly at long intervals
        IdlePingOff,
        IdlePingOn,

        // In this state we are about to explode; we wait a little time
        // before exploding, and ping regularly at short intervals
        DetonationLeadIn,

        // In this state we are exploding, and increment our counter to
        // match the explosion animation until the animation is over
        Exploding,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    static constexpr auto SlowPingOffInterval = 750ms;
    static constexpr auto SlowPingOnInterval = 250ms;
    static constexpr auto FastPingInterval = 100ms;
    static constexpr auto DetonationLeadInToExplosionInterval = 1500ms;
    static constexpr auto ExplosionProgressInterval = 20ms;
    static constexpr uint8_t ExplosionStepsCount = 8;
    static constexpr int PingFramesCount = 4;

    inline void TransitionToDetonationLeadIn(GameWallClock::time_point now)
    {
        mState = State::DetonationLeadIn;

        ++mDetonationLeadInStepCounter;

        mGameEventHandler->OnRCBombPing(
            mParentWorld.IsUnderwater(GetPosition()),
            1);

        // Schedule next transition
        mNextStateTransitionTimePoint = now + FastPingInterval;
    }

    inline void TransitionToExploding(
        GameWallClock::time_point now,
        GameParameters const & gameParameters)
    {
        mState = State::Exploding;

        ++mExplodingStepCounter;

        // Invoke blast handler
        mBlastHandler(
            GetPosition(),
            GetConnectedComponentId(),
            mExplodingStepCounter - 1,
            ExplosionStepsCount,
            gameParameters);

        // Schedule next transition
        mNextStateTransitionTimePoint = now + ExplosionProgressInterval;
    }

    State mState;

    // The next timestamp at which we'll automatically transition state
    GameWallClock::time_point mNextStateTransitionTimePoint;

    // The timestamp at which we'll explode while in detonation lead-in
    GameWallClock::time_point mExplosionTimePoint;

    // The counters for the various states; set to one upon
    // entering the state for the first time. Fine to rollover!
    uint8_t mIdlePingOnStepCounter;
    uint8_t mDetonationLeadInStepCounter;
    uint8_t mExplodingStepCounter;
};

}
