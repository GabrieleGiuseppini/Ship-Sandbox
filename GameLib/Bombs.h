/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "CircularList.h"
#include "GameParameters.h"
#include "IGameEventHandler.h"
#include "Physics.h"
#include "RenderContext.h"
#include "Vectors.h"

#include <functional>
#include <memory>

namespace Physics
{	

/*
 * This class manages a set of bombs.
 *
 * All game events are taken care of by this class.
 *
 * The explosion handler can be used to modify the world due to the explosion.
 */
class Bombs final
{
public:

    Bombs(
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        Bomb::BlastHandler blastHandler,
        Points & points)
        : mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mBlastHandler(blastHandler)
        , mPoints(points)
    {
    }

    void Update(GameParameters const & gameParameters);

    bool ToggleTimerBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleBombAt<TimerBomb>(
            targetPos,
            gameParameters);
    }

    bool ToggleRCBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters)
    {
        return ToggleBombAt<RCBomb>(
            targetPos,
            gameParameters);
    }

    void DetonateRCBombs();

    void Upload(
        int shipId,
        RenderContext & renderContext) const;

private:

    template <typename TBomb>
    bool ToggleBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters)
    {
        float const squareSearchRadius = gameParameters.ToolPointSearchRadius * gameParameters.ToolPointSearchRadius;

        //
        // See first if there's a bomb within the search radius, most recent first;
        // if so we remove it and we're done
        //

        for (auto it = mCurrentBombs.begin(); it != mCurrentBombs.end(); ++it)
        {
            float squareDistance = ((*it)->GetPosition() - targetPos).squareLength();
            if (squareDistance < squareSearchRadius)
            {
                // Found a bomb

                // Notify its removal
                mGameEventHandler->OnBombRemoved(
                    (*it)->GetType(),
                    mParentWorld.IsUnderwater(
                        (*it)->GetPosition()));

                // Detach it from its point, if it's attached
                (*it)->DetachFromPointIfAttached(gameParameters);

                // Remove from set of bombs - forget about it
                mCurrentBombs.erase(it);

                // We're done
                return true;
            }
        }


        //
        // No bombs points in radius...
        // ...so find closest point with no attached bomb within the search radius, and
        // if found, attach bomb to it it
        //

        ElementContainer::ElementIndex nearestUnarmedPointIndex = ElementContainer::NoneElementIndex;
        float nearestUnarmedPointDistance = std::numeric_limits<float>::max();

        for (auto pointIndex : mPoints)
        {
            if (!mPoints.IsDeleted(pointIndex) && !mPoints.IsBombAttached(pointIndex))
            {
                float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
                if (squareDistance < squareSearchRadius)
                {
                    // This point is within the search radius

                    // Keep the nearest
                    if (squareDistance < squareSearchRadius && squareDistance < nearestUnarmedPointDistance)
                    {
                        nearestUnarmedPointIndex = pointIndex;
                        nearestUnarmedPointDistance = squareDistance;
                    }
                }
            }
        }

        if (ElementContainer::NoneElementIndex != nearestUnarmedPointIndex)
        {
            // We have a nearest, unarmed point

            // Create bomb
            std::unique_ptr<Bomb> bomb(
                new TBomb(
                    nearestUnarmedPointIndex,
                    mParentWorld,
                    mGameEventHandler,
                    mBlastHandler,
                    mPoints));

            // Attach bomb to the point
            mPoints.AttachBomb(
                nearestUnarmedPointIndex,
                gameParameters);

            // Notify
            mGameEventHandler->OnBombPlaced(
                bomb->GetType(),
                mParentWorld.IsUnderwater(
                    bomb->GetPosition()));

            // Add new bomb to set of bombs, removing eventual bombs that might get purged 
            mCurrentBombs.emplace(
                [this, &gameParameters](std::unique_ptr<Bomb> const & purgedBomb)
                {
                    // Notify its removal
                    mGameEventHandler->OnBombRemoved(
                        purgedBomb->GetType(),
                        mParentWorld.IsUnderwater(
                            purgedBomb->GetPosition()));

                    // Detach bomb from its point, if it's attached
                    purgedBomb->DetachFromPointIfAttached(gameParameters);
                },
                std::move(bomb));

            // We're done
            return true;
        }

        // No point found on this ship
        return false;
    }

private:

    // Our parent world
    World & mParentWorld;

    // The game event handler
    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    // The handler to invoke for each explosion
    Bomb::BlastHandler const mBlastHandler;
    
    // The container of all the ship's points
    Points & mPoints;

    // The current set of bombs
    CircularList<std::unique_ptr<Bomb>, GameParameters::MaxBombs> mCurrentBombs;
};

}
