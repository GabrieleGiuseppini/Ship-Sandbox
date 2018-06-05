/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RotatedRectangle.h"
#include "Vectors.h"

#include <algorithm>
#include <cmath>
#include <limits>

/*
 * Contains all the information necessary to render a rotated texture.
 */
struct RotatedTextureRenderInfo
{
    vec2f CenterPosition;
    float Scale;
    vec2f RotationBaseAxis;
    vec2f RotationOffsetAxis;

    RotatedTextureRenderInfo(
        vec2f const & centerPosition,
        float scale,
        vec2f const & rotationBaseAxis,
        vec2f const & rotationOffsetAxis)
        : CenterPosition(centerPosition)
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
        float cosAlpha = d > 0.0f
            ? RotationBaseAxis.dot(RotationOffsetAxis) / d
            : 0.0f;
        float sinAlpha = cosAlpha < 1.0f
            ? sqrtf(1.0f - cosAlpha * cosAlpha)
            : 0.0f;

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
