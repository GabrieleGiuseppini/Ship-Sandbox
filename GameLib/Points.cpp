/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cmath>

namespace Physics {

void Points::Destroy(ElementIndex pointElementIndex)
{
    assert(pointElementIndex < mElementCount);

    Network & network = mNetworkBuffer[pointElementIndex];

    //
    // Destroy all springs attached to this point
    //

    for (Spring * spring : network.ConnectedSprings)
    {
        assert(!spring->IsDeleted());
        spring->Destroy(*this, pointElementIndex);
    }

    network.ConnectedSprings.clear();

    //
    // Destroy all triangles connected to this point
    //

    for (Triangle * triangle : network.ConnectedTriangles)
    {
        assert(!triangle->IsDeleted());
        triangle->Destroy(*this, pointElementIndex);
    }

    network.ConnectedTriangles.clear();

    //
    // Destroy the connected electrical element
    //

    if (nullptr != network.ConnectedElectricalElement)
    {
        network.ConnectedElectricalElement->Destroy();
        network.ConnectedElectricalElement = nullptr;
    }


    //
    // Flag ourselves as deleted
    //

    mIsDeletedBuffer[pointElementIndex] = true;
}

vec2f Points::CalculateMassFactor(float mass)
{
    assert(mass > 0.0f);

    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;
    
    return vec2f(dt * dt / mass, dt * dt / mass);
}

}