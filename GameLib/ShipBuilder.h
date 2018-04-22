/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "ImageSize.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "ShipDefinition.h"

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

/*
 * This class contains all the logic for building a ship out of a ShipDefinition.
 */
class ShipBuilder
{
public:

    static std::unique_ptr<Physics::Ship> Create(
        int shipId,        
        Physics::World * parentWorld,
        ShipDefinition const & shipDefinition,
        MaterialDatabase const & materials,
        GameParameters const & gameParameters,
        uint64_t currentStepSequenceNumber);

private:

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

private:

    /////////////////////////////////////////////////////////////////
    // Building helpers
    /////////////////////////////////////////////////////////////////

    static void CreateRopes(
        std::map<std::array<uint8_t, 3u>, RopeSegment> const & ropeSegments,
        ImageSize const & structureImageSize,
        Material const & ropeMaterial,        
        std::vector<PointInfo> & pointInfos,
        std::vector<SpringInfo> & springInfos);

    static void CreatePoints(
        std::vector<PointInfo> const & pointInfos,
        Physics::Ship * ship,
        ElementRepository<Physics::Point> & points,
        ElementRepository<vec3f> & pointColors,
        ElementRepository<vec2f> & pointTextureCoordinates);

    static void CreateShipElementInfos(
        std::unique_ptr<std::unique_ptr<std::optional<size_t>[]>[]> const & pointIndexMatrix,
        ImageSize const & structureImageSize,
        ElementRepository<Physics::Point> & points,
        std::vector<SpringInfo> & springInfos,
        std::vector<TriangleInfo> & triangleInfos,
        size_t & leakingPointsCount);

    static ElementRepository<Physics::Spring> CreateSprings(
        std::vector<SpringInfo> const & springInfos,
        Physics::Ship * ship,
        ElementRepository<Physics::Point> & points);

    static ElementRepository<Physics::Triangle> CreateTriangles(
        std::vector<TriangleInfo> const & triangleInfos,
        Physics::Ship * ship,
        ElementRepository<Physics::Point> & points);

    static std::vector<Physics::ElectricalElement*> CreateElectricalElements(
        ElementRepository<Physics::Point> & points,
        Physics::Ship * ship);

private:

    /////////////////////////////////////////////////////////////////
    // Vertex cache optimization
    /////////////////////////////////////////////////////////////////

    static float CalculateACMR(std::vector<SpringInfo> const & springInfos);
    static float CalculateACMR(std::vector<TriangleInfo> const & triangleInfos);

    template <size_t Size>
    class LRUVertexCache
    {
    public:

        LRUVertexCache() 
            : mEntries() 
        {}

        bool UseVertex(size_t vertexIndex);

        std::optional<size_t> GetCachePosition(size_t vertexIndex);

    private:

        std::list<size_t> mEntries;
    };
};
