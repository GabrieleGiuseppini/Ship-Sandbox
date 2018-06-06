﻿/***************************************************************************************
* Original Author:      Luke Wren (wren6991@gmail.com)
* Created:              2013-04-30
* Modified By:          Gabriele Giuseppini
* Copyright:            Luke Wren (http://github.com/Wren6991),
*                       Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"
#include "ShipBuilder.h"

#include <algorithm>
#include <cassert>

namespace Physics {

// W     W    OOO    RRRR     L        DDDD
// W     W   O   O   R   RR   L        D  DDD
// W     W  O     O  R    RR  L        D    DD
// W     W  O     O  R   RR   L        D     D
// W     W  O     O  RRRR     L        D     D
// W  W  W  O     O  R RR     L        D     D
// W  W  W  O     O  R   R    L        D    DD
//  W W W    O   O   R    R   L        D  DDD
//   W W      OOO    R     R  LLLLLLL  DDDD

World::World(
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    GameParameters const & gameParameters)
    : mAllShips()
    , mAllClouds()
    , mWaterSurface()
    , mOceanFloor()
    , mCurrentTime(0.0f)
    , mCurrentStepSequenceNumber(1u)
    , mGameEventHandler(std::move(gameEventHandler))
{
    // Initialize clouds
    UpdateClouds(gameParameters);

    // Initialize water and ocean
    mWaterSurface.Update(mCurrentTime, gameParameters);
    mOceanFloor.Update(gameParameters);
}

int World::AddShip(
    ShipDefinition const & shipDefinition,
    MaterialDatabase const & materials,
    GameParameters const & gameParameters)
{
    int shipId = static_cast<int>(mAllShips.size());

    auto newShip = ShipBuilder::Create(
        shipId,
        *this,
        mGameEventHandler,
        shipDefinition,
        materials,
        gameParameters,
        mCurrentStepSequenceNumber);

    mAllShips.push_back(std::move(newShip));

    return shipId;
}

void World::DestroyAt(
    vec2 const & targetPos, 
    float radius)
{
    for (auto & ship : mAllShips)
    {
        ship->DestroyAt(
            targetPos,
            radius);
    }
}

void World::DrawTo(
    vec2f const & targetPos,
    float strength)
{
    for (auto & ship : mAllShips)
    {
        ship->DrawTo(
            targetPos,
            strength);
    }
}

void World::TogglePinAt(
    vec2 const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully pins or unpins a point
    for (auto const & ship : mAllShips)
    {
        if (ship->TogglePinAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleTimerBombAt(
    vec2 const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto const & ship : mAllShips)
    {
        if (ship->ToggleTimerBombAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleRCBombAt(
    vec2 const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto const & ship : mAllShips)
    {
        if (ship->ToggleRCBombAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::DetonateRCBombs()
{
    for (auto const & ship : mAllShips)
    {
        ship->DetonateRCBombs();
    }
}

ElementIndex World::GetNearestPointAt(
    vec2 const & targetPos,
    float radius) const
{
    ElementIndex bestPointIndex = NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto const & ship : mAllShips)
    {
        auto shipBestPointIndex = ship->GetNearestPointIndexAt(targetPos, radius);
        if (NoneElementIndex != shipBestPointIndex)
        {
            float squareDistance = (ship->GetPoints().GetPosition(shipBestPointIndex) - targetPos).squareLength();
            if (squareDistance < bestSquareDistance)
            {
                bestPointIndex = shipBestPointIndex;
                bestSquareDistance = squareDistance;
            }
        }
    }

    return bestPointIndex;
}

void World::Update(GameParameters const & gameParameters)
{
    // Update current time
    mCurrentTime += GameParameters::SimulationStepTimeDuration<float>;

    // Generate a new step sequence number
    ++mCurrentStepSequenceNumber;
    if (0u == mCurrentStepSequenceNumber)
        mCurrentStepSequenceNumber = 1u;

    // Update water surface
    mWaterSurface.Update(mCurrentTime, gameParameters);

    // Update all ships
    for (auto & ship : mAllShips)
    {
        ship->Update(
            mCurrentStepSequenceNumber,
            gameParameters);
    }

    //buildBVHTree(true, points, collisionTree);

    // Update clouds
    UpdateClouds(gameParameters);
}

void World::Render( 
    GameParameters const & gameParameters,
    RenderContext & renderContext) const
{
    renderContext.RenderStart();

    // Upload land and water data
    UploadLandAndWater(gameParameters, renderContext);

    // Render the clouds
    RenderClouds(renderContext);

    // Render the ocean floor
    renderContext.RenderLand();

    // Render the water now, if we want to see the ship through the water
    if (renderContext.GetShowShipThroughWater())
    {
        renderContext.RenderWater();
    }

    // Render all ships
    for (auto const & ship : mAllShips)
    {
        ship->Render(
            gameParameters,
            renderContext);
    }

    // Render the water now, if we want to see the ship *in* the water instead
    if (!renderContext.GetShowShipThroughWater())
    {
        renderContext.RenderWater();
    }

    renderContext.RenderEnd();
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////

void World::UpdateClouds(GameParameters const & gameParameters)
{
    // Resize clouds vector
    if (gameParameters.NumberOfClouds < mAllClouds.size())
    {
        mAllClouds.resize(gameParameters.NumberOfClouds);
    }
    else
    {
        for (size_t c = mAllClouds.size(); c < gameParameters.NumberOfClouds; ++c)
        {
            mAllClouds.emplace_back(
                new Cloud(
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 100.0f,    // OffsetX
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.01f,      // SpeedX1
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.04f,     // AmpX
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.01f,     // SpeedX2
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 100.0f,    // OffsetY
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.001f,    // AmpY
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.005f,     // SpeedY
                    0.2f + static_cast<float>(c) / static_cast<float>(c + 3), // OffsetScale - the earlier clouds are smaller
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.05f,     // AmpScale
                    GameRandomEngine::GetInstance().GenerateRandomNormalReal() * 0.005f));    // SpeedScale
        }
    }

    // Update clouds
    for (auto & cloud : mAllClouds)
    {
        cloud->Update(
            mCurrentTime,
            gameParameters.WindSpeed);
    }
}

void World::RenderClouds(RenderContext & renderContext) const
{
    renderContext.RenderCloudsStart(mAllClouds.size());

    for (auto const & cloud : mAllClouds)
    {
        renderContext.RenderCloud(
            cloud->GetX(),
            cloud->GetY(),
            cloud->GetScale());
    }

    renderContext.RenderCloudsEnd();
}

void World::UploadLandAndWater(
    GameParameters const & gameParameters,
    RenderContext & renderContext) const
{
    static constexpr size_t SlicesCount = 500;

    float const visibleWorldWidth = renderContext.GetVisibleWorldWidth();
    float const sliceWidth = visibleWorldWidth / static_cast<float>(SlicesCount);
    float sliceX = renderContext.GetCameraWorldPosition().x - (visibleWorldWidth / 2.0f);

    renderContext.UploadLandAndWaterStart(SlicesCount);    

    for (size_t i = 0; i <= SlicesCount; ++i, sliceX += sliceWidth)
    {
        renderContext.UploadLandAndWater(
            sliceX,
            mOceanFloor.GetFloorHeightAt(sliceX),
            mWaterSurface.GetWaterHeightAt(sliceX),
            gameParameters.SeaDepth);
    }

    renderContext.UploadLandAndWaterEnd();
}

}