﻿/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

Lamp::Lamp(
    Ship * parentShip,
    ElementContainer::ElementIndex pointIndex)
    : ElectricalElement(        
        parentShip,
        pointIndex,
        ElectricalElement::Type::Lamp)
{
}

}