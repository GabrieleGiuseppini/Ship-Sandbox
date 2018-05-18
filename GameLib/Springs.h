/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"
#include "FixedSizeVector.h"
#include "GameParameters.h"
#include "IGameEventHandler.h"
#include "Material.h"
#include "RenderContext.h"

#include <cassert>

namespace Physics
{

class Springs : public ElementContainer
{
public:

    enum class Characteristics : uint8_t
    {
        None = 0,
        Hull = 1,    // Does not take water
        Rope = 2     // Ropes are drawn differently
    };

private:

    /*
     * The endpoints of a spring.
     */
    struct Endpoints
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;
        
        Endpoints(
            ElementIndex pointAIndex,
            ElementIndex pointBIndex)
            : PointAIndex(pointAIndex)
            , PointBIndex(pointBIndex)
        {}
    };

    /*
     * The coefficients used for the spring dynamics.
     */
    struct Coefficients
    {
        float StiffnessCoefficient;
        float DampingCoefficient;
        
        Coefficients(
            float stiffnessCoefficient,
            float dampingCoefficient)
            : StiffnessCoefficient(stiffnessCoefficient)
            , DampingCoefficient(dampingCoefficient)
        {}
    };

public:

    Springs(ElementCount elementCount)
        : ElementContainer(elementCount)
        , mIsDeletedBuffer(elementCount)
        // Endpoints
        , mEndpointsBuffer(elementCount)
        // Physical characteristics
        , mRestLengthBuffer(elementCount)
        , mCoefficientsBuffer(elementCount)
        , mCharacteristicsBuffer(elementCount)
        , mMaterialBuffer(elementCount)
        // Water characteristics
        , mWaterPermeabilityBuffer(elementCount)
        // Stress
        , mIsStressedBuffer(elementCount)
    {
    }

    Springs(Springs && other) = default;

    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        Characteristics characteristics,
        Material const * material,
        Points const & points);

    void Destroy(
        ElementIndex springElementIndex,        
        ElementIndex sourcePointElementIndex,
        Points & points,
        Triangles & triangles);

    void UploadElements(
        int shipId,
        RenderContext & renderContext,
        Points const & points) const;

    void UploadStressedSpringElements(
        int shipId,
        RenderContext & renderContext,
        Points const & points) const;

    /*
     * Calculates the current strain - due to tension or compression - and acts depending on it.
     *
     * Returns true if the spring got broken.
     */
    bool UpdateStrains(
        GameParameters const & gameParameters,
        World const & parentWorld,
        IGameEventHandler & gameEventHandler,
        Points & points,
        Triangles & triangles);

public:

    //
    // IsDeleted
    //

    inline bool IsDeleted(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mIsDeletedBuffer[springElementIndex];
    }

    //
    // Endpoints
    //

    inline ElementIndex GetPointAIndex(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mEndpointsBuffer[springElementIndex].PointAIndex;
    }

    inline ElementIndex GetPointBIndex(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mEndpointsBuffer[springElementIndex].PointBIndex;
    }

    //
    // Physical characteristics
    //

    inline float GetRestLength(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mRestLengthBuffer[springElementIndex];
    }

    inline float GetStiffnessCoefficient(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mCoefficientsBuffer[springElementIndex].StiffnessCoefficient;
    }

    inline float GetDampingCoefficient(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mCoefficientsBuffer[springElementIndex].DampingCoefficient;
    }

    inline bool IsHull(ElementIndex springElementIndex) const;

    inline bool IsRope(ElementIndex springElementIndex) const;

    inline Material const * GetMaterial(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mMaterialBuffer[springElementIndex];
    }

    //
    // Water characteristics
    //

    inline float GetWaterPermeability(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mWaterPermeabilityBuffer[springElementIndex];
    }
    
private:

    static float CalculateStiffnessCoefficient(        
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        float springStiffness,
        Points const & points);

    static float CalculateDampingCoefficient(        
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        Points const & points);

private:

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Endpoints
    Buffer<Endpoints> mEndpointsBuffer;

    //
    // Physical characteristics
    //

    Buffer<float> mRestLengthBuffer;
    Buffer<Coefficients> mCoefficientsBuffer;
    Buffer<Characteristics> mCharacteristicsBuffer;
    Buffer<Material const *> mMaterialBuffer;

    //
    // Water characteristics
    //

    // Water propagates through this spring according to this value;
    // 0.0 make water not propagate
    Buffer<float> mWaterPermeabilityBuffer;

    //
    // Stress
    //

    // State variable that tracks when we enter and exit the stressed state
    Buffer<bool> mIsStressedBuffer;
};

}

constexpr Physics::Springs::Characteristics operator&(Physics::Springs::Characteristics a, Physics::Springs::Characteristics b)
{
    return static_cast<Physics::Springs::Characteristics>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool Physics::Springs::IsHull(ElementContainer::ElementIndex springElementIndex) const
{
    assert(springElementIndex < mElementCount);

    return Physics::Springs::Characteristics::None != (mCharacteristicsBuffer[springElementIndex] & Physics::Springs::Characteristics::Hull);
}

inline bool Physics::Springs::IsRope(ElementContainer::ElementIndex springElementIndex) const
{
    assert(springElementIndex < mElementCount);

    return Physics::Springs::Characteristics::None != (mCharacteristicsBuffer[springElementIndex] & Physics::Springs::Characteristics::Rope);
}
