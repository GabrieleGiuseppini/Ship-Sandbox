/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "GameTypes.h"
#include "GameWallClock.h"
#include "IGameEventHandler.h"
#include "Physics.h"
#include "RenderContext.h"
#include "Vectors.h"

#include <cassert>
#include <functional>
#include <memory>
#include <optional>

namespace Physics
{   

/*
 * Base class of all bombs. Each bomb type has a specialization that takes care
 * of its own state machine.
 */
class Bomb
{
public:

    using BlastHandler = std::function<void(
        vec2f const & blastPosition,
        ConnectedComponentId connectedComponentId, 
        int blastSequenceNumber,
        int blastSequenceCount,
        GameParameters const & gameParameters)>;

public:

    /*
     * Updates the bomb's state machine.
     *
     * Returns false when the bomb has "expired" and thus can be deleted.
     */
    virtual bool Update(
        GameWallClock::time_point now,
        GameParameters const & gameParameters) = 0;

    /*
     * Invoked when the neighborhood of the point has been disturbed;
     * includes the point that the bomb is attached to.
     */
    virtual void OnNeighborhoodDisturbed() = 0;

    /*
     * Uploads rendering information to the render context.
     */
    virtual void Upload(
        int shipId,
        RenderContext & renderContext) const = 0;

    /*
     * If the point is attached, saves its current position and detaches itself from the Points container;
     * otherwise, it's a nop.
     */
    void DetachFromPointIfAttached()
    {
        if (!!mPointIndex)
        {
            assert(mPoints.IsBombAttached(*mPointIndex));

            mShipPoints.DetachBomb(*mPointIndex);
            mPosition = mShipPoints.GetPosition(*mPointIndex);
            mConnectedComponentId = mShipPoints.GetConnectedComponentId(*mPointIndex);
            mPointIndex.reset();
        }
        else
        {
            assert(!!mPosition);
            assert(!!mConnectedComponentId);
        }
    }

    /*
     * Returns the type of this bomb.
     */
    BombType GetType() const
    {
        return mType;
    }

    /*
     * Gets the point that the bomb is attached to, or none if the bomb is not
     * attached to any points/
     */
    std::optional<ElementContainer::ElementIndex> GetAttachedPointIndex() const
    {
        return mPointIndex;
    }

    /*
     * Returns the position of this bomb.
     */
    vec2f const GetPosition() const
    {
        if (!!mPosition)
            return *mPosition;
        else
        {
            assert(!!mPointIndex);
            return mShipPoints.GetPosition(*mPointIndex);
        }
    }

    /*
     * Returns the ID of the connected component of this bomb.
     */
    ConnectedComponentId GetConnectedComponentId() const
    {
        if (!!mConnectedComponentId)
            return *mConnectedComponentId;
        else
        {
            assert(!!mPointIndex);
            return mShipPoints.GetConnectedComponentId(*mPointIndex);
        }
    }

protected:

    Bomb(
        BombType type,
        ElementContainer::ElementIndex pointIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        BlastHandler blastHandler,
        Points & shipPoints)
        : mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mBlastHandler(blastHandler)
        , mShipPoints(shipPoints)
        , mType(type)
        , mPointIndex(pointIndex)
        , mPosition(std::nullopt)
        , mConnectedComponentId(std::nullopt)
    {
    }

    // Our parent world
    World & mParentWorld;

    // The game event handler
    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    // The handler to invoke for each explosion
    BlastHandler mBlastHandler;

    // The container of all the ship's points
    Points & mShipPoints;

private:

    // The type of this bomb
    BombType const mType;

    // The index of the point that we're attached to, or none
    // when the bomb has been detached
    std::optional<ElementContainer::ElementIndex> mPointIndex;

    // The position of this bomb, if the bomb has been detached from its point; otherwise, none
    std::optional<vec2f> mPosition;

    // The connected component ID of this bomb, if the bomb has been detached from its point; otherwise, none
    std::optional<ConnectedComponentId> mConnectedComponentId;
};

}
