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
        ElementContainer::ElementIndex pointIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        BlastHandler blastHandler,
        Points & points)
        : Bomb(
            BombType::TimerBomb,
            pointIndex,
            parentWorld,
            std::move(gameEventHandler),
            blastHandler,
            points)
    {}

    virtual bool Update() override;

    virtual float GetRenderScale() const override
    {
        // TODO
        return 1.0f;
    }

    virtual uint32_t GetRenderFrameIndex() const override
    {
        // TODO
        return 0;
    }


private:
};

}
