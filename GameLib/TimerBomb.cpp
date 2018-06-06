/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

TimerBomb::TimerBomb(
    ObjectId id,
    ElementIndex springIndex,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    BlastHandler blastHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Bomb(
        id,
        BombType::TimerBomb,
        springIndex,
        parentWorld,
        std::move(gameEventHandler),
        blastHandler,
        shipPoints,
        shipSprings)
    , mState(State::FuseBurning)
{}

bool TimerBomb::Update(
    GameWallClock::time_point now,
    GameParameters const & gameParameters)
{
    // TODO
    return false;
}

void TimerBomb::OnNeighborhoodDisturbed()
{
    // TODO
}

void TimerBomb::Upload(
    int shipId,
    RenderContext & renderContext) const
{
    // TODO
}

}