#include "phys.h"

#include "render.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/gl.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>


// W     W    OOO    RRRR     L        DDDD
// W     W   O   O   R   RR   L        D  DDD
// W     W  O     O  R    RR  L        D    DD
// W     W  O     O  R   RR   L        D     D
// W     W  O     O  RRRR     L        D     D
// W  W  W  O     O  R RR     L        D     D
// W  W  W  O     O  R   R    L        D    DD
//  W W W    O   O   R    R   L        D  DDD
//   W W      OOO    R     R  LLLLLLL  DDDD

int imin(int a, int b)
{
    return a < b ? a : b;
}

int imax(int a, int b)
{
    return a > b ? a : b;
}

void phys::world::update(double dt)
{
    time += dt;

    // Advance simulation for points (velocity and forces)
	for (point * pt : allPoints)
	{
		pt->update(dt);
	}

    // Iterate the spring relaxation (can tune this parameter, or make it scale automatically depending on free time)
    doSprings(dt);

    // Get set of springs that exceed their breaking strain (broken springs)
	std::vector<spring *> brokenSprings;
	for (spring * sp : allSprings)
    {
		if (sp->isBroken())
		{
			brokenSprings.push_back(sp);
		}
    }

	// Remove broken springs
	for (spring * sp : brokenSprings)
	{
		delete sp;
	}

    // Tell each ship to update all of its water stuff
	for (ship * sh : allShips)
	{
		sh->update(dt);
	}
}

void phys::world::doSprings(double dt)
{
    int nChunks = springScheduler.GetNumberOfThreads();
    size_t springChunkSize = allSprings.size() / (springScheduler.GetNumberOfThreads() + 1);

    for (int outiter = 0; outiter < 3; outiter++)
    {
        for (int iteration = 0; iteration < 8; iteration++)
        {
			std::set<spring *>::iterator springIt = allSprings.begin();
			for (size_t i = 0; i < allSprings.size(); i += springChunkSize)
			{
				size_t thisChunkSize = std::min(springChunkSize, allSprings.size() - i);

				springScheduler.Schedule(
					new springCalculateTask(
						springIt,
						thisChunkSize));
				
				std::advance(springIt, thisChunkSize);
			}

			assert(springIt == allSprings.end());

            springScheduler.WaitForAllTasks();
        }

		// Damp all springs
        float dampingamount = (1 - pow(0.0, dt)) * 0.5;
		for (spring * sp : allSprings)
		{
			sp->damping(dampingamount);
		}
    }
}

phys::world::springCalculateTask::springCalculateTask(std::set<spring*>::iterator _first, size_t _size)
{
    curIterator = _first;
    size = _size;
}

void phys::world::springCalculateTask::Process()
{
	for (size_t i = 0; i < size; ++i, ++curIterator)
	{
		spring * const sp = *curIterator;
		sp->update();
	}
}

phys::world::pointIntegrateTask::pointIntegrateTask(std::set<point *>::iterator _first, size_t _size, float _dt)
{
	curIterator = _first;
    size = _size;
    dt = _dt;
}

void phys::world::pointIntegrateTask::Process()
{
	for (size_t i = 0; i < size; ++i, ++curIterator)
	{
		point * const pt = *curIterator;
		pt->pos += pt->force * dt;
		pt->force = vec2(0, 0);
    }
}

void phys::world::render(double left, double right, double bottom, double top)
{
    // Draw the ocean floor
    renderLand(left, right, bottom, top);

	// Draw the water now if it needs to be behind the ship
    if (quickwaterfix)
        renderWater(left, right, bottom, top);

	// Draw all the points
    for (point * pt : allPoints)
        pt->render();
    
	// Draw all the springs
	for (spring * sp : allSprings)
        sp->render(false);

	// Draw all the ships, unless we are in X-Ray mode
	if (!xraymode)
	{
		for (ship * sh : allShips)
			sh->render();
	}

	if (showstress)
	{
		// Draw stressed springs
		for (spring * sp : allSprings)
			if (sp->isStressed())
				sp->render(true);
	}

	// Draw the water now if it needs to be in front of the ship
    if (!quickwaterfix)
        renderWater(left, right, bottom, top);

    glBegin(GL_LINES);
    glLineWidth(1.f);
    glEnd();

    //buildBVHTree(true, points, collisionTree);
}

void swapf(float &x, float &y)
{
    float temp = x;
    x = y;
    y = temp;
}

float medianOf3(float a, float b, float c)
{
    if (a < b)
        swapf(a, b);
    if (b < c)
        swapf(b, c);
    if (a < b)
        swapf(a, b);
    return b;
}

void phys::world::buildBVHTree(bool splitInX, std::vector<point*> &pointlist, BVHNode *thisnode, int depth)
{
    int npoints = pointlist.size();
    if (npoints)
        thisnode->volume = pointlist[0]->getAABB();
    for (unsigned int i = 1; i < npoints; i++)
        thisnode->volume.extendTo(pointlist[i]->getAABB());

    thisnode->volume.render();
    if (npoints <= BVHNode::MAX_N_POINTS || depth >= BVHNode::MAX_DEPTH)
    {
        thisnode->isLeaf = true;
        thisnode->pointCount = npoints;
        for (int i = 0; i < npoints; i++)
            thisnode->points[i] = pointlist[i];
    }
    else
    {
        float pivotline = splitInX ?
            medianOf3(pointlist[0]->pos.x, pointlist[npoints / 2]->pos.x, pointlist[npoints - 1]->pos.x) :
            medianOf3(pointlist[0]->pos.y, pointlist[npoints / 2]->pos.y, pointlist[npoints - 1]->pos.y);
        std::vector<point*> listL;
        std::vector<point*> listR;
        listL.reserve(npoints / 2);
        listR.reserve(npoints / 2);
        for (int i = 0; i < npoints; i++)
        {
            if (splitInX ? pointlist[i]->pos.x < pivotline : pointlist[i]->pos.y < pivotline)
                listL.push_back(pointlist[i]);
            else
                listR.push_back(pointlist[i]);
        }
        buildBVHTree(!splitInX, listL, thisnode->l, depth + 1);
        buildBVHTree(!splitInX, listR, thisnode->r, depth + 1);
    }
}

void phys::world::renderLand(double left, double right, double bottom, double top)
{
    glColor4f(0.5, 0.5, 0.5, 1);
    double slicewidth = (right - left) / 200.0;
    for (double slicex = left; slicex < right; slicex += slicewidth)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(slicex, oceanfloorheight(slicex), -1);
        glVertex3f(slicex + slicewidth, oceanfloorheight(slicex + slicewidth), -1);
        glVertex3f(slicex, bottom, -1);
        glVertex3f(slicex + slicewidth, bottom, -1);
        glEnd();
    }
}

void phys::world::renderWater(double left, double right, double bottom, double top)
{
    // Cut the water into vertical slices (to get the different heights of waves) and draw it
    glColor4f(0, 0.25, 1, 0.5);
    double slicewidth = (right - left) / 100.0;
    for (double slicex = left; slicex < right; slicex += slicewidth)
    {
        glBegin(GL_TRIANGLE_STRIP);
        glVertex3f(slicex, waterheight(slicex), -1);
        glVertex3f(slicex + slicewidth, waterheight(slicex + slicewidth), -1);
        glVertex3f(slicex, bottom, -1);
        glVertex3f(slicex + slicewidth, bottom, -1);
        glEnd();
    }
}

float phys::world::oceanfloorheight(float x)
{
    /*x += 1024.f;
    x = x - 2048.f * floorf(x / 2048.f);
    float t = x - floorf(x);
    return oceandepthbuffer[(int)floorf(x)] * (1 - t) + oceandepthbuffer[((int)ceilf(x)) % 2048] * t;*/
    return (sinf(x * 0.005f) * 10.f + sinf(x * 0.015f) * 6.f - sin(x * 0.0011f) * 45.f) - seadepth;
}

// Function of time and x (though time is constant during the update step, so no need to parameterise it)
float phys::world::waterheight(float x)
{
    return (sinf(x * 0.1f + time) * 0.5f + sinf(x * 0.3f - time * 1.1f) * 0.3f) * waveheight;
}

// Destroy all points within radius
void phys::world::destroyAt(vec2f pos, float radius)
{
	std::vector<point *> pointsToDelete;
	for (point * pt : allPoints)
    {
        if ((pt->pos - pos).length() < radius)
        {
			pointsToDelete.push_back(pt);
        }
    }

	for (point * pt : pointsToDelete)
	{
		delete pt;
	}
}

// Attract all points to a single position
void phys::world::drawTo(vec2f target)
{
	for (point * pt : allPoints)
    {
        vec2f dir = (target - pt->pos);
        double magnitude = 50000 / sqrt(0.1 + dir.length());
        pt->applyForce(dir.normalise() * magnitude);
    }
}

// Copy parameters and set up initial params:
phys::world::world(vec2f _gravity, double _buoyancy, double _strength)
{
    time = 0;
    gravity = _gravity;
    buoyancy = _buoyancy;
    strength = _strength;
    waterpressure = 0.3;
    waveheight = 1.0;
    seadepth = 150;
    collisionTree = BVHNode::allocateTree();
}

// Destroy everything in the set order
phys::world::~world()
{
    // DESTROY THE WORLD??? Y/N

	for (std::set<spring *>::iterator it = allSprings.begin(); it != allSprings.end();)
	{
		spring * sp = *it;
		it = allSprings.erase(it);

		delete sp;
	}

	/*
	std::vector<spring *> sps(allSprings.begin(), allSprings.end());
	for (spring * sp : sps)
        delete sp;
    */

	for (auto it = allPoints.begin(); it != allPoints.end();)
	{
		point * pt = *it;
		it = allPoints.erase(it);

		delete pt;
	}

	/*
	std::vector<point *> pts(allPoints.begin(), allPoints.end());
	for (point * pt : pts)
		delete pt;
	*/

	for (std::set<ship *>::iterator it = allShips.begin(); it != allShips.end();)
	{
		ship * sh = *it;
		it = allShips.erase(it);

		delete sh;
	}
	/*
	std::vector<ship *> shs(allShips.begin(), allShips.end());
    for (ship * sh : shs)
        delete sh;
	*/
}

// PPPP       OOO    IIIIIII  N     N  TTTTTTT
// P   PP    O   O      I     NN    N     T
// P    PP  O     O     I     N N   N     T
// P   PP   O     O     I     N N   N     T
// PPPP     O     O     I     N  N  N     T
// P        O     O     I     N   N N     T
// P        O     O     I     N   N N     T
// P         O   O      I     N    NN     T
// P          OOO    IIIIIII  N     N     T

// Just copies parameters into relevant fields:
phys::point::point(world *_parent, vec2 _pos, material *_mtl, double _buoyancy)
{
    wld = _parent;
    wld->allPoints.insert(this);
    pos = _pos;
    lastpos = pos;
    mtl = _mtl;
    buoyancy = _buoyancy;
    isLeaking = false;
    water = 0;
}

void phys::point::applyForce(vec2f f)
{
    force += f;
}

void phys::point::update(double dt)
{
    double mass = mtl->mass;
    this->applyForce(wld->gravity * (mass * (1 + fmin(water, 1) * wld->buoyancy * buoyancy)));    // clamp water to 1, so high pressure areas are not heavier.
    // Buoyancy:
    if (pos.y < wld->waterheight(pos.x))
        this->applyForce(wld->gravity * (-wld->buoyancy * buoyancy * mass));
    vec2f newlastpos = pos;
    // Water drag:
    if (pos.y < wld->waterheight(pos.x))
        lastpos += (pos - lastpos) * (1 - pow(0.6, dt));
    // Apply verlet integration:
    pos += (pos - lastpos) + force * (dt * dt / mass);
    // Collision with seafloor:
    float floorheight = wld->oceanfloorheight(pos.x);
    if (pos.y < floorheight)
    {
        vec2f dir = vec2f(floorheight - wld->oceanfloorheight(pos.x + 0.01f), 0.01f).normalise();   // -1 / derivative  => perpendicular to surface!
        pos += dir * (floorheight - pos.y);
    }
    lastpos = newlastpos;
    force = vec2f(0, 0);
}

vec2f phys::point::getPos()
{
    return pos;
}

vec3f phys::point::getColour(vec3f basecolour)
{
   float wetness = fmin(water, 1.0f) * 0.7f;
   return basecolour * (1.0f - wetness) + vec3f(0.0f, 0.0f, 0.8f) * wetness;
}

void phys::point::breach()
{
    isLeaking = true;

	// Remove all triangles that have this point in it
	std::vector<ship::triangle *> trs(triangles.begin(), triangles.end());
	for (ship::triangle * tr : trs)
    {
        delete tr;
    }
}

void phys::point::render()
{
    // Put a blue blob on leaking nodes (was more for debug purposes, but looks better IMO)
    if (isLeaking)
    {
        glColor3f(0, 0, 1);
        glBegin(GL_POINTS);
        glVertex3f(pos.x, pos.y, -1);
        glEnd();
    }
}

double phys::point::getPressure()
{
    return wld->gravity.length() * fmax(-pos.y, 0) * 0.1;  // 0.1 = scaling constant, represents 1/ship width
}

phys::AABB phys::point::getAABB()
{
    return phys::AABB(pos - vec2(radius, radius), pos + vec2(radius, radius));
}

phys::point::~point()
{
    // get rid of any attached triangles
    breach();

    // remove any springs attached to this point
	std::vector<spring *> springsToDelete;
    for (spring * sp : wld->allSprings)
    {
        if (sp->a == this || sp->b == this)
        {
			springsToDelete.push_back(sp);
        }
    }

	for (spring * sp : springsToDelete)
		delete sp;

    // remove from ships
    for (ship * sh : wld->allShips)
        sh->points.erase(this);

	// remove from world
	wld->allPoints.erase(this);
}

//   SSS    PPPP     RRRR     IIIIIII  N     N    GGGGG
// SS   SS  P   PP   R   RR      I     NN    N   GG
// S        P    PP  R    RR     I     N N   N  GG
// SS       P   PP   R   RR      I     N N   N  G
//   SSS    PPPP     RRRR        I     N  N  N  G
//      SS  P        R RR        I     N   N N  G  GGGG
//       S  P        R   R       I     N   N N  GG    G
// SS   SS  P        R    R      I     N    NN   GG  GG
//   SSS    P        R     R  IIIIIII  N     N    GGGG

phys::spring::spring(world *_parent, point *_a, point *_b, material *_mtl, double _length)
{
    wld = _parent;
    _parent->allSprings.insert(this);
    a = _a;
    b = _b;
    if (_length == -1)
        length = (a->pos - b->pos).length();
    else
        length = _length;
    mtl = _mtl;
}

phys::spring::~spring()
{
    // Used to do more complicated checks, but easier (and better) to make everything leak when it breaks
    a->breach();
    b->breach();

    // Scour out any references to this spring
    for (ship * shp : wld->allShips)
    {
        if (shp->adjacentnodes.find(a) != shp->adjacentnodes.end())
            shp->adjacentnodes[a].erase(b);

        if (shp->adjacentnodes.find(b) != shp->adjacentnodes.end())
            shp->adjacentnodes[b].erase(a);
    }

	wld->allSprings.erase(this);
}

void phys::spring::update()
{
    // Try to space the two points by the equilibrium length (need to iterate to actually achieve this for all points, but it's FAAAAST for each step)
    vec2f correction_dir = (b->pos - a->pos);
    float currentlength = correction_dir.length();
    correction_dir *= (length - currentlength) / (length * (a->mtl->mass + b->mtl->mass) * 0.85f); // * 0.8 => 25% overcorrection (stiffer, converges faster)
    a->pos -= correction_dir * b->mtl->mass;    // if b is heavier, a moves more.
    b->pos += correction_dir * a->mtl->mass;    // (and vice versa...)
}

void phys::spring::damping(float amount)
{
    vec2f springdir = (a->pos - b->pos).normalise();
    springdir *= (a->pos - a->lastpos - (b->pos - b->lastpos)).dot(springdir) * amount;   // relative velocity � spring direction = projected velocity, amount = amount of projected velocity that remains after damping
    a->lastpos += springdir;
    b->lastpos -= springdir;
}

void phys::spring::render(bool showStress)
{
    // If member is heavily stressed, highlight it in red (ignored if world's showstress field is false)
    glBegin(GL_LINES);
    if (showStress)
        glColor3f(1, 0, 0);
    else
        render::setColour(a->getColour(mtl->colour));
    glVertex3f(a->pos.x, a->pos.y, -1);
    if (!showStress)
        render::setColour(b->getColour(mtl->colour));
    glVertex3f(b->pos.x, b->pos.y, -1);
    glEnd();
}

bool phys::spring::isStressed()
{
    // Check whether strain is more than the word's base strength * this object's relative strength
    return (a->pos - b->pos).length() / this->length > 1 + (wld->strength * mtl->strength) * 0.25;
}

bool phys::spring::isBroken()
{
    // Check whether strain is more than the word's base strength * this object's relative strength
    return (a->pos - b->pos).length() / this->length > 1 + (wld->strength * mtl->strength);
}


//   SSS    H     H  IIIIIII  PPPP
// SS   SS  H     H     I     P   PP
// S        H     H     I     P    PP
// SS       H     H     I     P   PP
//   SSS    HHHHHHH     I     PPPP
//      SS  H     H     I     P
//       S  H     H     I     P
// SS   SS  H     H     I     P
//   SSS    H     H  IIIIIII  P

phys::ship::ship(world *_parent)
{
    wld = _parent;
    wld->allShips.insert(this);
}

void phys::ship::update(double dt)
{
    leakWater(dt);
    for (int i = 0; i < 4; i++)
    {
        gravitateWater(dt);
        balancePressure(dt);
    }
    for (int i = 0; i < 4; i++)
        balancePressure(dt);
}

void phys::ship::leakWater(double dt)
{
    // Stuff some water into all the leaking nodes, if they're not under too much pressure
   for (auto iter = points.begin(); iter != points.end(); iter++)
   {
        point *p = *iter;
        double pressure = p->getPressure();
        if (p->isLeaking && p->pos.y < wld->waterheight(p->pos.x) && p->water < 1.5)
        {
            p->water += dt * wld->waterpressure * (pressure - p->water);
        }
   }
}

void phys::ship::gravitateWater(double dt)
{
    // Water flows into adjacent nodes (from a into b) in a quantity proportional to the cos of angle the beam makes
    // against gravity (parallel with gravity => 1 (full flow), perpendicular = 0)
    for (auto iter = adjacentnodes.begin(); iter != adjacentnodes.end(); ++iter)
    {
        point *a = iter->first;
        for (auto second = iter->second.begin(); second != iter->second.end(); ++second)
        {
            point *b = *second;
            double cos_theta = (b->pos - a->pos).normalise().dot(wld->gravity.normalise());
            if (cos_theta > 0)
            {
                double correction = std::min(0.5 * cos_theta * dt, a->water);   // The 0.5 can be tuned, it's just to stop all the water being stuffed into the first node...
                a->water -= correction;
                b->water += correction;
            }
        }
    }

}

void phys::ship::balancePressure(double dt)
{
    // If there's too much water in a node, try and push it into the others
    // (This needs to iterate over multiple frames for pressure waves to spread through water)
    for (auto iter = adjacentnodes.begin(); iter != adjacentnodes.end(); ++iter)
    {
        point *a = iter->first;
        if (a->water < 1)   // if water content is not above threshold, no need to force water out
            continue;
        for (std::set<point*>::iterator second = iter->second.begin(); second != iter->second.end(); ++second)
        {
            point *b = *second;
            double correction = (b->water - a->water) * 8 * dt; // can tune this number; value of 1 means will equalise in 1 second.
            a->water += correction;
            b->water -= correction;
        }
    }
}

void phys::ship::render()
{
    for (auto iter = triangles.begin(); iter != triangles.end(); ++iter)
    {
        triangle const *t = *iter;
        render::triangle(t->a->pos, t->b->pos, t->c->pos,
                         t->a->getColour(t->a->mtl->colour),
                         t->b->getColour(t->b->mtl->colour),
                         t->c->getColour(t->c->mtl->colour));
    }
}

phys::ship::~ship()
{
    /*for (unsigned int i = 0; i < triangles.size(); i++)
        delete triangles[i];*/
}

phys::ship::triangle::triangle(phys::ship *_parent, point *_a, point *_b, point *_c)
{
    parent = _parent;
    a = _a;
    b = _b;
    c = _c;
    a->triangles.insert(this);
    b->triangles.insert(this);
    c->triangles.insert(this);
}

phys::ship::triangle::~triangle()
{
    parent->triangles.erase(this);
    a->triangles.erase(this);
    b->triangles.erase(this);
    c->triangles.erase(this);
}

phys::AABB::AABB(vec2 _bottomleft, vec2 _topright)
{
    bottomleft = _bottomleft;
    topright = _topright;
}

void phys::AABB::extendTo(phys::AABB other)
{
    if (other.bottomleft.x < bottomleft.x)
        bottomleft.x = other.bottomleft.x;
    if (other.bottomleft.y < bottomleft.y)
        bottomleft.y = other.bottomleft.y;
    if (other.topright.x > topright.x)
        topright.x = other.topright.x;
    if (other.topright.y > topright.y)
        topright.y = other.topright.y;
}

void phys::AABB::render()
{
    render::box(bottomleft, topright);
}

phys::BVHNode* phys::BVHNode::allocateTree(int depth)
{
    if (depth <= 0)
        return 0;
    BVHNode *thisnode = new BVHNode;
    thisnode->l = allocateTree(depth - 1);
    thisnode->r = allocateTree(depth - 1);
    return thisnode;
}