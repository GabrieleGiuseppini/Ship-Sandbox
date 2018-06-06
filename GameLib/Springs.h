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
#include <limits>

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
        // Bombs
        , mIsBombAttachedBuffer(elementCount)
        // Container state
        , mCurrentStiffnessAdjustment(std::numeric_limits<float>::lowest())
    {
    }

    Springs(Springs && other) = default;

    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        Characteristics characteristics,
        Material const * material,
        Points const & points);

    void Destroy(ElementIndex springElementIndex);

    void SetStiffnessAdjustment(
        float stiffnessAdjustment,
        Points const & points);

    inline void OnPointMassUpdated(
        ElementIndex springElementIndex,
        Points const & points)
    {
        assert(springElementIndex < mElementCount);

        CalculateStiffnessCoefficient(
            mEndpointsBuffer[springElementIndex].PointAIndex,
            mEndpointsBuffer[springElementIndex].PointBIndex,
            mMaterialBuffer[springElementIndex]->Stiffness,
            mCurrentStiffnessAdjustment,
            points);
    }

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
        Points & points);

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

    inline vec2f const & GetPointAPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        assert(springElementIndex < mElementCount);

        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointAIndex);
    }

    inline vec2f const & GetPointBPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        assert(springElementIndex < mElementCount);

        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointBIndex);
    }

    inline vec2f GetMidpointPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        assert(springElementIndex < mElementCount);

        return (GetPointAPosition(springElementIndex, points)
            + GetPointBPosition(springElementIndex, points)) / 2.0f;
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
    
    //
    // Bombs
    //

    inline bool IsBombAttached(ElementIndex springElementIndex) const
    {
        assert(springElementIndex < mElementCount);

        return mIsBombAttachedBuffer[springElementIndex];
    }

    void AttachBomb(
        ElementIndex springElementIndex,
        Points & points,
        GameParameters const & gameParameters)
    {
        assert(springElementIndex < mElementCount);
        assert(false == mIsBombAttachedBuffer[springElementIndex]);

        mIsBombAttachedBuffer[springElementIndex] = true;

        // Augment mass of endpoints due to bomb

        points.SetMassToMaterialOffset(
            mEndpointsBuffer[springElementIndex].PointAIndex, 
            gameParameters.BombMass,
            *this);

        points.SetMassToMaterialOffset(
            mEndpointsBuffer[springElementIndex].PointBIndex, 
            gameParameters.BombMass,
            *this);
    }

    void DetachBomb(
        ElementIndex springElementIndex,
        Points & points)
    {
        assert(springElementIndex < mElementCount);
        assert(true == mIsBombAttachedBuffer[springElementIndex]);

        mIsBombAttachedBuffer[springElementIndex] = false;

        // Reset mass of endpoints 
        points.SetMassToMaterialOffset(mEndpointsBuffer[springElementIndex].PointAIndex, 0.0f, *this);
        points.SetMassToMaterialOffset(mEndpointsBuffer[springElementIndex].PointBIndex, 0.0f, *this);
    }

private:

    static float CalculateStiffnessCoefficient(        
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        float springStiffness,
        float stiffnessAdjustment,
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

    //
    // Bombs
    //

    Buffer<bool> mIsBombAttachedBuffer;

    //
    // Container state
    //

    // The current stiffness adjustment
    float mCurrentStiffnessAdjustment;
};

}

constexpr Physics::Springs::Characteristics operator&(Physics::Springs::Characteristics a, Physics::Springs::Characteristics b)
{
    return static_cast<Physics::Springs::Characteristics>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool Physics::Springs::IsHull(ElementIndex springElementIndex) const
{
    assert(springElementIndex < mElementCount);

    return Physics::Springs::Characteristics::None != (mCharacteristicsBuffer[springElementIndex] & Physics::Springs::Characteristics::Hull);
}

inline bool Physics::Springs::IsRope(ElementIndex springElementIndex) const
{
    assert(springElementIndex < mElementCount);

    return Physics::Springs::Characteristics::None != (mCharacteristicsBuffer[springElementIndex] & Physics::Springs::Characteristics::Rope);
}
