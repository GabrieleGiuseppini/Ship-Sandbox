/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "Material.h"

#include <optional>

/*
 * This interface defines the methods that game event handlers must implement.
 *
 * The methods are default-implemented to facilitate implementation of handlers that 
 * only care about a subset of the events.
 */
class IGameEventHandler
{
public:

    virtual ~IGameEventHandler()
    {
    }

    virtual void OnGameReset()
    {
        // Default-implemented
    }

    virtual void OnShipLoaded(
        unsigned int /*id*/,
        std::string const & /*name*/)
    {
        // Default-implemented
    }

    virtual void OnDestroy(
        Material const * /*material*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnSawToggled(bool /*isSawing*/)
    {
        // Default-implemented
    }

    virtual void OnDraw()
    {
        // Default-implemented
    }

    virtual void OnSwirl()
    {
        // Default-implemented
    }

    virtual void OnPinToggled(
        bool /*isPinned*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnStress(
        Material const * /*material*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnBreak(
        Material const * /*material*/,
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnSinkingBegin(unsigned int /*shipId*/)
    {
        // Default-implemented
    }

    //
    // Bombs
    //

    virtual void OnBombPlaced(
        ObjectId /*bombId*/,
        BombType /*bombType*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnBombRemoved(
        ObjectId /*bombId*/,
        BombType /*bombType*/,
        std::optional<bool> /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnBombExplosion(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnRCBombPing(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }

    virtual void OnTimerBombSlowFuseStart(
        ObjectId /*bombId*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnTimerBombFastFuseStart(
        ObjectId /*bombId*/,
        bool /*isUnderwater*/)
    {
        // Default-implemented
    }

    virtual void OnTimerBombFuseStop(
        ObjectId /*bombId*/)
    {
        // Default-implemented
    }

    virtual void OnTimerBombDefused(
        bool /*isUnderwater*/,
        unsigned int /*size*/)
    {
        // Default-implemented
    }
};
