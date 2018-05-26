/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

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
        ElementContainer::ElementIndex pointIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        BlastHandler blastHandler,
        Points & points);

    virtual bool Update(GameWallClock::time_point now) override;

    virtual float GetRenderScale() const override
    {
        // TODO
        return 1.0f;
    }

    virtual uint32_t GetRenderFrameIndex() const override
    {
        return mCurrentFrameIndex;
    }

    void Detonate();

private:

    ///////////////////////////////////////////////////////
    // State machine
    ///////////////////////////////////////////////////////

    enum class State
    {
        // In this state we wait for remote detonation or disturbance,
        // and ping regularly at long intervals
        Idle,

        // In this state we are about to explode; we wait a little time
        // before exploding, and ping and shake regularly at short intervals
        DetonationLeadIn,

        // In this state we are exploding, and change our frame index to
        // match the explosion animation until the animation is over
        Exploding,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    State mCurrentState;

    // The current frame index
    uint32_t mCurrentFrameIndex;

    // The current ping phase:
    // - Even: ping off
    // - Odd: ping on; mod 8 and then division by two yields frame index
    // Fine to rollover!
    std::uint8_t mCurrentPingPhase;

    // The next timestamp at which we'll ping
    GameWallClock::time_point mNextPingTimePoint;

    // The timestamp at which we'll detonate while in detonation lead-in
    GameWallClock::time_point mDetonationTimePoint;

    // The current explosion phase
    std::uint8_t mCurrentExplosionPhase;
};

}
