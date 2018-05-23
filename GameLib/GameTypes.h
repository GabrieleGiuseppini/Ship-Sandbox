/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>

using ConnectedComponentId = uint32_t; // Connected component IDs start from 1

enum class ShipRenderMode
{
    Points,
    Springs,
    Structure,
    Texture
};

enum class BombType
{
    TimerBomb,
    RCBomb
};