/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <random>
/*
 * The random engine for the entire game.
 *
 * Not so random - always uses the same seed. On purpose! We want two instances
 * of the game to be identical to each other.
 *
 * Singleton.
 */
class GameRandomEngine
{
public:

public:

    static GameRandomEngine & GetInstance()
    {
        static GameRandomEngine * instance = new GameRandomEngine();
        
        return *instance;
    }

    inline size_t Choose(size_t count)
    {
        return GenerateRandomInteger<size_t>(0, count - 1);
    }

    inline size_t ChooseNew(
        size_t count,
        size_t last)
    {
        // Choose randomly, but avoid choosing the last-chosen again
        size_t chosen = GenerateRandomInteger<size_t>(0, count - 2);
        if (chosen >= last)
        {
            ++chosen;
        }

        return chosen;
    }

    template <typename T>
    inline T GenerateRandomInteger(
        T minValue,
        T maxValue)
    {
        std::uniform_int_distribution<T> dis(minValue, maxValue);
        return dis(mRandomEngine);
    }

    inline float GenerateRandomNormalReal()
    {        
        return mRandomNormalDistribution(mRandomEngine);
    }

private:

    GameRandomEngine()
    {
        std::seed_seq seed_seq({ 1, 242, 19730528 });
        mRandomEngine = std::ranlux48_base(seed_seq);
        mRandomNormalDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
    }

    std::ranlux48_base mRandomEngine;
    std::uniform_real_distribution<float> mRandomNormalDistribution;
};
