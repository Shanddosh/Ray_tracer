#pragma once
#include "vec3.h"

/**
 * Ray class - fundamental primitive for ray tracing
 * Represents a mathematical ray: origin + t * direction
 * Used for camera rays, reflected rays, refracted rays, and shadow rays
 */
class ray
{
  public:
    ray(){}
    ray(const vec3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    // Accessor methods
    vec3 origin() const {return orig;}
    vec3 direction() const {return dir;}

    /**
     * Evaluate ray at parameter t
     * @param t Distance along ray (t=0 gives origin, t=1 gives origin+direction)
     * @return Point on ray at distance t
     */
    vec3 at(float t) const {return orig+t*dir;}

    vec3 orig; // Ray starting point
    vec3 dir;  // Ray direction (not necessarily unit length)
};