/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IGameEventHandler.h"
#include "TupleKeys.h"

#include <algorithm>
#include <optional>
#include <vector>

class GameEventDispatcher : public IGameEventHandler
{
public:

    GameEventDispatcher()
        : mDestroyEvents()
        , mDrawEvent(false)
        , mSwirlEvent(false)
        , mPinToggledEvents()
        , mStressEvents()
        , mBreakEvents()
        , mSinkingBeginEvents()
        , mBombExplosionEvents()
        , mRCBombPingEvents()
        , mTimerBombDefusedEvents()
        , mSinks()
    {
    }

public:

    virtual void OnGameReset() override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnGameReset();
        }
    }

    virtual void OnShipLoaded(
        unsigned int id,
        std::string const & name) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnShipLoaded(id, name);
        }
    }

    virtual void OnDestroy(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override
    {
        mDestroyEvents[std::make_tuple(material, isUnderwater)] += size;
    }

    virtual void OnDraw() override
    {
        mDrawEvent = true;
    }

    virtual void OnSwirl() override
    {
        mSwirlEvent = true;
    }

    virtual void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override
    {
        mPinToggledEvents.insert(std::make_tuple(isPinned, isUnderwater));
    }

    virtual void OnStress(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override
    {
        mStressEvents[std::make_tuple(material, isUnderwater)] += size;
    }

    virtual void OnBreak(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override
    {
        mBreakEvents[std::make_tuple(material, isUnderwater)] += size;
    }

    virtual void OnSinkingBegin(unsigned int shipId) override
    {
        if (mSinkingBeginEvents.end() == std::find(mSinkingBeginEvents.begin(), mSinkingBeginEvents.end(), shipId))
        {
            mSinkingBeginEvents.push_back(shipId);
        }
    }

    //
    // Bombs
    //

    virtual void OnBombPlaced(
        ObjectId bombId,
        BombType bombType,
        bool isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnBombPlaced(
                bombId,
                bombType,
                isUnderwater);
        }
    }

    virtual void OnBombRemoved(
        ObjectId bombId,
        BombType bombType,
        std::optional<bool> isUnderwater) override
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnBombRemoved(
                bombId,
                bombType,
                isUnderwater);
        }
    }

    virtual void OnBombExplosion(
        bool isUnderwater,
        unsigned int size) override
    {
        mBombExplosionEvents[std::make_tuple(isUnderwater)] += size;
    }

    virtual void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override
    {
        mRCBombPingEvents[std::make_tuple(isUnderwater)] += size;
    }

    virtual void OnTimerBombSlowFuseStart(
        ObjectId bombId,
        bool isUnderwater)
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnTimerBombSlowFuseStart(
                bombId,
                isUnderwater);
        }
    }

    virtual void OnTimerBombFastFuseStart(
        ObjectId bombId,
        bool isUnderwater)
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnTimerBombFastFuseStart(
                bombId,
                isUnderwater);
        }
    }

    virtual void OnTimerBombFuseStop(
        ObjectId bombId)
    {
        // No need to aggregate this one
        for (auto sink : mSinks)
        {
            sink->OnTimerBombFuseStop(
                bombId);
        }
    }

    virtual void OnTimerBombDefused(
        bool isUnderwater,
        unsigned int size) override
    {
        mTimerBombDefusedEvents[std::make_tuple(isUnderwater)] += size;
    }

public:

    /*
     * Flushes all events aggregated so far and clears the state.
     */
    void Flush()
    {
        // Publish aggregations
        for (IGameEventHandler * sink : mSinks)
        {
            for (auto const & entry : mDestroyEvents)
            {
                sink->OnDestroy(std::get<0>(entry.first), std::get<1>(entry.first), entry.second);
            }

            if (mDrawEvent)
            {
                sink->OnDraw();
            }

            if (mSwirlEvent)
            {
                sink->OnSwirl();
            }

            for (auto const & entry : mPinToggledEvents)
            {
                sink->OnPinToggled(std::get<0>(entry), std::get<1>(entry));
            }

            for (auto const & entry : mStressEvents)
            {
                sink->OnStress(std::get<0>(entry.first), std::get<1>(entry.first), entry.second);
            }

            for (auto const & entry : mBreakEvents)
            {
                sink->OnBreak(std::get<0>(entry.first), std::get<1>(entry.first), entry.second);
            }

            for (auto const & shipId : mSinkingBeginEvents)
            {
                sink->OnSinkingBegin(shipId);
            }

            for (auto const & entry : mBombExplosionEvents)
            {
                sink->OnBombExplosion(std::get<0>(entry.first), entry.second);
            }

            for (auto const & entry : mRCBombPingEvents)
            {
                sink->OnRCBombPing(std::get<0>(entry.first), entry.second);
            }

            for (auto const & entry : mTimerBombDefusedEvents)
            {
                sink->OnTimerBombDefused(std::get<0>(entry.first), entry.second);
            }
        }

        // Clear collections
        mDestroyEvents.clear();
        mDrawEvent = false;
        mSwirlEvent = false;
        mPinToggledEvents.clear();
        mStressEvents.clear();
        mBreakEvents.clear();
        mSinkingBeginEvents.clear();
        mBombExplosionEvents.clear();
        mRCBombPingEvents.clear();
        mTimerBombDefusedEvents.clear();
    }

    void RegisterSink(IGameEventHandler * sink)
    {
        mSinks.push_back(sink);
    }

private:

    // The current events being aggregated
    unordered_tuple_map<std::tuple<Material const *, bool>, unsigned int> mDestroyEvents;
    bool mDrawEvent;
    bool mSwirlEvent;
    unordered_tuple_set<std::tuple<bool, bool>> mPinToggledEvents;
    unordered_tuple_map<std::tuple<Material const *, bool>, unsigned int> mStressEvents;
    unordered_tuple_map<std::tuple<Material const *, bool>, unsigned int> mBreakEvents;
    std::vector<unsigned int> mSinkingBeginEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mBombExplosionEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mRCBombPingEvents;
    unordered_tuple_map<std::tuple<bool>, unsigned int> mTimerBombDefusedEvents;

    // The registered sinks
    std::vector<IGameEventHandler *> mSinks;
};
