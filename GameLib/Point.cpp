/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cassert>
#include <cmath>

namespace Physics {

// PPPP       OOO    IIIIIII  N     N  TTTTTTT
// P   PP    O   O      I     NN    N     T
// P    PP  O     O     I     N N   N     T
// P   PP   O     O     I     N N   N     T
// PPPP     O     O     I     N  N  N     T
// P        O     O     I     N   N N     T
// P        O     O     I     N   N N     T
// P         O   O      I     N    NN     T
// P          OOO    IIIIIII  N     N     T

vec2 const Point::AABBRadius = vec2(0.4f, 0.4f);

Point::Point(
	Ship * parentShip,
	vec2 const & position,
	Material const * material,
	float buoyancy,
    int elementIndex)
	: ShipElement(parentShip)
	, mPosition(position)
	, mVelocity(0.0f, 0.0f)
    , mForce(0.0f, 0.0f)
    , mMassFactor(CalculateMassFactor(material->Mass))
	, mBuoyancy(buoyancy)
    , mMaterial(material)
	, mWater(0.0f)
    , mLight(0.0f)
    , mIsLeaking(false)
    , mElementIndex(elementIndex)
    , mConnectedSprings()
    , mConnectedTriangles()
    , mConnectedElectricalElement(nullptr)
    , mConnectedComponentId(0u)
    , mCurrentConnectedComponentDetectionStepSequenceNumber(0u)
{
}

void Point::Destroy()
{
    //
    // Destroy all springs attached to this point
    //

    for (Spring * spring : mConnectedSprings)
    {
        assert(!spring->IsDeleted());
        spring->Destroy(this);
    }

    mConnectedSprings.clear();

    //
	// Destroy connected triangles
    //

    DestroyConnectedTriangles();

    assert(0u == mConnectedTriangles.size());

    //
    // Destroy connected electrical elements
    //

    if (nullptr != mConnectedElectricalElement)
    { 
        mConnectedElectricalElement->Destroy();
        mConnectedElectricalElement = nullptr;
    }
    

    //
    // Remove ourselves
    //

    ShipElement::Destroy();
}

void Point::Breach()
{
    // Start leaking
	mIsLeaking = true;

    // Destroy all of our connected triangles
    DestroyConnectedTriangles();
}

void Point::DestroyConnectedTriangles()
{
    for (Triangle * triangle : mConnectedTriangles)
    {
        assert(!triangle->IsDeleted());
        triangle->Destroy(this);
    }

    mConnectedTriangles.clear();
}

float Point::CalculateMassFactor(float mass)
{
    assert(mass > 0.0f);

    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;

    return dt * dt / mass;
}

}