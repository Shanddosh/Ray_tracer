#pragma once
#include <math.h>
#include "ray.h"

/**
 * Camera class for generating rays through pixels
 * Sets up a virtual camera with specified position, orientation, and field of view
 */
class camera{
  public:
    /**
     * Camera constructor - sets up the camera coordinate system and viewport
     * @param lookfrom Camera position in world space
     * @param lookat Point the camera is looking at
     * @param vup "View up" direction (usually (0,1,0) for Y-up)
     * @param vfov Vertical field of view in degrees
     * @param aspect Aspect ratio (width/height)
     */
    camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect){
      vec3 u,v,w; // Camera coordinate system basis vectors
      
      // Convert vertical FOV from degrees to radians and calculate viewport dimensions
      float theta = vfov*M_PI/180;
      float half_height = tan(theta/2);
      float half_width = aspect*half_height;

      origin = lookfrom;
      
      // Build orthonormal basis for camera coordinate system
      w = unit_vector(lookfrom-lookat);  // Camera "back" direction (negative z)
      u = unit_vector(cross(vup,w));     // Camera "right" direction (positive x)
      v = cross(w,u);                    // Camera "up" direction (positive y)

      // Calculate viewport corners and dimensions in world space
      lower_left_corner = origin - half_width*u - half_height*v - w;
      horizontal = 2*half_width*u;   // Full width vector
      vertical = 2*half_height*v;    // Full height vector
    }

    /**
     * Generate a ray from camera through a specific pixel
     * @param s Horizontal coordinate [0,1] across image width
     * @param t Vertical coordinate [0,1] across image height
     * @return Ray from camera origin through specified pixel
     */
    ray get_ray(float s, float t) { 
      return ray(origin, lower_left_corner + s*horizontal + t*vertical - origin);
    }

    vec3 origin;           // Camera position
    vec3 horizontal;       // Horizontal span of viewport
    vec3 vertical;         // Vertical span of viewport
    vec3 lower_left_corner;// Bottom-left corner of viewport
};