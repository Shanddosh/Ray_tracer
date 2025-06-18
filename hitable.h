#pragma once
#include <vector>
#include <memory>

#include "ray.h"

using std::shared_ptr;
using std::make_shared;

class material;

/**
 * Structure to store information about a ray-object intersection
 */
struct hit_record {
  float t;                      // Distance along ray where hit occurred
  vec3 p;                       // 3D point of intersection
  vec3 normal;                  // Surface normal at intersection point
  shared_ptr<material> mat_ptr; // Material of the hit object
};

/**
 * Abstract base class for all objects that can be hit by rays
 * Defines the interface for ray-object intersection testing
 */
class hitable{
  public:
    /**
     * Test if a ray intersects with this object
     * @param r The ray to test
     * @param t_min Minimum distance to consider valid hits
     * @param t_max Maximum distance to consider valid hits
     * @param rec Output: hit information if intersection occurs
     * @return true if ray hits object, false otherwise
     */
    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
};

/**
 * Container class that holds multiple hitable objects
 * Allows treating a collection of objects as a single hitable entity
 */
class hitable_list : public hitable {
  public:
    hitable_list() {}
    hitable_list(shared_ptr<hitable> object){ add(object); }

    void add(shared_ptr<hitable> object) { objects.push_back(object); }
    void clear() { objects.clear(); }
    virtual bool hit(const ray& r, float tmin, float t_max, hit_record& rec) const;

    std::vector<shared_ptr<hitable> > objects;
};

/**
 * Hit testing for collections of objects
 * Finds the closest intersection among all objects in the list
 */
bool hitable_list::hit(const ray& r, float t_min, float t_max, hit_record& rec) const{
  hit_record temp_rec;
  bool hit_anything = false;
  double closest_so_far = t_max;
  
  // Test ray against each object, keeping track of closest hit
  for (const auto& object: objects)
    if (object->hit(r, t_min, closest_so_far, temp_rec))
    {
      hit_anything = true;
      closest_so_far = temp_rec.t;  // Update search range to closest hit
      rec = temp_rec;               // Store closest hit info
    }
  return hit_anything;
}

/**
 * Sphere primitive - most common shape in ray tracing
 * Defined by center point, radius, and material
 */
class sphere: public hitable{
  public:
    sphere(){}
    sphere(vec3 cen, float r, shared_ptr<material> mat_ptr) : center(cen), radius(r), mat(mat_ptr){};
    virtual bool hit(const ray& r, float tmin, float t_max, hit_record& rec) const override;

    vec3 center;
    float radius;
    shared_ptr<material> mat;
};

/**
 * Ray-sphere intersection using quadratic formula
 * Solves: |ray(t) - center|² = radius²
 */
bool sphere::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
  vec3 oc = r.origin() - center;
  
  // Quadratic equation coefficients: at² + bt + c = 0
  float a = dot(r.direction(), r.direction());
  float b = dot(oc, r.direction());
  float c = dot(oc,oc) - radius*radius;
  float discriminant = b*b - a*c;
  
  if (discriminant > 0) {
    // Two potential intersection points - check closer one first
    float temp = (-b-sqrt(discriminant))/a;
    if (temp < t_max && temp > t_min){
      rec.t = temp;
      rec.p = r.at(rec.t);
      rec.normal = (rec.p-center)/radius;  // Normalized outward normal
      rec.mat_ptr = mat;
      return true;
    }
    
    // Check farther intersection point
    temp = (-b+sqrt(discriminant))/a;
    if (temp < t_max && temp > t_min){
      rec.t = temp;
      rec.p = r.at(rec.t);
      rec.normal = (rec.p-center)/radius;
      rec.mat_ptr = mat;
      return true;
    }
  }
  return false; // No valid intersection found
}