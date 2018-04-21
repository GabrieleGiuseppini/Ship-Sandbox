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


namespace /* anonymous */ {

    bool IsConnectedToNonRopePoints(Point const * p)
    {
        assert(p->GetMaterial()->IsRope);

        for (auto const & spring : p->GetConnectedSprings())
        {
            if (!spring->GetPointA()->GetMaterial()->IsRope
                || !spring->GetPointB()->GetMaterial()->IsRope)
            {
                return true;
            }
        }

        return false;
    }

}

std::unique_ptr<Ship> Ship::Create(
    int shipId,
    World * parentWorld,
    ShipDefinition const & shipDefinition,
    MaterialDatabase const & materials,
    GameParameters const & /*gameParameters*/,
    uint64_t currentStepSequenceNumber)
{
    //
    // 1. Process image points and create:
    // - PointInfo's
    // - Matrix of point indices
    // - RopeInfo's
    //    

    // PointInfo's

    struct PointInfo
    {
        vec2f Position;
        vec2f TextureCoordinates;
        Material const * Mtl;

        PointInfo(
            vec2f position,
            vec2f textureCoordinates,
            Material const * mtl)
            : Position(position)
            , TextureCoordinates(textureCoordinates)
            , Mtl(mtl)
        {
        }
    };

    std::vector<PointInfo> pointInfos;


    // Matrix of points

    int const structureWidth = shipDefinition.StructuralImage.Size.Width;
    float const halfWidth = static_cast<float>(structureWidth) / 2.0f;
    int const structureHeight = shipDefinition.StructuralImage.Size.Height;

    // We allocate 2 extra rows and cols to avoid checking for boundaries
    std::unique_ptr<std::unique_ptr<std::optional<size_t>[]>[]> pointIndexMatrix(new std::unique_ptr<std::optional<size_t>[]>[structureWidth + 2]);    
    

    // RopeInfo's

    struct RopeInfo
    {
        std::optional<size_t> PointAIndex;
        std::optional<size_t> PointBIndex;

        RopeInfo()
            : PointAIndex(std::nullopt)
            , PointBIndex(std::nullopt)
        {
        }
    };

    std::map<std::array<uint8_t, 3u>, RopeInfo> ropeInfos;


    // First dummy column
    pointIndexMatrix[0] = std::unique_ptr<std::optional<size_t>[]>(new std::optional<size_t>[structureHeight + 2]);

    // Visit all real columns
    for (int x = 0; x < structureWidth; ++x)
    {
        // We allocate 2 extra rows and cols to avoid checking for boundaries
        pointIndexMatrix[x + 1] = std::unique_ptr<std::optional<size_t>[]>(new std::optional<size_t>[structureHeight + 2]);

        // From bottom to top
        for (int y = 0; y < structureHeight; ++y)
        {
            // R G B
            std::array<uint8_t, 3u> rgbColour = {
                shipDefinition.StructuralImage.Data[(x + (structureHeight - y - 1) * structureWidth) * 3 + 0],
                shipDefinition.StructuralImage.Data[(x + (structureHeight - y - 1) * structureWidth) * 3 + 1],
                shipDefinition.StructuralImage.Data[(x + (structureHeight - y - 1) * structureWidth) * 3 + 2] };

            Material const * material = materials.Get(rgbColour);
            if (nullptr == material)
            {
                // Check whether it's a rope endpoint (#000xxx)
                if (0x00 == rgbColour[0]
                    && 0 == (rgbColour[1] & 0xF0))
                {
                    // Store in RopeInfo
                    RopeInfo & ropeInfo = ropeInfos[rgbColour];
                    if (!ropeInfo.PointAIndex)
                    {
                        ropeInfo.PointAIndex = pointInfos.size();
                    }
                    else if (!ropeInfo.PointBIndex)
                    {
                        ropeInfo.PointBIndex = pointInfos.size();
                    }
                    else
                    {
                        throw GameException(
                            std::string("More than two rope endpoints found at (")
                            + std::to_string(x) + "," + std::to_string(y) + ")");
                    }

                    // Point to rope (#000000)
                    material = &(materials.GetRopeMaterial());
                }
            }

            if (nullptr != material)
            {
                //
                // Make a point
                //

                pointIndexMatrix[x + 1][y + 1] = pointInfos.size();

                pointInfos.emplace_back(
                    vec2f(
                        static_cast<float>(x) - halfWidth,
                        static_cast<float>(y))
                        + shipDefinition.Offset,
                    vec2f(
                        static_cast<float>(x) / static_cast<float>(structureWidth),
                        static_cast<float>(y) / static_cast<float>(structureHeight)),
                    material);
            }
        }
    }

    // Last dummy column
    pointIndexMatrix[structureWidth + 1] = std::unique_ptr<std::optional<size_t>[]>(new std::optional<size_t>[structureHeight + 2]);

    //
    // 2. Process RopeInfo's and create:
    // - Additional PointInfo's
    // - SpringInfo's (for ropes)
    //    

    struct SpringInfo
    {
        size_t PointAIndex;
        size_t PointBIndex;

        SpringInfo(
            size_t pointAIndex,
            size_t pointBIndex)
            : PointAIndex(pointAIndex)
            , PointBIndex(pointBIndex)
        {
        }
    };

    std::vector<SpringInfo> springInfos;

    // Visit all RopeInfo's
    for (auto const & ropeInfoEntry : ropeInfos)
    {
        auto const & ropeInfo = ropeInfoEntry.second;

        // Make sure we've got both endpoints
        assert(!!ropeInfo.PointAIndex);
        if (!ropeInfo.PointBIndex)
        {
            throw GameException(
                std::string("Only one rope endpoint found with index <")
                + std::to_string(static_cast<int>(ropeInfoEntry.first[1]))
                + "," + std::to_string(static_cast<int>(ropeInfoEntry.first[2])) + ">");
        }

        // Get endpoint positions
        vec2f startPos = pointInfos[*ropeInfo.PointAIndex].Position;
        vec2f endPos = pointInfos[*ropeInfo.PointBIndex].Position;

        //
        // "Draw" line from start position to end position
        //
        // Go along widest of Dx and Dy, in steps of 1.0, until we're very close to end position
        //
        
        // W = wide, N = narrow

        float dx = endPos.x - startPos.x;
        float dy = endPos.y - startPos.y;
        bool widestIsX;
        float slope;
        float curW, curN;
        float endW;
        float stepW;
        if (fabs(dx) > fabs(dy))
        {
            widestIsX = true;
            slope = dy / dx;
            curW = startPos.x;
            curN = startPos.y;
            endW = endPos.x;
            stepW = dx / fabs(dx);
        }
        else
        {
            widestIsX = false;
            slope = dx / dy;
            curW = startPos.y;
            curN = startPos.x;
            endW = endPos.y;
            stepW = dy / fabs(dy);
        }

        size_t curStartPointIndex = *ropeInfo.PointAIndex;
        while (true)
        {
            curW += stepW;
            curN += slope * stepW;

            if (fabs(endW - curW) <= 0.5f)
                break;

            // Create position
            vec2f newPosition;
            if (widestIsX)
            { 
                newPosition = vec2f(curW, curN);
            }
            else
            {
                newPosition = vec2f(curN, curW);
            }

            // Add SpringInfo
            springInfos.emplace_back(curStartPointIndex, pointInfos.size());

            curStartPointIndex = pointInfos.size();

            // Add PointInfo
            pointInfos.emplace_back(
                newPosition,
                vec2f(
                    newPosition.x / static_cast<float>(structureWidth),
                    newPosition.y / static_cast<float>(structureHeight)),
                &(materials.GetRopeMaterial()));
        }

        // Add last SpringInfo (no PointInfo as the endpoint has already a PointInfo)
        springInfos.emplace_back(curStartPointIndex, *ropeInfo.PointBIndex);
    }


    //
    // 3. Visit all PointInfo's and create:
    //  - Points
    //  - PointColors
    //  - PointTextureCoordinates
    //

    Ship *ship = new Ship(shipId, parentWorld);

    ElementRepository<Point> allPoints(pointInfos.size());
    ElementRepository<vec3f> allPointColors(pointInfos.size());
    ElementRepository<vec2f> allPointTextureCoordinates(pointInfos.size());

    for (size_t p = 0; p < pointInfos.size(); ++p)
    {
        PointInfo const & pointInfo = pointInfos[p];

        Material const * mtl = pointInfo.Mtl;

        //
        // Create point
        //

        allPoints.emplace_back(
            ship,
            pointInfo.Position,
            mtl,
            mtl->IsHull ? 0.0f : 1.0f, // No buoyancy if it's a hull point, as it can't get water
            static_cast<int>(p));

        //
        // Create point color
        //

        allPointColors.emplace_back(mtl->RenderColour);


        //
        // Create point texture coordinates
        //

        if (!!shipDefinition.TextureImage)
        {
            allPointTextureCoordinates.emplace_back(pointInfo.TextureCoordinates);
        }
    }


    //
    // 4. Visit point matrix and:
    //  - Set non-fully-surrounded Points as leaking
    //  - Create SpringInfo (additional to ropes)
    //  - Create TriangleInfo
    //

    size_t leakingPointsCount = 0;


    struct TriangleInfo
    {
        size_t PointAIndex;
        size_t PointBIndex;
        size_t PointCIndex;

        TriangleInfo(
            size_t pointAIndex,
            size_t pointBIndex,
            size_t pointCIndex)
            : PointAIndex(pointAIndex)
            , PointBIndex(pointBIndex)
            , PointCIndex(pointCIndex)
        {
        }
    };

    std::vector<TriangleInfo> triangleInfos;


    // Visit point matrix

    static const int Directions[8][2] = {
        {  1,  0 },  // E
        {  1, -1 },  // SE
        {  0, -1 },  // S
        { -1, -1 },  // SW
        { -1,  0 },  // W
        { -1,  1 },  // NW
        {  0,  1 },  // N
        {  1,  1 }   // NE
    };

    // From bottom to top
    for (int y = 1; y <= structureHeight; ++y)
    {
        // We're starting a new row, so we're not in a ship now
        bool isInShip = false;

       for (int x = 1; x <= structureWidth; ++x)
        {
            if (!!pointIndexMatrix[x][y])
            {
                //
                // A point exists at these coordinates
                //

                size_t pointIndex = *pointIndexMatrix[x][y];
                Point & point = allPoints[pointIndex];

                // If a non-hull node has empty space on one of its four sides, it is automatically leaking.
                // Check if a is leaking; a is leaking if:
                // - a is not hull, AND
                // - there is at least a hole at E, S, W, N
                if (!point.GetMaterial()->IsHull)
                {
                    if (!pointIndexMatrix[x + 1][y]
                        || !pointIndexMatrix[x][y + 1]
                        || !pointIndexMatrix[x - 1][y]
                        || !pointIndexMatrix[x][y - 1])
                    {
                        point.SetLeaking();

                        ++leakingPointsCount;
                    }
                }


                //
                // Check if a spring exists
                //

                // First four directions out of 8: from 0 deg (+x) through to 225 deg (-x -y),
                // i.e. E, SE, S, SW - this covers each pair of points in each direction
                for (int i = 0; i < 4; ++i)
                {
                    int adjx1 = x + Directions[i][0];
                    int adjy1 = y + Directions[i][1];                    

                    if (!!pointIndexMatrix[adjx1][adjy1])
                    {
                        // This point is adjacent to the first point at one of E, SE, S, SW

                        //
                        // Create SpringInfo
                        // 

                        springInfos.emplace_back(
                            pointIndex, 
                            *pointIndexMatrix[adjx1][adjy1]);


                        //
                        // Check if a triangle exists
                        // - If this is the first point that is in a ship, we check all the way up to W;
                        // - Else, we check up to S, so to avoid covering areas already covered by the triangulation
                        //   at the previous point
                        //

                        // Check adjacent point in next CW direction
                        int adjx2 = x + Directions[i + 1][0];
                        int adjy2 = y + Directions[i + 1][1];   
                        if ( (!isInShip || i < 2) 
                             && !!pointIndexMatrix[adjx2][adjy2])
                        {
                            // This point is adjacent to the first point at one of SE, S, SW, W

                            //
                            // Create TriangleInfo
                            // 

                            triangleInfos.emplace_back(
                                pointIndex,
                                *pointIndexMatrix[adjx1][adjy1],
                                *pointIndexMatrix[adjx2][adjy2]);
                        }

                        // Now, we also want to check whether the single "irregular" triangle from this point exists, 
                        // i.e. the triangle between this point, the point at its E, and the point at its
                        // S, in case there is no point at SE.
                        // We do this so that we can forget the entire W side for inner points and yet ensure
                        // full coverage of the area
                        if (i == 0
                            && !pointIndexMatrix[x + Directions[1][0]][y + Directions[1][1]]
                            && !!pointIndexMatrix[x + Directions[2][0]][y + Directions[2][1]])
                        { 
                            // If we're here, the point at E exists
                            assert(!!pointIndexMatrix[x + Directions[0][0]][y + Directions[0][1]]);

                            //
                            // Create TriangleInfo
                            // 

                            triangleInfos.emplace_back(
                                pointIndex,
                                *pointIndexMatrix[x + Directions[0][0]][y + Directions[0][1]],
                                *pointIndexMatrix[x + Directions[2][0]][y + Directions[2][1]]);
                        }
                    }
                }

                // Remember now that we're in a ship
                isInShip = true;
            }
            else
            {
                //
                // No point exists at these coordinates
                //

                // From now on we're not in a ship anymore
                isInShip = false;
            }
        }
    }


    //
    // 5. Create Springs for all SpringInfo's
    //

    ElementRepository<Spring> allSprings(springInfos.size());

    for (SpringInfo const & springInfo : springInfos)
    {
        Point * a = &(allPoints[springInfo.PointAIndex]);
        Point * b = &(allPoints[springInfo.PointBIndex]);

        // We choose the spring to be as strong as its strongest point
        Material const * const strongestMaterial = a->GetMaterial()->Strength > b->GetMaterial()->Strength ? a->GetMaterial() : b->GetMaterial();

        int characteristics = 0;

        // The spring is hull if at least one node is hull 
        // (we don't propagate water along a hull spring)
        if (a->GetMaterial()->IsHull || b->GetMaterial()->IsHull)
            characteristics |= static_cast<int>(Spring::Characteristics::Hull);

        // If both nodes are rope, then the spring is rope 
        // (non-rope <-> rope springs are "connections" and not to be treated as ropes)
        if (a->GetMaterial()->IsRope && b->GetMaterial()->IsRope)
            characteristics |= static_cast<int>(Spring::Characteristics::Rope);

        // Create spring
        allSprings.emplace_back(
            ship,
            a,
            b,
            static_cast<Spring::Characteristics>(characteristics),
            strongestMaterial);
    }


    //
    // 6. Create Triangles for all TriangleInfo's except those whose vertices
    //    are all rope points, of which at least one is connected exclusively 
    //    to rope points (these would be knots "sticking out" of the structure)
    //

    ElementRepository<Triangle> allTriangles(triangleInfos.size());

    for (TriangleInfo const & triangleInfo : triangleInfos)
    {
        Point * a = &(allPoints[triangleInfo.PointAIndex]);
        Point * b = &(allPoints[triangleInfo.PointBIndex]);
        Point * c = &(allPoints[triangleInfo.PointCIndex]);

        if (a->GetMaterial()->IsRope
            && b->GetMaterial()->IsRope
            && c->GetMaterial()->IsRope)
        {
            // Do not add triangle if at least one vertex is connected to rope points only
            if (!IsConnectedToNonRopePoints(a)
                || !IsConnectedToNonRopePoints(b)
                || !IsConnectedToNonRopePoints(c))
            {
                continue;
            }
        }

        allTriangles.emplace_back(
            ship,
            a,
            b,
            c);
    }


    //
    // 7. Create Electrical Elements
    //

    std::vector<ElectricalElement*> allElectricalElements;

    for (Point & point : allPoints)
    {
        if (!!point.GetMaterial()->Electrical)
        {
            switch (point.GetMaterial()->Electrical->ElementType)
            {
                case Material::ElectricalProperties::ElectricalElementType::Cable:
                {
                    allElectricalElements.emplace_back(new Cable(ship, &point));
                    break;
                }

                case Material::ElectricalProperties::ElectricalElementType::Generator:
                {
                    allElectricalElements.emplace_back(new Generator(ship, &point));
                    break;
                }

                case Material::ElectricalProperties::ElectricalElementType::Lamp:
                {
                    allElectricalElements.emplace_back(new Lamp(ship, &point));
                    break;
                }
            }
        }
    }

    LogMessage("Created ship: W=", shipDefinition.StructuralImage.Size.Width, ", H=", shipDefinition.StructuralImage.Size.Height, ", ",
        allPoints.size(), " points, ", allSprings.size(),
        " springs, ", allTriangles.size(), " triangles, ", allElectricalElements.size(), " electrical elements.");

    ship->Initialize(
        std::move(allPoints),   
        std::move(allPointColors),
        std::move(allPointTextureCoordinates),
        std::move(allSprings),
        std::move(allTriangles),
        std::move(allElectricalElements),
        currentStepSequenceNumber);

    return std::unique_ptr<Ship>(ship);
}

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