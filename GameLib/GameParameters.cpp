/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

GameParameters::GameParameters()
    : Gravity(0.0f, -9.80f)
    , GravityNormal(Gravity.normalise())
    , GravityMagnitude(Gravity.length())
    , StrengthAdjustment(0.0048f)
    , BuoyancyAdjustment(4.0f)
    , WaterPressureAdjustment(0.75f)
    , WaveHeight(2.5f)
    , SeaDepth(150.0f)
    , DestroyRadius(0.55f)
    , LightDiffusionAdjustment(0.5f)
    , NumberOfClouds(50)
    , WindSpeed(3.0f)
{
}
