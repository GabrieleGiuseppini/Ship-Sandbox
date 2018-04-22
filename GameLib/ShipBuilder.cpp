﻿/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
*
* Contains code from Tom Forsyth - https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
***************************************************************************************/
#include "ShipBuilder.h"

#include "Log.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <queue>
#include <set>

using namespace Physics;

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

std::unique_ptr<Ship> ShipBuilder::Create(
    int shipId,
    World * parentWorld,
    ShipDefinition const & shipDefinition,
    MaterialDatabase const & materials,
    GameParameters const & /*gameParameters*/,
    uint64_t currentStepSequenceNumber)
{
    //
    // 1. Process image points and:
    // - Identify all points, and create PointInfo's for them
    // - Build a 2D matrix containing indices to the points above
    // - Identify rope endpoints, and create RopeSegment's for them
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
    

    // RopeEndpoints's

    struct RopeSegment
    {
        std::optional<size_t> PointAIndex;
        std::optional<size_t> PointBIndex;

        RopeSegment()
            : PointAIndex(std::nullopt)
            , PointBIndex(std::nullopt)
        {
        }
    };

    std::map<std::array<uint8_t, 3u>, RopeSegment> ropeSegments;


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
                    // Store in RopeSegments
                    RopeSegment & ropeSegment = ropeSegments[rgbColour];
                    if (!ropeSegment.PointAIndex)
                    {
                        ropeSegment.PointAIndex = pointInfos.size();
                    }
                    else if (!ropeSegment.PointBIndex)
                    {
                        ropeSegment.PointBIndex = pointInfos.size();
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
    // 2. Process all identified rope endpoints and:
    // - Fill-in points between the endpoints, creating additional PointInfo's for them
    // - Fill-in springs between each pair of points in the rope, creating SpringInfo's for them
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

    // Visit all RopeSegment's
    for (auto const & ropeSegmentEntry : ropeSegments)
    {
        auto const & ropeSegment = ropeSegmentEntry.second;

        // Make sure we've got both endpoints
        assert(!!ropeSegment.PointAIndex);
        if (!ropeSegment.PointBIndex)
        {
            throw GameException(
                std::string("Only one rope endpoint found with index <")
                + std::to_string(static_cast<int>(ropeSegmentEntry.first[1]))
                + "," + std::to_string(static_cast<int>(ropeSegmentEntry.first[2])) + ">");
        }

        // Get endpoint positions
        vec2f startPos = pointInfos[*ropeSegment.PointAIndex].Position;
        vec2f endPos = pointInfos[*ropeSegment.PointBIndex].Position;

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

        size_t curStartPointIndex = *ropeSegment.PointAIndex;
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
        springInfos.emplace_back(curStartPointIndex, *ropeSegment.PointBIndex);
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
    //  - Set non-fully-surrounded Points as "leaking"
    //  - Detect springs and create SpringInfo's for them (additional to ropes)
    //  - Do tessellation and create TriangleInfo's
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
