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

    // See Tom's comments: using 32 is good enough; apparently 64 does not yield significant differences
    static constexpr size_t VertexCacheSize = 32;

    using ModelLRUVertexCache = std::list<size_t>;

    struct VertexData
    {
        int32_t CachePosition;                          // Position in cache; -1 if not in cache
        float CurrentScore;                             // Current score of the vertex
        std::vector<size_t> RemainingElementIndices;    // Indices of not yet drawn elements that use this vertex

        VertexData()
            : CachePosition(-1)
            , CurrentScore(0.0f)
            , RemainingElementIndices()
        {
        }
    };

    struct ElementData
    {
        bool HasBeenDrawn;                  // Set to true when the element has been drawn already
        float CurrentScore;                 // Current score of the element - sum of its vertices' scores
        std::vector<size_t> VertexIndices;  // Indices of vertices in this element

        ElementData()
            : HasBeenDrawn(false)
            , CurrentScore(0.0f)
            , VertexIndices()
        {
        }
    };

    static std::vector<SpringInfo> ReorderOptimally(
        std::vector<SpringInfo> & springInfos,
        size_t vertexCount);

    static std::vector<TriangleInfo> ReorderOptimally(
        std::vector<TriangleInfo> & triangleInfos,
        size_t vertexCount);

    template <size_t VerticesInElement>
    static std::vector<size_t> ReorderOptimally(
        std::vector<VertexData> & vertexData,
        std::vector<ElementData> & elementData);


    static float CalculateACMR(std::vector<SpringInfo> const & springInfos);

    static float CalculateACMR(std::vector<TriangleInfo> const & triangleInfos);

    static void AddVertexToCache(
        size_t vertexIndex,
        ModelLRUVertexCache & cache);

    template <size_t VerticesInElement>
    static float CalculateVertexScore(VertexData const & vertexData);

    template <size_t Size>
    class TestLRUVertexCache
    {
    public:

        bool UseVertex(size_t vertexIndex);

        std::optional<size_t> GetCachePosition(size_t vertexIndex);

    private:

        std::list<size_t> mEntries;
    };    
};
