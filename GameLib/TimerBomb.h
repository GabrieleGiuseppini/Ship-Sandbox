/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

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
        // In this state the fuse burns slowly, and after a while the bomb explodes
        FuseBurning,

        // In this state we are about to explode; we wait a little time
        // before exploding, and burn the fuse faster
        DetonationLeadIn,

        // In this state we are exploding, and increment our counter to
        // match the explosion animation until the animation is over
        Exploding,

        // This is the final state; once this state is reached, we're expired
        Expired
    };

    State mState;
};

}
