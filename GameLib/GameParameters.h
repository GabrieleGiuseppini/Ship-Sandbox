/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

/*
* Parameters that affect the game (physics, world).
*/
struct GameParameters
{
    //
    // The dt of each step
    //

    template <typename T>
    static constexpr T SimulationStepTimeDuration = 0.02f;


    //
    // The number of iterations we run in the dynamics step for
    // each simulation step
    //
    // The number of iterations dictates how stiff bodies are:
    // - Less iterations => softer (jelly) body
    // - More iterations => hard body (never breaks though) 
    //

    template <typename T>
    static constexpr T NumDynamicIterations = 12;


    //
    // The dt of each iteration in the dynamics step
    //

    template <typename T>
    static constexpr T DynamicsSimulationStepTimeDuration = SimulationStepTimeDuration<T> / NumDynamicIterations<T>;


    //
    // Tunable parameters
    //

	vec2 const Gravity;
	vec2 const GravityNormal;
	float const GravityMagnitude;

	float StrengthAdjustment;
	static constexpr float MinStrengthAdjustment = 0.0010f;
	static constexpr float MaxStrengthAdjustment = 0.5f;

	float BuoyancyAdjustment;
	static constexpr float MinBuoyancyAdjustment = 0.0f;
	static constexpr float MaxBuoyancyAdjustment = 10.0f;

	float WaterPressureAdjustment;
	static constexpr float MinWaterPressureAdjustment = 0.0f;
	static constexpr float MaxWaterPressureAdjustment = 4.0f;

	float WaveHeight;
	static constexpr float MinWaveHeight = 0.0f;
	static constexpr float MaxWaveHeight = 30.0f;

	float SeaDepth;
	static constexpr float MinSeaDepth = 20.0f;
	static constexpr float MaxSeaDepth = 300.0f;

	float DestroyRadius;
	static constexpr float MinDestroyRadius = 0.1f;
	static constexpr float MaxDestroyRadius = 10.0f;

    float LightDiffusionAdjustment;

    size_t NumberOfClouds;

    float WindSpeed;

    GameParameters();
};
