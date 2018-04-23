/***************************************************************************************
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

using namespace Physics;

//////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Ship> ShipBuilder::Create(
    int shipId,
    World * parentWorld,
    ShipDefinition const & shipDefinition,
    MaterialDatabase const & materials,
    GameParameters const & /*gameParameters*/,
    uint64_t currentStepSequenceNumber)
{
    int const structureWidth = shipDefinition.StructuralImage.Size.Width;
    float const halfWidth = static_cast<float>(structureWidth) / 2.0f;
    int const structureHeight = shipDefinition.StructuralImage.Size.Height;

    // PointInfo's
    std::vector<PointInfo> pointInfos;

    // SpringInfo's
    std::vector<SpringInfo> springInfos;

    // RopeSegment's, indexed by the rope color
    std::map<std::array<uint8_t, 3u>, RopeSegment> ropeSegments;

    // TriangleInfo's
    std::vector<TriangleInfo> triangleInfos;


    //
    // 1. Process image points and:
    // - Identify all points, and create PointInfo's for them
    // - Build a 2D matrix containing indices to the points above
    // - Identify rope endpoints, and create RopeSegment's for them
    //    

    // Matrix of points - we allocate 2 extra dummy rows and cols to avoid checking for boundaries
    std::unique_ptr<std::unique_ptr<std::optional<size_t>[]>[]> pointIndexMatrix(new std::unique_ptr<std::optional<size_t>[]>[structureWidth + 2]);    
    for (int c = 0; c < structureWidth + 2; ++c)
    {
        pointIndexMatrix[c] = std::unique_ptr<std::optional<size_t>[]>(new std::optional<size_t>[structureHeight + 2]);
    }

    // Visit all real columns
    for (int x = 0; x < structureWidth; ++x)
    {
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



    //
    // 2. Process all identified rope endpoints and:
    // - Fill-in points between the endpoints, creating additional PointInfo's for them
    // - Fill-in springs between each pair of points in the rope, creating SpringInfo's for them
    //    

    CreateRopes(
        ropeSegments,
        shipDefinition.StructuralImage.Size,
        materials.GetRopeMaterial(),
        pointInfos,
        springInfos);


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

    CreatePoints(
        pointInfos,
        ship,
        allPoints,
        allPointColors,
        allPointTextureCoordinates);


    //
    // 4. Visit point matrix and:
    //  - Set non-fully-surrounded Points as "leaking"
    //  - Detect springs and create SpringInfo's for them (additional to ropes)
    //  - Do tessellation and create TriangleInfo's
    //

    size_t leakingPointsCount;

    CreateShipElementInfos(
        pointIndexMatrix,
        shipDefinition.StructuralImage.Size,
        allPoints,
        springInfos,
        triangleInfos,
        leakingPointsCount);


    //
    // 5. Optimize order of SpringInfo's to minimize cache misses
    //

    float originalSpringACMR = CalculateACMR(springInfos);

    springInfos = ReorderOptimally(springInfos, pointInfos.size());

    float optimizedSpringACMR = CalculateACMR(springInfos);

    LogMessage("Spring ACMR: original=", originalSpringACMR, ", optimized=", optimizedSpringACMR);


    //
    // 6. Optimize order of TriangleInfo's to minimize cache misses
    //
    // Applies to both GPU and CPU!
    //

    float originalTriangleACMR = CalculateACMR(triangleInfos);

    // TODOTEST
    //triangleInfos = ReorderOptimally(triangleInfos, pointInfos.size());

    float optimizedTriangleACMR = CalculateACMR(triangleInfos);

    LogMessage("Triangle ACMR: original=", originalTriangleACMR, ", optimized=", optimizedTriangleACMR);


    //
    // 7. Create Springs for all SpringInfo's
    //

    ElementRepository<Spring> allSprings = CreateSprings(
        springInfos,
        ship,
        allPoints);


    //
    // 8. Create Triangles for all TriangleInfo's except those whose vertices
    //    are all rope points, of which at least one is connected exclusively 
    //    to rope points (these would be knots "sticking out" of the structure)
    //

    ElementRepository<Triangle> allTriangles = CreateTriangles(
        triangleInfos,
        ship,
        allPoints);


    //
    // 9. Create Electrical Elements
    //

    std::vector<ElectricalElement*> allElectricalElements = CreateElectricalElements(
        allPoints,
        ship);


    //
    // We're done!
    //

    LogMessage("Created ship: W=", shipDefinition.StructuralImage.Size.Width, ", H=", shipDefinition.StructuralImage.Size.Height, ", ",
        allPoints.size(), " points, ", allSprings.size(), " springs, ", allTriangles.size(), " triangles, ", 
        allElectricalElements.size(), " electrical elements.");

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

//////////////////////////////////////////////////////////////////////////////////////////////////
// Building helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

void ShipBuilder::CreateRopes(
    std::map<std::array<uint8_t, 3u>, RopeSegment> const & ropeSegments,
    ImageSize const & structureImageSize,
    Material const & ropeMaterial,
    std::vector<PointInfo> & pointInfos,
    std::vector<SpringInfo> & springInfos)
{
    //
    // - Fill-in points between each pair of endpoints, creating additional PointInfo's for them
    // - Fill-in springs between each pair of points in the rope, creating SpringInfo's for them
    //    

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
                    newPosition.x / static_cast<float>(structureImageSize.Width),
                    newPosition.y / static_cast<float>(structureImageSize.Height)),
                &ropeMaterial);
        }

        // Add last SpringInfo (no PointInfo as the endpoint has already a PointInfo)
        springInfos.emplace_back(curStartPointIndex, *ropeSegment.PointBIndex);
    }
}

void ShipBuilder::CreatePoints(
    std::vector<PointInfo> const & pointInfos,
    Physics::Ship * ship,
    ElementRepository<Physics::Point> & points,
    ElementRepository<vec3f> & pointColors,
    ElementRepository<vec2f> & pointTextureCoordinates)
{
    assert(points.max_size() == pointInfos.size());
    assert(pointColors.max_size() == pointInfos.size());
    assert(pointTextureCoordinates.max_size() == pointInfos.size());

    for (size_t p = 0; p < pointInfos.size(); ++p)
    {
        PointInfo const & pointInfo = pointInfos[p];

        Material const * mtl = pointInfo.Mtl;

        //
        // Create point
        //

        points.emplace_back(
            ship,
            pointInfo.Position,
            mtl,
            mtl->IsHull ? 0.0f : 1.0f, // No buoyancy if it's a hull point, as it can't get water
            static_cast<int>(p));

        //
        // Create point color
        //

        pointColors.emplace_back(mtl->RenderColour);


        //
        // Create point texture coordinates
        //

        pointTextureCoordinates.emplace_back(pointInfo.TextureCoordinates);
    }
}

void ShipBuilder::CreateShipElementInfos(
    std::unique_ptr<std::unique_ptr<std::optional<size_t>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize,
    ElementRepository<Point> & points,
    std::vector<SpringInfo> & springInfos,
    std::vector<TriangleInfo> & triangleInfos,
    size_t & leakingPointsCount)
{
    //
    // Visit point matrix and:
    //  - Set non-fully-surrounded Points as "leaking"
    //  - Detect springs and create SpringInfo's for them (additional to ropes)
    //  - Do tessellation and create TriangleInfo's
    //

    // Initialize count of leaking points
    leakingPointsCount = 0;

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
    for (int y = 1; y <= structureImageSize.Height; ++y)
    {
        // We're starting a new row, so we're not in a ship now
        bool isInShip = false;

        for (int x = 1; x <= structureImageSize.Width; ++x)
        {
            if (!!pointIndexMatrix[x][y])
            {
                //
                // A point exists at these coordinates
                //

                size_t pointIndex = *pointIndexMatrix[x][y];
                Point & point = points[pointIndex];

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
                        if ((!isInShip || i < 2)
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
}

ElementRepository<Spring> ShipBuilder::CreateSprings(
    std::vector<SpringInfo> const & springInfos,
    Physics::Ship * ship,
    ElementRepository<Point> & points)
{
    ElementRepository<Spring> allSprings(springInfos.size());

    for (SpringInfo const & springInfo : springInfos)
    {
        Point * a = &(points[springInfo.PointAIndex]);
        Point * b = &(points[springInfo.PointBIndex]);

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

    return allSprings;
}

ElementRepository<Physics::Triangle> ShipBuilder::CreateTriangles(
    std::vector<TriangleInfo> const & triangleInfos,
    Physics::Ship * ship,
    ElementRepository<Physics::Point> & points)
{
    ElementRepository<Triangle> allTriangles(triangleInfos.size());

    for (TriangleInfo const & triangleInfo : triangleInfos)
    {
        Point * a = &(points[triangleInfo.PointAIndex]);
        Point * b = &(points[triangleInfo.PointBIndex]);
        Point * c = &(points[triangleInfo.PointCIndex]);

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

    return allTriangles;
}

std::vector<ElectricalElement*> ShipBuilder::CreateElectricalElements(
    ElementRepository<Physics::Point> & points,
    Physics::Ship * ship)
{
    std::vector<ElectricalElement*> allElectricalElements;

    for (Point & point : points)
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

    return allElectricalElements;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex cache optimization
//////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ShipBuilder::SpringInfo> ShipBuilder::ReorderOptimally(
    std::vector<SpringInfo> & springInfos,
    size_t vertexCount)
{
    // TODOHERE
    return springInfos;
}

std::vector<ShipBuilder::TriangleInfo> ShipBuilder::ReorderOptimally(
    std::vector<TriangleInfo> & triangleInfos,
    size_t vertexCount)
{
    //
    // Initialization
    //

    std::vector<VertexData> vertexData(vertexCount);
    std::vector<ElementData> elementData(triangleInfos.size());

    // Fill-in cross-references between vertices and elements
    for (size_t t = 0; t < triangleInfos.size(); ++t)
    {
        vertexData[triangleInfos[t].PointAIndex].RemainingElementIndices.push_back(t);
        vertexData[triangleInfos[t].PointBIndex].RemainingElementIndices.push_back(t);
        vertexData[triangleInfos[t].PointCIndex].RemainingElementIndices.push_back(t);

        elementData[t].VertexIndices.push_back(triangleInfos[t].PointAIndex);
        elementData[t].VertexIndices.push_back(triangleInfos[t].PointBIndex);
        elementData[t].VertexIndices.push_back(triangleInfos[t].PointCIndex);
    }

    // Calculate vertex scores
    for (VertexData & v : vertexData)
    {
        v.CurrentScore = CalculateVertexScore<3>(v);
    }

    // Calculate element scores, remembering best so far
    float bestElementScore = std::numeric_limits<float>::lowest();
    std::optional<size_t> bestElementIndex(std::nullopt);
    for (size_t ei = 0; ei < elementData.size(); ++ei)
    {
        for (size_t vi : elementData[ei].VertexIndices)
        {
            elementData[ei].CurrentScore += vertexData[vi].CurrentScore;
        }

        if (elementData[ei].CurrentScore > bestElementScore)
        {
            bestElementScore = elementData[ei].CurrentScore;
            bestElementIndex = ei;
        }
    }


    //
    // Main loop - run until we've drawn all triangles
    //

    std::list<size_t> modelLruVertexCache;

    std::vector<size_t> optimalIndices;
    optimalIndices.reserve(triangleInfos.size());

    while (optimalIndices.size() < triangleInfos.size())
    {
        //
        // Find best element
        //

        if (!bestElementIndex)
        {
            // Have to find best element
            bestElementScore = std::numeric_limits<float>::lowest();
            for (size_t ei = 0; ei < elementData.size(); ++ei)
            {
                if (!elementData[ei].HasBeenDrawn
                    && elementData[ei].CurrentScore > bestElementScore)
                {
                    bestElementScore = elementData[ei].CurrentScore > bestElementScore;
                    bestElementIndex = ei;
                }
            }
        }

        assert(!!bestElementIndex);
        assert(!elementData[*bestElementIndex].HasBeenDrawn);

        // Add the best element to the optimal list
        optimalIndices.push_back(*bestElementIndex);

        // Mark the best element as drawn
        elementData[*bestElementIndex].HasBeenDrawn = true;

        // Update all of the element's vertices
        for (auto vi : elementData[*bestElementIndex].VertexIndices)
        {
            // Remove the best element element from the lists of remaining elements for this vertex
            vertexData[vi].RemainingElementIndices.erase(
                std::remove(
                    vertexData[vi].RemainingElementIndices.begin(),
                    vertexData[vi].RemainingElementIndices.end(),
                    *bestElementIndex));

            // Update the LRU cache with this vertex
            AddVertexToCache(vi, modelLruVertexCache);
        }

        // Re-assign positions and scores of all vertices in the cache
        int32_t currentCachePosition = 0;
        for (auto it = modelLruVertexCache.begin(); it != modelLruVertexCache.end(); ++it, ++currentCachePosition)
        {
            vertexData[*it].CachePosition = (currentCachePosition < VertexCacheSize)
                ? currentCachePosition
                : -1;

            vertexData[*it].CurrentScore = CalculateVertexScore<3>(vertexData[*it]);

            // Zero the score of this vertices' elements, as we'll be updating it next
            for (size_t ei : vertexData[*it].RemainingElementIndices)
            {
                elementData[ei].CurrentScore = 0.0f;
            }
        }

        // Update scores of all triangles in the cache, maintaining best score at the same time
        bestElementScore = std::numeric_limits<float>::lowest();
        bestElementIndex = std::nullopt;
        for (size_t vi : modelLruVertexCache)
        {
            for (size_t ei : vertexData[vi].RemainingElementIndices)
            {
                assert(!elementData[ei].HasBeenDrawn);

                // Add this vertex's score to the element's score
                elementData[ei].CurrentScore += vertexData[vi].CurrentScore;

                // Check if best so far
                if (elementData[ei].CurrentScore > bestElementScore)
                {
                    bestElementScore = elementData[ei].CurrentScore;
                    bestElementIndex = ei;
                }
            }
        }

        // Shrink cache back to its size
        if (modelLruVertexCache.size() > VertexCacheSize)
        {
            modelLruVertexCache.resize(VertexCacheSize);
        }
    }


    //
    // Return optimally-ordered set of triangles
    //

    std::vector<TriangleInfo> newTriangleInfos;
    newTriangleInfos.reserve(triangleInfos.size());
    for (size_t ti : optimalIndices)
    {
        newTriangleInfos.push_back(triangleInfos[ti]);
    }

    return newTriangleInfos;
}

float ShipBuilder::CalculateACMR(std::vector<SpringInfo> const & springInfos)
{
    //
    // Calculate the average cache miss ratio
    //

    if (springInfos.empty())
    {
        return 0.0f;
    }

    TestLRUVertexCache<VertexCacheSize> cache;

    float cacheMisses = 0.0f;

    for (auto const & springInfo : springInfos)
    {
        if (!cache.UseVertex(springInfo.PointAIndex))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(springInfo.PointBIndex))
        {
            cacheMisses += 1.0f;
        }
    }

    return cacheMisses / static_cast<float>(springInfos.size());
}

float ShipBuilder::CalculateACMR(std::vector<TriangleInfo> const & triangleInfos)
{
    //
    // Calculate the average cache miss ratio
    //

    if (triangleInfos.empty())
    {
        return 0.0f;
    }

    TestLRUVertexCache<VertexCacheSize> cache;

    float cacheMisses = 0.0f;

    for (auto const & triangleInfo : triangleInfos)
    {
        if (!cache.UseVertex(triangleInfo.PointAIndex))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(triangleInfo.PointBIndex))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(triangleInfo.PointCIndex))
        {
            cacheMisses += 1.0f;
        }
    }

    return cacheMisses / static_cast<float>(triangleInfos.size());
}

void ShipBuilder::AddVertexToCache(
    size_t vertexIndex,
    ModelLRUVertexCache & cache)
{
    for (auto it = cache.begin(); it != cache.end(); ++it)
    {
        if (vertexIndex == *it)
        {
            // It's already in the cache...
            // ...move it to front
            cache.erase(it);
            cache.push_front(vertexIndex);

            return;
        }
    }

    // Not in the cache...
    // ...insert in front of cache
    cache.push_front(vertexIndex);
}

template <size_t VerticesInElement>
float ShipBuilder::CalculateVertexScore(VertexData const & vertexData)
{
    static_assert(VerticesInElement < VertexCacheSize);

    //
    // Almost verbatim from Tom Forsyth
    //

    static constexpr float FindVertexScore_CacheDecayPower = 1.5f;
    static constexpr float FindVertexScore_LastElementScore = 0.75f;
    static constexpr float FindVertexScore_ValenceBoostScale = 2.0f;
    static constexpr float FindVertexScore_ValenceBoostPower = 0.5f;        

    if (vertexData.RemainingElementIndices.size() == 0)
    {
        // No elements left using this vertex, give it a bad score
        return -1.0f;
    }

    float score = 0.0f;
    if (vertexData.CachePosition >= 0)
    {
        // This vertex is in the cache

        if (vertexData.CachePosition < VerticesInElement)
        {
            // This vertex was used in the last element,
            // so it has a fixed score, whichever of the vertices
            // it is. Otherwise, you can get very different
            // answers depending on whether you add, for example,
            // a triangle's 1,2,3 or 3,1,2 - which is silly.
            score = FindVertexScore_LastElementScore;
        }
        else
        {
            assert(vertexData.CachePosition < VertexCacheSize);

            // Score vertices high for being high in the cache
            float const scaler = 1.0f / (VertexCacheSize - VerticesInElement);
            score = 1.0f - (vertexData.CachePosition - VerticesInElement) * scaler;
            score = powf(score, FindVertexScore_CacheDecayPower);
        }
    }

    // Bonus points for having a low number of elements still 
    // using this vertex, so we get rid of lone vertices quickly
    float valenceBoost = powf(
        static_cast<float>(vertexData.RemainingElementIndices.size()),
        -FindVertexScore_ValenceBoostPower);
    score += FindVertexScore_ValenceBoostScale * valenceBoost;

    return score;
}

template<size_t Size>
bool ShipBuilder::TestLRUVertexCache<Size>::UseVertex(size_t vertexIndex)
{
    for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
    {
        if (vertexIndex == *it)
        {
            // It's already in the cache...
            // ...move it to front
            mEntries.erase(it);
            mEntries.push_front(vertexIndex);

            // It was a cache hit
            return true;
        }
    }

    // Not in the cache...
    // ...insert in front of cache
    mEntries.push_front(vertexIndex);

    // Trim
    while (mEntries.size() > Size)
    {
        mEntries.pop_back();
    }

    // It was a cache miss
    return false;
}

template<size_t Size>
std::optional<size_t> ShipBuilder::TestLRUVertexCache<Size>::GetCachePosition(size_t vertexIndex)
{
    size_t position = 0;
    for (auto const & vi : mEntries)
    {
        if (vi == vertexIndex)
        {
            // Found!
            return position;
        }

        ++position;
    }

    // Not found
    return std::nullopt;
}
