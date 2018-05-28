/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include <cstdint>

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

    virtual bool Update(
        GameWallClock::time_point now,
        GameParameters const & gameParameters) override;

    virtual void OnNeighborhoodDisturbed() override
    {
        Detonate();
    }

    virtual float GetRenderScale() const override
    {
        // TODOTEST
        //return 1.0f;
        return 1.0f + static_cast<float>(mCurrentExplosionPhase + 1) / 8.0f;
    }

    virtual uint32_t GetRenderFrameIndex() const override
    {
        return mCurrentFrameIndex;
    }

    virtual vec2f const GetPosition() const override
    {
        return Bomb::GetPosition() + mCurrentShakeOffset;
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

    // The current texture frame to use when rendering
    uint32_t mCurrentFrameIndex;

    // The current ping phase;
    // - In Idle state:
    //      - Even: ping off
    //      - Odd: ping on; mod 8 and then division by two yields frame index
    // - In DetonationLeadIn: simply the index of the ping on frame
    //
    // Fine to rollover!
    uint8_t mCurrentPingPhase;

    // The next timestamp at which we'll perform a ping state step
    GameWallClock::time_point mNextPingProgressTimePoint;

    // The timestamp at which we'll detonate while in detonation lead-in
    GameWallClock::time_point mDetonationTimePoint;

    // The current position offset use for shaking during detonation lead-in
    vec2f mCurrentShakeOffset;
    uint8_t mCurrentShakeOffsetIndex;

    // The current explosion phase
    uint8_t mCurrentExplosionPhase;

    // The next timestamp at which we'll progress the explosion
    GameWallClock::time_point mNextExplosionProgressTimePoint;
};

}
