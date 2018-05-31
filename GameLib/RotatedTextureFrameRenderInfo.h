/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RotatedRectangle.h"
#include "Vectors.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>

/*
 * Contains all the information necessary to render a rotated frame of a texture.
 */
struct RotatedTextureFrameRenderInfo
{
    uint32_t FrameIndex;
    vec2f CenterPosition;
    float Scale;
    vec2f RotationBaseAxis;
    vec2f RotationOffsetAxis;

    RotatedTextureFrameRenderInfo(
        uint32_t frameIndex,
        vec2f const & centerPosition,
        float scale,
        vec2f const & rotationBaseAxis,
        vec2f const & rotationOffsetAxis)
        : FrameIndex(frameIndex)
        , CenterPosition(centerPosition)
        , Scale(scale)
        , RotationBaseAxis(rotationBaseAxis)
        , RotationOffsetAxis(rotationOffsetAxis)
    {}

    RotatedRectangle CalculateRotatedRectangle(
        float textureWidth,
        float textureHeight) const
    {
        //
        // Calculate rotation matrix, based off cosine of the angle between rotation offset and rotation base
        //

        float d = sqrtf(RotationBaseAxis.squareLength() * RotationOffsetAxis.squareLength());
        float cosAlpha = RotationBaseAxis.dot(RotationOffsetAxis) / std::max(d, std::numeric_limits<float>::min());
        float sinAlpha = sqrtf(1.0f - cosAlpha * cosAlpha);

        // First column
        vec2f rotationMatrixX(cosAlpha, sinAlpha);

        // Second column
        vec2f rotationMatrixY(-sinAlpha, cosAlpha);


        //
        // Calculate rectangle vertices
        //
        
        float leftX = -textureWidth * Scale / 2.0f;
        float rightX = textureWidth * Scale / 2.0f;
        float topY = -textureHeight * Scale / 2.0f;
        float bottomY = textureHeight * Scale / 2.0f;

        vec2f topLeft{ leftX, topY };
        vec2f topRight{ rightX, topY };
        vec2f bottomLeft{ leftX, bottomY };
        vec2f bottomRight{ rightX, bottomY };

        return RotatedRectangle(
            { topLeft.dot(rotationMatrixX) + CenterPosition.x, topLeft.dot(rotationMatrixY) + CenterPosition.y },
            { topRight.dot(rotationMatrixX) + CenterPosition.x, topRight.dot(rotationMatrixY) + CenterPosition.y },
            { bottomLeft.dot(rotationMatrixX) + CenterPosition.x, bottomLeft.dot(rotationMatrixY) + CenterPosition.y },
            { bottomRight.dot(rotationMatrixX) + CenterPosition.x, bottomRight.dot(rotationMatrixY) + CenterPosition.y });
    }
};
