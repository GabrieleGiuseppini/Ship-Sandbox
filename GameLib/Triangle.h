/***************************************************************************************
* Original Author:		Luke Wren (wren6991@gmail.com)
* Created:				2013-04-30
* Modified By:			Gabriele Giuseppini
* Copyright:			Luke Wren (http://github.com/Wren6991),
*						Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

namespace Physics
{

class Triangle : public ShipElement<Triangle>
{
public:

    Triangle(
        Ship * parentShip,
        ElementContainer::ElementIndex pointAIndex,
        ElementContainer::ElementIndex pointBIndex,
        ElementContainer::ElementIndex pointCIndex);

	void Destroy(
        Points & points,
        ElementContainer::ElementIndex sourcePointElementIndex);

	inline ElementContainer::ElementIndex GetPointAIndex() const { return mPointAIndex; }
	inline ElementContainer::ElementIndex GetPointBIndex() const { return mPointBIndex; }
	inline ElementContainer::ElementIndex GetPointCIndex() const { return mPointCIndex; }

private:
	
    ElementContainer::ElementIndex const mPointAIndex;
    ElementContainer::ElementIndex const mPointBIndex;
    ElementContainer::ElementIndex const mPointCIndex;
};

}
