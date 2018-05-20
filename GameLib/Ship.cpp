﻿/***************************************************************************************
* Original Author:      Luke Wren (wren6991@gmail.com)
* Created:              2013-04-30
* Modified By:          Gabriele Giuseppini
* Copyright:            Luke Wren (http://github.com/Wren6991),
*                       Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "Log.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <queue>
#include <set>

namespace Physics {

//   SSS    H     H  IIIIIII  PPPP
// SS   SS  H     H     I     P   PP
// S        H     H     I     P    PP
// SS       H     H     I     P   PP
//   SSS    HHHHHHH     I     PPPP
//      SS  H     H     I     P
//       S  H     H     I     P
// SS   SS  H     H     I     P
//   SSS    H     H  IIIIIII  P


Ship::Ship(
    int id,
    World * parentWorld,
    Points && points,
    Springs && springs,
    Triangles && triangles,
    ElectricalElements && electricalElements,
    uint64_t currentStepSequenceNumber)
    : mId(id)
    , mParentWorld(parentWorld)    
    , mPoints(std::move(points))
    , mSprings(std::move(springs))
    , mTriangles(std::move(triangles))
    , mElectricalElements(std::move(electricalElements))
    , mConnectedComponentSizes()
    , mAreElementsDirty(true)
    , mIsSinking(false)
    , mTotalWater(0.0)
    , mCurrentPinnedPoints()
    , mArePinnedPointsDirty(false)
    , mCurrentDrawForce(std::nullopt)
{
    DetectConnectedComponents(currentStepSequenceNumber);
}

Ship::~Ship()
{
}

void Ship::DestroyAt(
    vec2 const & targetPos, 
    float radius)
{
    float const squareRadius = radius * radius;

    // Destroy all points within the radius
    for (auto pointIndex : mPoints)
    {
        if (!mPoints.IsDeleted(pointIndex))
        {
            if ((mPoints.GetPosition(pointIndex) - targetPos).squareLength() < squareRadius)
            {
                //
                // This point must be deleted
                //

                //
                // If the point is pinned, unpin it
                //

                auto it = std::find(mCurrentPinnedPoints.begin(), mCurrentPinnedPoints.end(), pointIndex);
                if (it != mCurrentPinnedPoints.end())
                {
                    assert(!mPoints.IsDeleted(*it));

                    // Unpin it
                    assert(mPoints.IsPinned(*it));
                    mPoints.Unpin(*it);

                    // Remove from stack
                    mCurrentPinnedPoints.erase(it);

                    // Remember we have to re-upload the pinned points
                    mArePinnedPointsDirty = true;
                }

                // Destroy point
                mPoints.Destroy(
                    pointIndex,
                    mSprings,
                    mTriangles,
                    mElectricalElements);

                // Notify
                mParentWorld->GetGameEventHandler()->OnDestroy(
                    mPoints.GetMaterial(pointIndex),
                    mParentWorld->IsUnderwater(mPoints.GetPosition(pointIndex)),
                    1u);

                // Remember the elements are now dirty
                mAreElementsDirty = true;
            }
        }
    }
}

void Ship::DrawTo(
    vec2f const & targetPos,
    float strength)
{
    // Store the force
    assert(!mCurrentDrawForce);
    mCurrentDrawForce.emplace(targetPos, strength);
}

bool Ship::TogglePinAt(
    vec2f const & targetPos,
    float searchRadius)
{
    float const squareSearchRadius = searchRadius * searchRadius;

    //
    // See first if there's a pinned point within the search radius, most recent first;
    // if so we unpin it and we're done
    //

    for (auto it = mCurrentPinnedPoints.rbegin(); it != mCurrentPinnedPoints.rend(); ++it)
    {
        assert(!mPoints.IsDeleted(*it));
        assert(mPoints.IsPinned(*it));

        float squareDistance = (mPoints.GetPosition(*it) - targetPos).squareLength();
        if (squareDistance < squareSearchRadius)
        {
            // Found a pinned point

            // Unpin it
            mPoints.Unpin(*it);

            // Remove from set of pinned points
            mCurrentPinnedPoints.erase(std::next(it).base());

            // Remember we have to re-upload the pinned points
            mArePinnedPointsDirty = true;

            // Notify
            mParentWorld->GetGameEventHandler()->OnPinToggled(
                false,
                mParentWorld->IsUnderwater(mPoints.GetPosition(*it)));

            // We're done
            return true;
        }
    }


    //
    // No pinned points in radius...
    // ...so find closest unpinned point within the search radius, and
    // if found, pin it
    //

    ElementContainer::ElementIndex nearestUnpinnedPointIndex = ElementContainer::NoneElementIndex;
    float nearestUnpinnedPointDistance = std::numeric_limits<float>::max();

    for (auto pointIndex : mPoints)
    {
        if (!mPoints.IsDeleted(pointIndex) && !mPoints.IsPinned(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < squareSearchRadius)
            {
                // This point is within the search radius

                // Keep the nearest
                if (squareDistance < squareSearchRadius && squareDistance < nearestUnpinnedPointDistance)
                {
                    nearestUnpinnedPointIndex = pointIndex;
                    nearestUnpinnedPointDistance = squareDistance;
                }
            }
        }
    }

    if (ElementContainer::NoneElementIndex != nearestUnpinnedPointIndex)
    {
        // We have a nearest, unpinned point

        // Pin it
        mPoints.Pin(nearestUnpinnedPointIndex);

        // Add to set of pinned points
        mCurrentPinnedPoints.push_back(nearestUnpinnedPointIndex);

        // Remember we have to re-upload the pinned points
        mArePinnedPointsDirty = true;

        // Notify
        mParentWorld->GetGameEventHandler()->OnPinToggled(
            true,
            mParentWorld->IsUnderwater(mPoints.GetPosition(nearestUnpinnedPointIndex)));

        // We're done
        return true;
    }

    // No point found on this ship
    return false;
}

ElementContainer::ElementIndex Ship::GetNearestPointIndexAt(
    vec2 const & targetPos, 
    float radius) const
{
    float const squareRadius = radius * radius;

    ElementContainer::ElementIndex bestPointIndex = ElementContainer::NoneElementIndex;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto pointIndex : mPoints)
    {
        if (!mPoints.IsDeleted(pointIndex))
        {
            float squareDistance = (mPoints.GetPosition(pointIndex) - targetPos).squareLength();
            if (squareDistance < squareRadius && squareDistance < bestSquareDistance)
            {
                bestPointIndex = pointIndex;
                bestSquareDistance = squareDistance;
            }
        }
    }

    return bestPointIndex;
}

void Ship::Update(
    uint64_t currentStepSequenceNumber,
    GameParameters const & gameParameters)
{
    IGameEventHandler * const gameEventHandler = mParentWorld->GetGameEventHandler();

    //
    // Update dynamics
    //

    UpdateDynamics(gameParameters);


    //
    // Update strain for all springs; might cause springs to break
    // (which would flag elements as dirty)
    //

    if (mSprings.UpdateStrains(
        gameParameters,
        *mParentWorld,
        *gameEventHandler,                
        mPoints,
        mTriangles))
    {
        // At least one spring broke up...
        // ...remember the elements are now dirty
        mAreElementsDirty = true;
    }


    //
    // Detect connected components, if there have been deletions
    //

    if (mAreElementsDirty)
    {
        DetectConnectedComponents(currentStepSequenceNumber);
    }


    //
    // Update water dynamics
    //

    LeakWater(gameParameters);

    for (int i = 0; i < 4; i++)
        BalancePressure(gameParameters);

    for (int i = 0; i < 4; i++)
    {
        BalancePressure(gameParameters);
        GravitateWater(gameParameters);
    }


    //
    // Update electrical dynamics
    //

    DiffuseLight(gameParameters);
}

void Ship::Render(
    GameParameters const & /*gameParameters*/,
    RenderContext & renderContext) const
{
    //
    // Upload points's mutable attributes
    //

    mPoints.Upload(
        mId,
        renderContext);


    //
    // Upload elements
    //    

    if (!mConnectedComponentSizes.empty())
    {
        //
        // Upload elements (point (elements), springs, ropes, triangles), iff dirty
        //

        if (mAreElementsDirty)
        {
            renderContext.UploadShipElementsStart(
                mId,
                mConnectedComponentSizes);

            //
            // Upload all the point elements
            //

            mPoints.UploadElements(
                mId,
                renderContext);

            //
            // Upload all the spring elements (including ropes)
            //

            mSprings.UploadElements(
                mId,
                renderContext,
                mPoints);

            //
            // Upload all the triangle elements
            //

            mTriangles.UploadElements(
                mId,
                renderContext,
                mPoints);

            renderContext.UploadShipElementsEnd(mId);            
        }


        //
        // Upload stressed springs
        //

        renderContext.UploadShipElementStressedSpringsStart(mId);

        if (renderContext.GetShowStressedSprings())
        {        
            mSprings.UploadStressedSpringElements(
                mId,
                renderContext,
                mPoints);
        }

        renderContext.UploadShipElementStressedSpringsEnd(mId);


        //
        // Upload pinned points, if they've changed since the last time
        //

        if (mArePinnedPointsDirty || mAreElementsDirty)
        {
            renderContext.UploadShipElementPinnedPointsStart(mId);

            for (auto pinnedPointIndex : mCurrentPinnedPoints)
            {
                assert(!mPoints.IsDeleted(pinnedPointIndex));
                assert(mPoints.IsPinned(pinnedPointIndex));

                renderContext.UploadShipElementPinnedPoint(
                    mId,
                    mPoints.GetPosition(pinnedPointIndex).x,
                    mPoints.GetPosition(pinnedPointIndex).y,
                    mPoints.GetConnectedComponentId(pinnedPointIndex));
            }

            renderContext.UploadShipElementPinnedPointsEnd(mId);

            mArePinnedPointsDirty = false;
        }

        mAreElementsDirty = false;
    }        



    //
    // Render ship
    //

    renderContext.RenderShip(mId);
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////

void Ship::UpdateDynamics(GameParameters const & gameParameters)
{
    for (int iter = 0; iter < GameParameters::NumDynamicIterations<int>; ++iter)
    {
        // Update draw forces, if we have any
        if (!!mCurrentDrawForce)
        {
            UpdateDrawForces(
                mCurrentDrawForce->Position,
                mCurrentDrawForce->Strength);
        }

        // Update point forces
        UpdatePointForces(gameParameters);

        // Update springs forces
        UpdateSpringForces(gameParameters);

        // Integrate
        Integrate();

        // Handle collisions with sea floor
        HandleCollisionsWithSeaFloor();
    }

    //
    // Reset draw force
    //

    mCurrentDrawForce.reset();
}

void Ship::UpdateDrawForces(
    vec2f const & position,
    float forceStrength)
{
    for (auto pointIndex : mPoints)
    {
        vec2f displacement = (position - mPoints.GetPosition(pointIndex));
        float forceMagnitude = forceStrength / sqrtf(0.1f + displacement.length());
        mPoints.GetForce(pointIndex) += displacement.normalise() * forceMagnitude;
    }
}

void Ship::UpdatePointForces(GameParameters const & gameParameters)
{
    // Underwater points feel this amount of water drag
    //
    // The higher the value, the more viscous the water looks when a body moves through it
    constexpr float WaterDragCoefficient = 0.020f; // ~= 1.0f - powf(0.6f, 0.02f)
    
    for (auto pointIndex : mPoints)
    {
        // Get height of water at this point
        float const waterHeightAtThisPoint = mParentWorld->GetWaterHeightAt(mPoints.GetPosition(pointIndex).x);


        //
        // 1. Add gravity and buoyancy
        //

        float const effectiveBuoyancy = gameParameters.BuoyancyAdjustment * mPoints.GetBuoyancy(pointIndex);

        // Mass = own + contained water (clamped to 1)
        float effectiveMassMultiplier = 1.0f + std::min(mPoints.GetWater(pointIndex), 1.0f) * effectiveBuoyancy;
        if (mPoints.GetPosition(pointIndex).y < waterHeightAtThisPoint)
        {
            // Apply buoyancy of own mass (i.e. 1 * effectiveBuoyancy), which is
            // opposite to gravity
            effectiveMassMultiplier -= effectiveBuoyancy;
        }

        mPoints.GetForce(pointIndex) += gameParameters.Gravity * mPoints.GetMass(pointIndex) * effectiveMassMultiplier;


        //
        // 2. Apply water drag
        //
        // FUTURE: should replace with directional water drag, which acts on frontier points only, 
        // proportional to angle between velocity and normal to surface at this point;
        // this would ensure that masses would also have a horizontal velocity component when sinking,
        // providing a "gliding" effect
        //

        if (mPoints.GetPosition(pointIndex).y < waterHeightAtThisPoint)
        {
            mPoints.GetForce(pointIndex) += mPoints.GetVelocity(pointIndex) * (-WaterDragCoefficient);
        }
    }
}

void Ship::UpdateSpringForces(GameParameters const & /*gameParameters*/)
{
    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);

        // No need to check whether the spring is deleted, as a deleted spring
        // has zero coefficients

        vec2f const displacement = mPoints.GetPosition(pointBIndex) - mPoints.GetPosition(pointAIndex);
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);


        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        vec2f const fSpringA = springDir * (displacementLength - mSprings.GetRestLength(springIndex)) * mSprings.GetStiffnessCoefficient(springIndex);



        //
        // 2. Damper forces
        //
        // Damp the velocities of the two points, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = mPoints.GetVelocity(pointBIndex) - mPoints.GetVelocity(pointAIndex);
        vec2f const fDampA = springDir * relVelocity.dot(springDir) * mSprings.GetDampingCoefficient(springIndex);


        //
        // Apply forces
        //

        mPoints.GetForce(pointAIndex) += fSpringA + fDampA;
        mPoints.GetForce(pointBIndex) -= fSpringA + fDampA;
    }
}

void Ship::Integrate()
{
    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;

    // Global damp - lowers velocity uniformly, damping oscillations originating between gravity and buoyancy
    // Note: it's extremely sensitive, big difference between 0.9995 and 0.9998
    // Note: technically it's not a drag force, it's just a dimensionless deceleration
    float constexpr GlobalDampCoefficient = 0.9996f;

    //
    // Take the four buffers that we need as restrict pointers, so that the compiler
    // can better see it should parallelize this loop as much as possible
    //
    // This loop is compiled with single-precision packet SSE instructions on MSVC 17,
    // integrating two points at each iteration
    //

    float * restrict positionBuffer = mPoints.GetPositionBufferAsFloat();
    float * restrict velocityBuffer = mPoints.GetVelocityBufferAsFloat();
    float * restrict forceBuffer = mPoints.GetForceBufferAsFloat();
    float * restrict massFactorBuffer = mPoints.GetMassFactorBufferAsFloat();

    size_t const numIterations = mPoints.GetElementCount() * 2;
    for (size_t i = 0; i < numIterations; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos = velocityBuffer[i] * dt + forceBuffer[i] * massFactorBuffer[i];
        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * GlobalDampCoefficient / dt;

        // Zero out force now that we've integrated it
        forceBuffer[i] = 0.0f;
    }
}

void Ship::HandleCollisionsWithSeaFloor()
{
    for (auto pointIndex : mPoints)
    {
        // Check if point is now below the sea floor
        float const floorheight = mParentWorld->GetOceanFloorHeightAt(mPoints.GetPosition(pointIndex).x);
        if (mPoints.GetPosition(pointIndex).y < floorheight)
        {
            // Calculate normal to sea floor
            vec2f seaFloorNormal = vec2f(
                floorheight - mParentWorld->GetOceanFloorHeightAt(mPoints.GetPosition(pointIndex).x + 0.01f),
                0.01f).normalise();

            // Calculate displacement to move point back to sea floor, along the normal to the floor
            vec2f bounceDisplacement = seaFloorNormal * (floorheight - mPoints.GetPosition(pointIndex).y);

            // Move point back along normal
            mPoints.GetPosition(pointIndex) += bounceDisplacement;
            mPoints.GetVelocity(pointIndex) = bounceDisplacement / GameParameters::DynamicsSimulationStepTimeDuration<float>;
        }
    }
}

void Ship::DetectConnectedComponents(uint64_t currentStepSequenceNumber)
{
    mConnectedComponentSizes.clear();

    uint32_t currentConnectedComponentId = 0;
    std::queue<ElementContainer::ElementIndex> pointsToVisitForConnectedComponents;    

    // Visit all points
    for (auto pointIndex : mPoints)
    {
        // Don't visit destroyed points, or we run the risk of creating a zillion connected components
        if (!mPoints.IsDeleted(pointIndex))
        {
            // Check if visited
            if (mPoints.GetCurrentConnectedComponentDetectionStepSequenceNumber(pointIndex) != currentStepSequenceNumber)
            {
                // This node has not been visited, hence it's the beginning of a new connected component
                ++currentConnectedComponentId;
                size_t pointsInCurrentConnectedComponent = 0;

                //
                // Propagate the connected component ID to all points reachable from this point
                //

                assert(pointsToVisitForConnectedComponents.empty());
                pointsToVisitForConnectedComponents.push(pointIndex);
                mPoints.SetCurrentConnectedComponentDetectionStepSequenceNumber(pointIndex, currentStepSequenceNumber);

                while (!pointsToVisitForConnectedComponents.empty())
                {
                    auto currentPointIndex = pointsToVisitForConnectedComponents.front();
                    pointsToVisitForConnectedComponents.pop();

                    // Assign the connected component ID
                    mPoints.SetConnectedComponentId(currentPointIndex, currentConnectedComponentId);
                    ++pointsInCurrentConnectedComponent;

                    // Go through this point's adjacents
                    for (auto adjacentSpringElementIndex : mPoints.GetConnectedSprings(currentPointIndex))
                    {
                        assert(!mSprings.IsDeleted(adjacentSpringElementIndex));

                        auto pointAIndex = mSprings.GetPointAIndex(adjacentSpringElementIndex);
                        assert(!mPoints.IsDeleted(pointAIndex));
                        if (mPoints.GetCurrentConnectedComponentDetectionStepSequenceNumber(pointAIndex) != currentStepSequenceNumber)
                        {
                            mPoints.SetCurrentConnectedComponentDetectionStepSequenceNumber(pointAIndex, currentStepSequenceNumber);
                            pointsToVisitForConnectedComponents.push(pointAIndex);
                        }

                        auto pointBIndex = mSprings.GetPointBIndex(adjacentSpringElementIndex);
                        assert(!mPoints.IsDeleted(pointBIndex));
                        if (mPoints.GetCurrentConnectedComponentDetectionStepSequenceNumber(pointBIndex) != currentStepSequenceNumber)
                        {
                            mPoints.SetCurrentConnectedComponentDetectionStepSequenceNumber(pointBIndex, currentStepSequenceNumber);
                            pointsToVisitForConnectedComponents.push(pointBIndex);
                        }
                    }
                }

                // Store number of connected components
                mConnectedComponentSizes.push_back(pointsInCurrentConnectedComponent);
            }
        }
    }
}

void Ship::LeakWater(GameParameters const & gameParameters)
{
    for (auto pointIndex : mPoints)
    {
        //
        // Stuff some water into all the leaking nodes that are underwater, 
        // if the external pressure is larger
        //

        if (mPoints.IsLeaking(pointIndex))
        {
            float waterLevel = mParentWorld->GetWaterHeightAt(mPoints.GetPosition(pointIndex).x);

            float const externalWaterPressure = mPoints.GetExternalWaterPressure(
                pointIndex,
                waterLevel,
                gameParameters) * gameParameters.WaterPressureAdjustment;

            if (externalWaterPressure > mPoints.GetWater(pointIndex))
            {
                float newWater = GameParameters::SimulationStepTimeDuration<float> * (externalWaterPressure - mPoints.GetWater(pointIndex));
                mPoints.GetWater(pointIndex) += newWater;
                mTotalWater += newWater;
            }
        }
    }

    //
    // Check whether we've started sinking
    //

    if (!mIsSinking
        && mTotalWater > static_cast<float>(mPoints.GetElementCount()) / 2.0f)
    {
        // Started sinking
        mParentWorld->GetGameEventHandler()->OnSinkingBegin(mId);
        mIsSinking = true;
    }
}

void Ship::GravitateWater(GameParameters const & gameParameters)
{
    //
    // Water flows into adjacent nodes in a quantity proportional to the cos of angle the spring makes
    // against gravity (parallel with gravity => 1 (full flow), perpendicular = 0, parallel-opposite => -1 (goes back))
    //
    // Note: we don't take any shortcuts when a point has no water, as that would cause the speed of the 
    // simulation to change over time.
    //

    // Visit all connected non-hull points - i.e. non-hull springs
    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);

        // cos_theta > 0 => pointA above pointB
        float cos_theta = (mPoints.GetPosition(pointBIndex) - mPoints.GetPosition(pointAIndex)).normalise().dot(gameParameters.GravityNormal);

        // This amount of water falls in a second; 
        // a value too high causes all the water to be stuffed into the lowest node
        static constexpr float velocity = 0.60f;
                
        // Calculate amount of water that falls from highest point to lowest point
        float correction = mSprings.GetWaterPermeability(springIndex) * (velocity * GameParameters::SimulationStepTimeDuration<float>)
            * cos_theta  * (cos_theta > 0.0f ? mPoints.GetWater(pointAIndex) : mPoints.GetWater(pointBIndex));

        // TODO: use code below and store at Spring::WaterGravityFactorA/B
        ////float cos_theta_select = (1.0f + cos_theta) / 2.0f;
        ////float correction = 0.60f * cos_theta_select * pointA->GetWater() - 0.60f * (1.0f - cos_theta_select) * pointB->GetWater();
        ////correction *= GameParameters::SimulationStepTimeDuration<float>;

        mPoints.GetWater(pointAIndex) -= correction;
        mPoints.GetWater(pointBIndex) += correction;
    }
}

void Ship::BalancePressure(GameParameters const & /*gameParameters*/)
{
    //
    // If there's too much water in this node, try and push it into the others
    // (This needs to iterate over multiple frames for pressure waves to spread through water)
    //
    // Note: we don't take any shortcuts when a point has no water, as that would cause the speed of the 
    // simulation to change over time.
    //

    // Visit all connected non-hull points - i.e. non-hull springs
    for (auto springIndex : mSprings)
    {
        auto const pointAIndex = mSprings.GetPointAIndex(springIndex);
        float const aWater = mPoints.GetWater(pointAIndex);

        auto const pointBIndex = mSprings.GetPointBIndex(springIndex);
        float const bWater = mPoints.GetWater(pointBIndex);

        if (aWater < 1 && bWater < 1)   // if water content below threshold, no need to force water out
            continue;

        // This amount of water difference propagates in 1 second
        static constexpr float velocity = 2.5f;

        // Move water from more wet to less wet
        float const correction = mSprings.GetWaterPermeability(springIndex) * (bWater - aWater) * (velocity * GameParameters::SimulationStepTimeDuration<float>);
        mPoints.GetWater(pointAIndex) += correction;
        mPoints.GetWater(pointBIndex) -= correction;
    }
}

void Ship::DiffuseLight(GameParameters const & gameParameters)
{
    //
    // Diffuse light from each lamp to the closest adjacent (i.e. spring-connected) points,
    // inversely-proportional to the square of the distance
    //

    // Greater adjustment => underrated distance => wider diffusion
    float const adjustmentCoefficient = powf(1.0f - gameParameters.LightDiffusionAdjustment, 2.0f);

    // Visit all points
    for (auto pointIndex : mPoints)
    {
        // Zero light
        mPoints.GetLight(pointIndex) = 0.0f;

        vec2f const & pointPosition = mPoints.GetPosition(pointIndex);

        // Go through all lamps in the same connected component
        for (auto electricaElementIndex : mElectricalElements)
        {
            if (!mElectricalElements.IsDeleted(electricaElementIndex))
            {
                if (ElectricalElement::Type::Lamp == mElectricalElements.GetElectricalElement(electricaElementIndex)->GetType()
                    && mPoints.GetConnectedComponentId(mElectricalElements.GetElectricalElement(electricaElementIndex)->GetPointIndex()) == mPoints.GetConnectedComponentId(pointIndex))
                {
                    auto lampPointIndex = mElectricalElements.GetElectricalElement(electricaElementIndex)->GetPointIndex();

                    assert(!mPoints.IsDeleted(lampPointIndex));

                    // TODO: this needs to be replaced with getting Light from the lamp itself
                    float const lampLight = 1.0f;

                    float squareDistance = std::max(
                        1.0f,
                        (pointPosition - mPoints.GetPosition(lampPointIndex)).squareLength() * adjustmentCoefficient);

                    assert(squareDistance >= 1.0f);

                    float newLight = lampLight / squareDistance;
                    if (newLight > mPoints.GetLight(pointIndex))
                        mPoints.GetLight(pointIndex) = newLight;
                }
            }
        }
    }
}

}