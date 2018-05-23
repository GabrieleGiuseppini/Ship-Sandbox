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
        Points & points)
        : Bomb(
            BombType::RCBomb,
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
