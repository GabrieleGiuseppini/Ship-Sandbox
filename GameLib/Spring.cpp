/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cassert>

namespace Physics {

//   SSS    PPPP     RRRR     IIIIIII  N     N    GGGGG
// SS   SS  P   PP   R   RR      I     NN    N   GG
// S        P    PP  R    RR     I     N N   N  GG
// SS       P   PP   R   RR      I     N N   N  G
//   SSS    PPPP     RRRR        I     N  N  N  G
//      SS  P        R RR        I     N   N N  G  GGGG
//       S  P        R   R       I     N   N N  GG    G
// SS   SS  P        R    R      I     N    NN   GG  GG
//   SSS    P        R     R  IIIIIII  N     N    GGGG

Spring::Spring(
	Ship * parentShip,
	Point * a,
	Point * b,
	float restLength,
    Characteristics characteristics,
	Material const *material)
	: ShipElement(parentShip)
	, mPointA(a)
	, mPointB(b)
	, mRestLength(restLength)
    , mStiffnessCoefficient(CalculateStiffnessCoefficient(*mPointA, *mPointB, material->Stiffness))
    , mDampingCoefficient(CalculateDampingCoefficient(*mPointA, *mPointB))
    , mCharacteristics(characteristics)
	, mMaterial(material)
    , mWaterPermeability(Characteristics::None != (mCharacteristics & Characteristics::Hull) ? 0.0f : 1.0f)
    , mIsStressed(false)
{
    // Add ourselves to our endpoints
    a->AddConnectedSpring(this);
    b->AddConnectedSpring(this);
}

void Spring::Destroy(Point const * pointSource)
{
    assert(!mPointA->IsDeleted());
    assert(!mPointB->IsDeleted());

    // Used to do more complicated checks, but easier (and better) to make everything leak when it breaks

    // Make endpoints leak and destroy their triangles
    // Note: technically, should only destroy those triangles that contain the A-B side, and definitely
    // make both A and B leak
    if (mPointA != pointSource)
        mPointA->Breach();
    if (mPointB != pointSource)
        mPointB->Breach();

    // Remove ourselves from our endpoints
    if (mPointA != pointSource)
        mPointA->RemoveConnectedSpring(this);
    if (mPointB != pointSource)
        mPointB->RemoveConnectedSpring(this);

    // Zero out our dynamics factors, so that we can still calculate Hooke's 
    // and damping forces for this spring without running the risk of 
    // affecting non-deleted points
    mStiffnessCoefficient = 0.0f;
    mDampingCoefficient = 0.0f;

    // Zero out our water permeability, to
    // avoid draining water to destroyed points
    mWaterPermeability = 0.0f;

    // Remove ourselves
    ShipElement::Destroy();
}

float Spring::CalculateStiffnessCoefficient(
    Point const & pointA,
    Point const & pointB,
    float springStiffness)
{
    // The empirically-determined constant for the spring stiffness
    //
    // The simulation is quite sensitive to this value:
    // - 0.80 is almost fine (though bodies are sometimes soft)
    // - 0.95 makes everything explode (this was the equivalent of Luke's original code)
    static constexpr float C = 0.8f;

    float const massFactor = (pointA.GetMass() * pointB.GetMass()) / (pointA.GetMass() + pointB.GetMass());
    static constexpr float dtSquared = GameParameters::DynamicsSimulationStepTimeDuration<float> * GameParameters::DynamicsSimulationStepTimeDuration<float>;

    return C * springStiffness * massFactor / dtSquared;
}

float Spring::CalculateDampingCoefficient(
    Point const & pointA,
    Point const & pointB)
{
    // The empirically-determined constant for the spring damping
    //
    // The simulation is quite sensitive to this value:
    // - 0.03 is almost fine (though bodies are sometimes soft)
    // - 0.8 makes everything explode (this was the equivalent of Luke's original code)
    static constexpr float C = 0.03f;

    float const massFactor = (pointA.GetMass() * pointB.GetMass()) / (pointA.GetMass() + pointB.GetMass());
    static constexpr float dt = GameParameters::DynamicsSimulationStepTimeDuration<float>;

    return C * massFactor / dt;
}

}