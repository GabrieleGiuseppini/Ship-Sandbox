/***************************************************************************************
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
    World * parentWorld)
    : mId(id)
    , mParentWorld(parentWorld)    
    , mAllPoints(0)
    , mAllPointColors(0)
    , mAllPointTextureCoordinates(0)
    , mAllSprings(0)
    , mAllTriangles(0)
    , mAllElectricalElements()
    , mConnectedComponentSizes()
    , mIsPointCountDirty(true)
    , mAreElementsDirty(true)
    , mIsSinking(false)
    , mTotalWater(0.0)
    , mCurrentDrawForce(std::nullopt)
{
}

Ship::~Ship()
{
    // Delete elements that are not unique_ptr's nor are kept
    // in a PointerContainer

    // (None now!)
}

void Ship::DestroyAt(
    vec2 const & targetPos, 
    float radius)
{
    // Destroy all points within the radius
    for (Point & point : mAllPoints)
    {
        if (!point.IsDeleted())
        {
            if ((point.GetPosition() - targetPos).length() < radius)
            {
                // Notify
                mParentWorld->GetGameEventHandler()->OnDestroy(
                    point.GetMaterial(), 
                    mParentWorld->IsUnderwater(point.GetPosition()),
                    1u);

                // Destroy point
                point.Destroy();
            }
        }
    }

    mAllElectricalElements.shrink_to_fit();
}

void Ship::DrawTo(
    vec2f const & targetPos,
    float strength)
{
    // Store
    assert(!mCurrentDrawForce);
    mCurrentDrawForce.emplace(targetPos, strength);
}

Point const * Ship::GetNearestPointAt(
    vec2 const & targetPos, 
    float radius) const
{
    Point const * bestPoint = nullptr;
    float bestDistance = std::numeric_limits<float>::max();

    for (Point const & point : mAllPoints)
    {
        if (!point.IsDeleted())
        {
            float distance = (point.GetPosition() - targetPos).length();
            if (distance < radius && distance < bestDistance)
            {
                bestPoint = &point;
                bestDistance = distance;
            }
        }
    }

    return bestPoint;
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

    for (Spring & spring : mAllSprings)
    {
        // Avoid breaking deleted springs
        if (!spring.IsDeleted())
        {
            spring.UpdateStrain(gameParameters, gameEventHandler);
        }
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

    // Clear up pointer containers, in case there have been deletions
    // during or before this update step
    mAllElectricalElements.shrink_to_fit();

    DiffuseLight(gameParameters);
}

void Ship::Render(
    GameParameters const & /*gameParameters*/,
    RenderContext & renderContext) const
{
    if (mIsPointCountDirty)
    {
        //
        // Upload point colors and texture coordinates
        //

        renderContext.UploadShipPointVisualAttributes(
            mId,
            mAllPointColors.data(), 
            mAllPointTextureCoordinates.data(),
            mAllPointColors.size());

        mIsPointCountDirty = false;
    }


    //
    // Upload points
    //

    renderContext.UploadShipPointsStart(
        mId,
        mAllPoints.size());

    for (Point const & point : mAllPoints)
    {
        renderContext.UploadShipPoint(
            mId,
            point.GetPosition().x,
            point.GetPosition().y,
            point.GetLight(),
            point.GetWater());
    }

    renderContext.UploadShipPointsEnd(mId);


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
            // Upload all the points
            //

            for (Point const & point : mAllPoints)
            {
                if (!point.IsDeleted())
                {
                    renderContext.UploadShipElementPoint(
                        mId,
                        point.GetElementIndex(),
                        point.GetConnectedComponentId());
                }
            }

            //
            // Upload all the springs (including ropes)
            //

            for (Spring const & spring : mAllSprings)
            {
                if (!spring.IsDeleted())
                {
                    assert(spring.GetPointA()->GetConnectedComponentId() == spring.GetPointB()->GetConnectedComponentId());

                    if (spring.IsRope())
                    {
                        renderContext.UploadShipElementRope(
                            mId,
                            spring.GetPointA()->GetElementIndex(),
                            spring.GetPointB()->GetElementIndex(),
                            spring.GetPointA()->GetConnectedComponentId());
                    }
                    else
                    {
                        renderContext.UploadShipElementSpring(
                            mId,
                            spring.GetPointA()->GetElementIndex(),
                            spring.GetPointB()->GetElementIndex(),
                            spring.GetPointA()->GetConnectedComponentId());
                    }
                }
            }


            //
            // Upload all the triangles
            //

            for (Triangle const & triangle : mAllTriangles)
            {
                if (!triangle.IsDeleted())
                {
                    assert(triangle.GetPointA()->GetConnectedComponentId() == triangle.GetPointB()->GetConnectedComponentId()
                        && triangle.GetPointA()->GetConnectedComponentId() == triangle.GetPointC()->GetConnectedComponentId());

                    renderContext.UploadShipElementTriangle(
                        mId,
                        triangle.GetPointA()->GetElementIndex(),
                        triangle.GetPointB()->GetElementIndex(),
                        triangle.GetPointC()->GetElementIndex(),
                        triangle.GetPointA()->GetConnectedComponentId());
                }
            }

            renderContext.UploadShipElementsEnd(mId);

            mAreElementsDirty = false;
        }


        //
        // Upload stressed springs
        //

        renderContext.UploadShipElementStressedSpringsStart(mId);

        if (renderContext.GetShowStressedSprings())
        {            
            for (Spring const & spring : mAllSprings)
            {
                if (!spring.IsDeleted())
                {
                    if (spring.IsStressed())
                    {
                        assert(spring.GetPointA()->GetConnectedComponentId() == spring.GetPointB()->GetConnectedComponentId());

                        renderContext.UploadShipElementStressedSpring(
                            mId,
                            spring.GetPointA()->GetElementIndex(),
                            spring.GetPointB()->GetElementIndex(),
                            spring.GetPointA()->GetConnectedComponentId());
                    }
                }
            }
        }

        renderContext.UploadShipElementStressedSpringsEnd(mId);
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
        // Update draw forces
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
    float strength)
{
    for (Point & point : mAllPoints)
    {
        vec2f displacement = (position - point.GetPosition());
        float forceMagnitude = strength / sqrtf(0.1f + displacement.length());
        point.AddToForce(displacement.normalise() * forceMagnitude);
    }
}

void Ship::UpdatePointForces(GameParameters const & gameParameters)
{
    // Underwater points feel this amount of water drag
    //
    // The higher the value, the more viscous the water looks when a body moves through it
    constexpr float WaterDragCoefficient = 0.020f; // ~= 1.0f - powf(0.6f, 0.02f)
    
    for (Point & point : mAllPoints)
    {
        // Get height of water at this point
        float const waterHeightAtThisPoint = mParentWorld->GetWaterHeightAt(point.GetPosition().x);


        //
        // 1. Add gravity and buoyancy
        //

        float const effectiveBuoyancy = gameParameters.BuoyancyAdjustment * point.GetBuoyancy();

        // Mass = own + contained water (clamped to 1)
        float effectiveMassMultiplier = 1.0f + std::min(point.GetWater(), 1.0f) * effectiveBuoyancy;
        if (point.GetPosition().y < waterHeightAtThisPoint)
        {
            // Apply buoyancy of own mass (i.e. 1 * effectiveBuoyancy), which is
            // opposite to gravity
            effectiveMassMultiplier -= effectiveBuoyancy;
        }

        point.AddToForce(gameParameters.Gravity * point.GetMaterial()->Mass * effectiveMassMultiplier);


        //
        // 2. Apply water drag
        //
        // TBD: should replace with directional water drag, which acts on frontier points only, 
        // proportional to angle between velocity and normal to surface at this point;
        // this would ensure that masses would also have a horizontal velocity component when sinking
        //

        if (point.GetPosition().y < waterHeightAtThisPoint)
        {
            point.AddToForce(point.GetVelocity() * (-WaterDragCoefficient));
        }
    }
}

void Ship::UpdateSpringForces(GameParameters const & /*gameParameters*/)
{
    for (Spring & spring : mAllSprings)
    {
        // No need to check whether the spring is deleted, as a deleted spring
        // has zero coefficients

        vec2f const displacement = (spring.GetPointB()->GetPosition() - spring.GetPointA()->GetPosition());
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);


        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        vec2f const fSpringA = springDir * (displacementLength - spring.GetRestLength()) * spring.GetStiffnessCoefficient();

        // Apply force 
        spring.GetPointA()->AddToForce(fSpringA);
        spring.GetPointB()->AddToForce(-fSpringA);


        //
        // 2. Damper forces
        //
        // Damp the velocities of the two points, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = (spring.GetPointB()->GetVelocity() - spring.GetPointA()->GetVelocity());
        vec2f const fDampA = springDir * relVelocity.dot(springDir) * spring.GetDampingCoefficient();

        // Apply force
        spring.GetPointA()->AddToForce(fDampA);
        spring.GetPointB()->AddToForce(-fDampA);
    }
}

void Ship::Integrate()
{
    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;

    // Global damp - lowers velocity uniformly, damping oscillations originating between gravity and buoyancy
    // Note: it's extremely sensitive, big difference between 0.9995 and 0.9998
    // Note: it's not technically a drag force, it's just a dimensionless deceleration
    float constexpr GlobalDampCoefficient = 0.9995f;

    for (Point & point : mAllPoints)
    {
        // Verlet (fourth order, with velocity being first order)
        // - For each point: 6 * mul + 4 * add
        auto const deltaPos = point.GetVelocity() * dt + point.GetForce() * point.GetMassFactor();
        point.SetPosition(point.GetPosition() + deltaPos);
        point.SetVelocity(deltaPos * GlobalDampCoefficient / dt);
        point.ZeroForce();
    }
}

void Ship::HandleCollisionsWithSeaFloor()
{
    for (Point & point : mAllPoints)
    {
        // Check if point is now below the sea floor
        float const floorheight = mParentWorld->GetOceanFloorHeightAt(point.GetPosition().x);
        if (point.GetPosition().y < floorheight)
        {
            // Calculate normal to sea floor
            vec2f seaFloorNormal = vec2f(
                floorheight - mParentWorld->GetOceanFloorHeightAt(point.GetPosition().x + 0.01f),
                0.01f).normalise();

            // Calculate displacement to move point back to sea floor, along the normal to the floor
            vec2f bounceDisplacement = seaFloorNormal * (floorheight - point.GetPosition().y);

            // Move point back along normal
            point.AddToPosition(bounceDisplacement);
            point.SetVelocity(bounceDisplacement / GameParameters::DynamicsSimulationStepTimeDuration<float>);
        }
    }
}

void Ship::DetectConnectedComponents(uint64_t currentStepSequenceNumber)
{
    mConnectedComponentSizes.clear();

    uint64_t currentConnectedComponentId = 0;
    std::queue<Point *> pointsToVisitForConnectedComponents;    

    // Visit all points
    for (Point & point : mAllPoints)
    {
        // Don't visit destroyed points, or we run the risk of creating a zillion connected components
        if (!point.IsDeleted())
        {
            // Check if visited
            if (point.GetCurrentConnectedComponentDetectionStepSequenceNumber() != currentStepSequenceNumber)
            {
                // This node has not been visited, hence it's the beginning of a new connected component
                ++currentConnectedComponentId;
                size_t pointsInCurrentConnectedComponent = 0;

                //
                // Propagate the connected component ID to all points reachable from this point
                //

                assert(pointsToVisitForConnectedComponents.empty());
                pointsToVisitForConnectedComponents.push(&point);
                point.SetConnectedComponentDetectionStepSequenceNumber(currentStepSequenceNumber);

                while (!pointsToVisitForConnectedComponents.empty())
                {
                    Point * currentPoint = pointsToVisitForConnectedComponents.front();
                    pointsToVisitForConnectedComponents.pop();

                    // Assign the connected component ID
                    currentPoint->SetConnectedComponentId(currentConnectedComponentId);
                    ++pointsInCurrentConnectedComponent;

                    // Go through this point's adjacents
                    for (Spring * spring : currentPoint->GetConnectedSprings())
                    {
                        assert(!spring->IsDeleted());

                        Point * const pointA = spring->GetPointA();
                        assert(!pointA->IsDeleted());
                        if (pointA->GetCurrentConnectedComponentDetectionStepSequenceNumber() != currentStepSequenceNumber)
                        {
                            pointA->SetConnectedComponentDetectionStepSequenceNumber(currentStepSequenceNumber);
                            pointsToVisitForConnectedComponents.push(pointA);
                        }

                        Point * const pointB = spring->GetPointB();
                        assert(!pointB->IsDeleted());
                        if (pointB->GetCurrentConnectedComponentDetectionStepSequenceNumber() != currentStepSequenceNumber)
                        {
                            pointB->SetConnectedComponentDetectionStepSequenceNumber(currentStepSequenceNumber);
                            pointsToVisitForConnectedComponents.push(pointB);
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
    for (Point & point : mAllPoints)
    {
        //
        // 1) Leak water: stuff some water into all the leaking nodes that are underwater, 
        //    if the external pressure is larger
        //

        if (point.IsLeaking())
        {
            float waterLevel = mParentWorld->GetWaterHeightAt(point.GetPosition().x);

            float const externalWaterPressure = point.GetExternalWaterPressure(
                waterLevel,
                gameParameters);

            if (externalWaterPressure > point.GetWater())
            {
                float newWater = GameParameters::SimulationStepTimeDuration<float> * gameParameters.WaterPressureAdjustment * (externalWaterPressure - point.GetWater());
                point.AddWater(newWater);
                mTotalWater += newWater;
            }
        }
    }

    //
    // Check whether we've started sinking
    //

    if (!mIsSinking
        && mTotalWater > static_cast<float>(mAllPoints.size()) / 2.0f)
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
    for (Spring & spring : mAllSprings)
    {
        Point * const pointA = spring.GetPointA();
        Point * const pointB = spring.GetPointB();

        // cos_theta > 0 => pointA above pointB
        float cos_theta = (pointB->GetPosition() - pointA->GetPosition()).normalise().dot(gameParameters.GravityNormal);

        // This amount of water falls in a second; 
        // a value too high causes all the water to be stuffed into the lowest node
        static constexpr float velocity = 0.60f;
                
        // Calculate amount of water that falls from highest point to lowest point
        float correction = spring.GetWaterPermeability() * (velocity * GameParameters::SimulationStepTimeDuration<float>)
            * cos_theta  * (cos_theta > 0.0f ? pointA->GetWater() : pointB->GetWater());

        // TODO: use code below and store at Spring::WaterGravityFactorA/B
        ////float cos_theta_select = (1.0f + cos_theta) / 2.0f;
        ////float correction = 0.60f * cos_theta_select * pointA->GetWater() - 0.60f * (1.0f - cos_theta_select) * pointB->GetWater();
        ////correction *= GameParameters::SimulationStepTimeDuration<float>;

        pointA->AddWater(-correction);
        pointB->AddWater(correction);
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
    for (Spring & spring : mAllSprings)
    {
        Point * const pointA = spring.GetPointA();
        float const aWater = pointA->GetWater();

        Point * const pointB = spring.GetPointB();
        float const bWater = pointB->GetWater();

        if (aWater < 1 && bWater < 1)   // if water content below threshold, no need to force water out
            continue;

        // This amount of water difference propagates in 1 second
        static constexpr float velocity = 2.5f;

        // Move water from more wet to less wet
        float const correction = spring.GetWaterPermeability() * (bWater - aWater) * (velocity * GameParameters::SimulationStepTimeDuration<float>);
        pointA->AddWater(correction);
        pointB->AddWater(-correction);
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
    for (Point & point : mAllPoints)
    {
        point.ZeroLight();

        vec2f const & pointPosition = point.GetPosition();

        // Go through all lamps in the same connected component
        for (ElectricalElement * el : mAllElectricalElements)
        {
            assert(!el->IsDeleted());

            if (ElectricalElement::Type::Lamp == el->GetType()
                && el->GetPoint()->GetConnectedComponentId() == point.GetConnectedComponentId())
            {
                Point * const lampPoint = el->GetPoint();

                assert(!lampPoint->IsDeleted());

                // TODO: this needs to be replaced with getting Light from the lamp itself
                float const lampLight = 1.0f;

                float squareDistance = std::max(
                    1.0f,
                    (pointPosition - lampPoint->GetPosition()).squareLength() * adjustmentCoefficient);

                assert(squareDistance >= 1.0f);

                point.AdjustLight(lampLight / squareDistance);
            }
        }
    }
}

}