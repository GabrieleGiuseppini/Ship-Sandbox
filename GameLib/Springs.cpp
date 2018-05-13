/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cmath>

namespace Physics {

void Springs::Add(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    Characteristics characteristics,
    Material const * material,
    Points const & points)
{
    mIsDeletedBuffer.emplace_back(false);

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex);

    mRestLengthBuffer.emplace_back((points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length());
    mCoefficientsBuffer.emplace_back(
        CalculateStiffnessCoefficient(pointAIndex, pointBIndex, material->Stiffness, points),
        CalculateDampingCoefficient(pointAIndex, pointBIndex, points));
    mCharacteristicsBuffer.emplace_back(characteristics);
    mMaterialBuffer.emplace_back(material);

    mWaterPermeabilityBuffer.emplace_back(Characteristics::None != (characteristics & Characteristics::Hull) ? 0.0f : 1.0f);

    mIsStressedBuffer.emplace_back(false);
}

void Springs::Destroy(
    ElementIndex springElementIndex,
    ElementIndex sourcePointElementIndex,
    Points & points,
    Triangles & triangles)
{
    assert(springElementIndex < mElementCount);
    
    Endpoints const & endpoints = mEndpointsBuffer[springElementIndex];

    assert(!points.IsDeleted(endpoints.PointAIndex));
    assert(!points.IsDeleted(endpoints.PointBIndex));

    // Used to do more complicated checks, but easier (and better) to make everything leak when it breaks

    // Make endpoints leak and destroy their triangles
    // Note: technically, should only destroy those triangles that contain the A-B side, and definitely
    // make both A and B leak
    if (endpoints.PointAIndex != sourcePointElementIndex)
        points.Breach(endpoints.PointAIndex, triangles);
    if (endpoints.PointBIndex != sourcePointElementIndex)
        points.Breach(endpoints.PointBIndex, triangles);

    // Remove ourselves from our endpoints
    if (endpoints.PointAIndex != sourcePointElementIndex)
        points.RemoveConnectedSpring(endpoints.PointAIndex, springElementIndex);
    if (endpoints.PointBIndex != sourcePointElementIndex)
        points.RemoveConnectedSpring(endpoints.PointBIndex, springElementIndex);

    // Zero out our coefficients, so that we can still calculate Hooke's 
    // and damping forces for this spring without running the risk of 
    // affecting non-deleted points
    mCoefficientsBuffer[springElementIndex].StiffnessCoefficient = 0.0f;
    mCoefficientsBuffer[springElementIndex].DampingCoefficient = 0.0f;

    // Zero out our water permeability, to
    // avoid draining water to destroyed points
    mWaterPermeabilityBuffer[springElementIndex] = 0.0f;

    //
    // Flag ourselves as deleted
    //

    mIsDeletedBuffer[springElementIndex] = true;
}

void Springs::UploadElements(
    int shipId,
    RenderContext & renderContext,
    Points const & points) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            assert(points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointBIndex(i)));

            if (IsRope(i))
            {
                renderContext.UploadShipElementRope(
                    shipId,
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    points.GetConnectedComponentId(GetPointAIndex(i)));
            }
            else
            {
                renderContext.UploadShipElementSpring(
                    shipId,
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    points.GetConnectedComponentId(GetPointAIndex(i)));
            }
        }
    }
}

void Springs::UploadStressedSpringElements(
    int shipId,
    RenderContext & renderContext,
    Points const & points) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            if (mIsStressedBuffer[i])
            {
                assert(points.GetConnectedComponentId(GetPointAIndex(i)) == points.GetConnectedComponentId(GetPointBIndex(i)));
                
                renderContext.UploadShipElementStressedSpring(
                    shipId,
                    GetPointAIndex(i),
                    GetPointBIndex(i),
                    points.GetConnectedComponentId(GetPointAIndex(i)));
            }
        }
    }
}

bool Springs::UpdateStrains(
    GameParameters const & gameParameters,
    World const & parentWorld,    
    IGameEventHandler * gameEventHandler,
    Points & points,
    Triangles & triangles)
{
    bool isAtLeastOneBroken = false;
    
    for (ElementIndex i : *this)
    {
        // Avoid breaking deleted springs
        if (!mIsDeletedBuffer[i])
        {
            // Calculate strain
            float dx = (points.GetPosition(mEndpointsBuffer[i].PointAIndex) - points.GetPosition(mEndpointsBuffer[i].PointBIndex)).length();
            float const strain = fabs(mRestLengthBuffer[i] - dx) / mRestLengthBuffer[i];

            // Check against strength
            float const effectiveStrength = gameParameters.StrengthAdjustment * mMaterialBuffer[i]->Strength;
            if (strain > effectiveStrength)
            {
                // It's broken!
                this->Destroy(i, NoneElementIndex, points, triangles);

                // Notify
                gameEventHandler->OnBreak(
                    mMaterialBuffer[i],
                    parentWorld.IsUnderwater(points.GetPosition(mEndpointsBuffer[i].PointAIndex)),
                    1);

                isAtLeastOneBroken = true;
            }
            else if (strain > 0.25f * effectiveStrength)
            {
                // It's stressed!
                if (!mIsStressedBuffer[i])
                {
                    mIsStressedBuffer[i] = true;

                    // Notify
                    gameEventHandler->OnStress(
                        mMaterialBuffer[i],
                        parentWorld.IsUnderwater(points.GetPosition(mEndpointsBuffer[i].PointAIndex)),
                        1);
                }
            }
            else
            {
                // Just fine
                mIsStressedBuffer[i] = false;
            }
        }
    }

    return isAtLeastOneBroken;
}

float Springs::CalculateStiffnessCoefficient(    
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    float springStiffness,
    Points const & points)
{
    // The empirically-determined constant for the spring stiffness
    //
    // The simulation is quite sensitive to this value:
    // - 0.80 is almost fine (though bodies are sometimes soft)
    // - 0.95 makes everything explode (this was the equivalent of Luke's original code)
    static constexpr float C = 0.8f;

    float const massFactor = (points.GetMass(pointAIndex) * points.GetMass(pointBIndex)) / (points.GetMass(pointAIndex) + points.GetMass(pointBIndex));
    static constexpr float dtSquared = GameParameters::DynamicsSimulationStepTimeDuration<float> * GameParameters::DynamicsSimulationStepTimeDuration<float>;

    return C * springStiffness * massFactor / dtSquared;
}

float Springs::CalculateDampingCoefficient(    
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    Points const & points)
{
    // The empirically-determined constant for the spring damping
    //
    // The simulation is quite sensitive to this value:
    // - 0.03 is almost fine (though bodies are sometimes soft)
    // - 0.8 makes everything explode (this was the equivalent of Luke's original code)
    static constexpr float C = 0.03f;

    float const massFactor = (points.GetMass(pointAIndex) * points.GetMass(pointBIndex)) / (points.GetMass(pointAIndex) + points.GetMass(pointBIndex));
    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;

    return C * massFactor / dt;
}

}