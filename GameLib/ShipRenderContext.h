/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "ImageData.h"
#include "RenderTypes.h"
#include "Vectors.h"

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ShipRenderContext
{
public:

    ShipRenderContext(
        std::optional<ImageData> texture,
        vec3f const & ropeColour);
    
    ~ShipRenderContext();

public:

    //
    // Points
    //

    void UploadPointVisualAttributes(
        vec3f const * colors, 
        vec2f const * textureCoordinates,
        size_t pointCount);

    void UploadPointsStart(size_t maxPoints);

    inline void UploadPoint(
        float x,
        float y,
        float light,
        float water)
    {
        assert(mPointBufferSize + 1u <= mPointBufferMaxSize);
        ShipRenderContext::Point * point = &(mPointBuffer[mPointBufferSize]);

        point->x = x;
        point->y = y;
        point->light = light;
        point->water = water;

        ++mPointBufferSize;
    }

    void UploadPointsEnd();


    //
    // Springs and triangles
    //

    void UploadElementsStart(std::vector<std::size_t> const & connectedComponentsMaxSizes);

    inline void UploadElementPoint(
        int pointIndex,
        size_t connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].pointElementCount + 1u <= mConnectedComponents[connectedComponentIndex].pointElementMaxCount);

        PointElement * const pointElement = &(mConnectedComponents[connectedComponentIndex].pointElementBuffer[mConnectedComponents[connectedComponentIndex].pointElementCount]);

        pointElement->pointIndex = pointIndex;

        ++(mConnectedComponents[connectedComponentIndex].pointElementCount);
    }

    inline void UploadElementSpring(
        int pointIndex1,
        int pointIndex2,
        size_t connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].springElementCount + 1u <= mConnectedComponents[connectedComponentIndex].springElementMaxCount);

        SpringElement * const springElement = &(mConnectedComponents[connectedComponentIndex].springElementBuffer[mConnectedComponents[connectedComponentIndex].springElementCount]);

        springElement->pointIndex1 = pointIndex1;
        springElement->pointIndex2 = pointIndex2;

        ++(mConnectedComponents[connectedComponentIndex].springElementCount);
    }

    inline void UploadElementRope(
        int pointIndex1,
        int pointIndex2,
        size_t connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].ropeElementCount + 1u <= mConnectedComponents[connectedComponentIndex].ropeElementMaxCount);

        RopeElement * const ropeElement = &(mConnectedComponents[connectedComponentIndex].ropeElementBuffer[mConnectedComponents[connectedComponentIndex].ropeElementCount]);

        ropeElement->pointIndex1 = pointIndex1;
        ropeElement->pointIndex2 = pointIndex2;

        ++(mConnectedComponents[connectedComponentIndex].ropeElementCount);
    }

    inline void UploadElementTriangle(
        int pointIndex1,
        int pointIndex2,
        int pointIndex3,
        size_t connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].triangleElementCount + 1u <= mConnectedComponents[connectedComponentIndex].triangleElementMaxCount);

        TriangleElement * const triangleElement = &(mConnectedComponents[connectedComponentIndex].triangleElementBuffer[mConnectedComponents[connectedComponentIndex].triangleElementCount]);

        triangleElement->pointIndex1 = pointIndex1;
        triangleElement->pointIndex2 = pointIndex2;
        triangleElement->pointIndex3 = pointIndex3;

        ++(mConnectedComponents[connectedComponentIndex].triangleElementCount);
    }

    void UploadElementsEnd();


    void UploadElementStressedSpringsStart();

    inline void UploadElementStressedSpring(
        int pointIndex1,
        int pointIndex2,
        size_t connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mConnectedComponents.size());
        assert(mConnectedComponents[connectedComponentIndex].stressedSpringElementCount + 1u <= mConnectedComponents[connectedComponentIndex].stressedSpringElementMaxCount);

        StressedSpringElement * const stressedSpringElement = &(mConnectedComponents[connectedComponentIndex].stressedSpringElementBuffer[mConnectedComponents[connectedComponentIndex].stressedSpringElementCount]);

        stressedSpringElement->pointIndex1 = pointIndex1;
        stressedSpringElement->pointIndex2 = pointIndex2;

        ++(mConnectedComponents[connectedComponentIndex].stressedSpringElementCount);
    }

    void UploadElementStressedSpringsEnd();


    //
    // Lamps
    //

    void UploadLampsStart(size_t connectedComponents);

    void UploadLamp(
        float x,
        float y,
        float lightIntensity,
        size_t connectedComponentId)
    {
        size_t const connectedComponentIndex = connectedComponentId - 1;

        assert(connectedComponentIndex < mLampBuffers.size());

        LampElement & lampElement = mLampBuffers[connectedComponentIndex].emplace_back();

        lampElement.x = x;
        lampElement.y = y;
        lampElement.lightIntensity = lightIntensity;
    }

    void UploadLampsEnd();


    /////////////////////////////////////////////////////////////

    void Render(
        ShipRenderMode renderMode,
        bool showStressedSprings,
        float ambientLightIntensity,
        float canvasToVisibleWorldHeightRatio,
        float(&orthoMatrix)[4][4]);

private:

    struct ConnectedComponentData;

    void RenderPointElements(
        ConnectedComponentData const & connectedComponent,
        float ambientLightIntensity,
        float canvasToVisibleWorldHeightRatio,
        float(&orthoMatrix)[4][4]);

    void RenderSpringElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture,
        float ambientLightIntensity,
        float canvasToVisibleWorldHeightRatio,
        float(&orthoMatrix)[4][4]);

    void RenderRopeElements(
        ConnectedComponentData const & connectedComponent,
        float ambientLightIntensity,
        float canvasToVisibleWorldHeightRatio,
        float(&orthoMatrix)[4][4]);

    void RenderTriangleElements(
        ConnectedComponentData const & connectedComponent,
        bool withTexture,
        float ambientLightIntensity,
        float(&orthoMatrix)[4][4]);

    void RenderStressedSpringElements(
        ConnectedComponentData const & connectedComponent,
        float canvasToVisibleWorldHeightRatio,
        float(&orthoMatrix)[4][4]);

private:

    //
    // Points
    //

#pragma pack(push)
    struct Point
    {
        float x;
        float y;
        float light;
        float water;
    };
#pragma pack(pop)

    size_t mPointCount;
    std::unique_ptr<ShipRenderContext::Point[]> mPointBuffer;
    size_t mPointBufferSize;
    size_t mPointBufferMaxSize;

    GameOpenGLVBO mPointVBO;
    GameOpenGLVBO mPointColorVBO;
    GameOpenGLVBO mPointElementTextureCoordinatesVBO;
    
    static constexpr GLuint InputPosPosition = 0;
    static constexpr GLuint InputLightPosition = 1;
    static constexpr GLuint InputWaterPosition = 2;
    static constexpr GLuint InputColorPosition = 3;
    static constexpr GLuint InputTextureCoordinatesPosition = 4;


    //
    // Elements (points, springs, ropes, triangles, stressed springs)
    //

    GameOpenGLShaderProgram mElementColorShaderProgram;
    GLint mElementColorShaderOrthoMatrixParameter;
    GLint mElementColorShaderAmbientLightIntensityParameter;

    GameOpenGLShaderProgram mElementRopeShaderProgram;
    GLint mElementRopeShaderOrthoMatrixParameter;
    GLint mElementRopeShaderAmbientLightIntensityParameter;

    GameOpenGLShaderProgram mElementTextureShaderProgram;
    GLint mElementTextureShaderOrthoMatrixParameter;
    GLint mElementTextureShaderAmbientLightIntensityParameter;

    GameOpenGLShaderProgram mElementStressedSpringShaderProgram;
    GLint mElementStressedSpringShaderOrthoMatrixParameter;

#pragma pack(push)
    struct PointElement
    {
        int pointIndex;
    };
#pragma pack(pop)

#pragma pack(push)
    struct SpringElement
    {
        int pointIndex1;
        int pointIndex2;
    };
#pragma pack(pop)

#pragma pack(push)
    struct RopeElement
    {
        int pointIndex1;
        int pointIndex2;
    };
#pragma pack(pop)

#pragma pack(push)
    struct TriangleElement
    {
        int pointIndex1;
        int pointIndex2;
        int pointIndex3;
    };
#pragma pack(pop)

#pragma pack(push)
    struct StressedSpringElement
    {
        int pointIndex1;
        int pointIndex2;
    };
#pragma pack(pop)

    /*
     * All the data that belongs to a single connected component.
     */
    struct ConnectedComponentData
    {
        size_t pointElementCount;
        size_t pointElementMaxCount;
        std::unique_ptr<PointElement[]> pointElementBuffer;
        GameOpenGLVBO pointElementVBO;

        size_t springElementCount;
        size_t springElementMaxCount;
        std::unique_ptr<SpringElement[]> springElementBuffer;
        GameOpenGLVBO springElementVBO;

        size_t ropeElementCount;
        size_t ropeElementMaxCount;
        std::unique_ptr<RopeElement[]> ropeElementBuffer;
        GameOpenGLVBO ropeElementVBO;

        size_t triangleElementCount;
        size_t triangleElementMaxCount;
        std::unique_ptr<TriangleElement[]> triangleElementBuffer;
        GameOpenGLVBO triangleElementVBO;

        size_t stressedSpringElementCount;
        size_t stressedSpringElementMaxCount;
        std::unique_ptr<StressedSpringElement[]> stressedSpringElementBuffer;
        GameOpenGLVBO stressedSpringElementVBO;
        
        ConnectedComponentData()
            : pointElementCount(0)
            , pointElementMaxCount(0)
            , pointElementBuffer()
            , pointElementVBO()
            , springElementCount(0)
            , springElementMaxCount(0)
            , springElementBuffer()
            , springElementVBO()
            , ropeElementCount(0)
            , ropeElementMaxCount(0)
            , ropeElementBuffer()
            , ropeElementVBO()
            , triangleElementCount(0)
            , triangleElementMaxCount(0)
            , triangleElementBuffer()
            , triangleElementVBO()
            , stressedSpringElementCount(0)
            , stressedSpringElementMaxCount(0)
            , stressedSpringElementBuffer()
            , stressedSpringElementVBO()
        {}
    };

    std::vector<ConnectedComponentData> mConnectedComponents;

    GameOpenGLTexture mElementTexture;
    GameOpenGLTexture mElementStressedSpringTexture;

    //
    // Lamps
    //

#pragma pack(push)
    struct LampElement
    {
        float x;
        float y;
        float lightIntensity;
    };
#pragma pack(pop)

    std::vector<std::vector<LampElement>> mLampBuffers;
};
