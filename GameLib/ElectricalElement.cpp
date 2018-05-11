/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

ElectricalElement::ElectricalElement(    
    Ship * parentShip,
    ElementContainer::ElementIndex pointIndex,
    Type type)
    : ShipElement(parentShip)
    , mPointIndex(pointIndex)
    , mType(type)
    , mLastGraphVisitStepSequenceNumber(0u)
{
}

void ElectricalElement::Destroy()
{
    // Remove ourselves
    ShipElement::Destroy();
}

}