/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

Triangle::Triangle(
    Ship * parentShip,
    Point * a,
    Point * b,
    Point * c)
    : ShipElement(parentShip)
    , mPointA(a)
    , mPointB(b)
    , mPointC(c)
{
    // Add ourselves to each point
    a->AddConnectedTriangle(this);
    b->AddConnectedTriangle(this);
    c->AddConnectedTriangle(this);
}

void Triangle::Destroy(
    Points & points,
    ElementContainer::ElementIndex sourcePointElementIndex)
{
    // Remove ourselves from each point
    if (mPointAIndex != sourcePointElementIndex)
	    points.RemoveConnectedTriangle(mPointAIndex, this);
    if (mPointBIndex != sourcePointElementIndex)
        points.RemoveConnectedTriangle(mPointBIndex, this);
    if (mPointCIndex != sourcePointElementIndex)
        points.RemoveConnectedTriangle(mPointCIndex, this);

    // Remove ourselves from ship
    ShipElement::Destroy();
}

}