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
        ElementContainer::ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        BlastHandler blastHandler,
        Points & shipPoints,
        Springs & shipSprings)
        : Bomb(
            BombType::TimerBomb,
            springIndex,
            parentWorld,
            std::move(gameEventHandler),
            blastHandler,
            shipPoints,
            shipSprings)
    {}

    virtual bool Update(
        GameWallClock::time_point now,
        GameParameters const & gameParameters) override;

    virtual void OnNeighborhoodDisturbed() override;

    virtual void Upload(
        int shipId,
        RenderContext & renderContext) const override;

private:
};

}
