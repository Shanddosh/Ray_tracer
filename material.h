#pragma once
#include "vec3.h"
#include "ray.h"
#include "hitable.h"

/**
 * Abstract base class for all materials in the ray tracer
 * Defines the interface that all materials must implement
 */
class material {
  public: 
    /**
     * Pure virtual function that determines how a ray interacts with this material
     * @param r_in The incoming ray that hit the surface
     * @param rec Hit record containing hit point, normal, and other surface info
     * @param attenuation Output: how much light is absorbed (color filter effect)
     * @param scattered Output: the new ray direction after material interaction
     * @return true if ray scatters, false if ray is completely absorbed
     */
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const = 0;
};

/**
 * Lambertian (diffuse) material class
 * Simulates perfectly matte surfaces that scatter light uniformly in all directions
 * Examples: chalk, unpolished wood, fabric, rough concrete
 */
class lambertian: public material{
  public:
    /**
     * Constructor
     * @param a The albedo (surface color/reflectance) - how much of each color channel is reflected
     */
    lambertian(const vec3& a) : albedo(a) {}
    
    /**
     * Scatter function for diffuse materials
     * Implements perfect Lambertian scattering - rays bounce randomly in hemisphere around surface normal
     */
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {      
      // Generate random target point in unit sphere around surface normal
      // This creates the characteristic diffuse look where light scatters uniformly
      vec3 target = rec.p + rec.normal + random_in_unit_sphere();
      
      // Create scattered ray from hit point toward random target
      scattered = ray(rec.p, target-rec.p);
      
      // Set color attenuation - how much of each color channel is reflected
      attenuation = albedo;
      
      // Lambertian materials always scatter (never perfectly absorb)
      return true;
    }

    vec3 albedo; // Surface color/reflectance [0,1] for each RGB channel
};

/**
 * Metal material class
 * Simulates reflective metallic surfaces with perfect mirror-like reflection
 * Examples: polished metal, mirrors, chrome surfaces
 */
class metal: public material{
  public:
    /**
     * Constructor
     * @param a The albedo (metallic color/tint) - metals can have colored reflections
     */
    metal(const vec3& a) : albedo(a) {}

    /**
     * Calculate perfect mirror reflection using the law of reflection
     * @param v Incident ray direction (should be unit vector)
     * @param n Surface normal (should be unit vector)
     * @return Reflected ray direction
     */
    vec3 reflect(const vec3& v, const vec3& n) const {
      // Formula: reflected = incident - 2 * (incident · normal) * normal
      // This implements the law of reflection: angle of incidence = angle of reflection
      return v-2*dot(v,n)*n;
    }

    /**
     * Scatter function for metallic materials
     * Implements perfect specular (mirror) reflection
     */
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {      
      // Calculate perfect reflection direction
      vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
      
      // Create scattered ray in reflected direction
      scattered = ray(rec.p, reflected);
      
      // Set metallic color tint
      attenuation = albedo;
      
      // Only scatter if reflection direction is above surface (dot product > 0)
      // This prevents rays from reflecting "into" the surface due to numerical errors
      return dot(scattered.direction(), rec.normal) > 0;
    }

    vec3 albedo; // Metallic color/tint - affects color of reflections
};

/**
 * Dielectric material class
 * Simulates transparent materials like glass, water, diamond, etc.
 * Handles both reflection and refraction based on viewing angle and refractive index
 */
class dielectric : public material{
  public:
    /**
     * Constructor
     * @param ri Refractive index of the material (glass ≈ 1.5, water ≈ 1.33, diamond ≈ 2.4)
     */
    dielectric(float ri) : ref_idx(ri){}

    /**
     * Calculate perfect mirror reflection (same as metal class)
     * @param v Incident ray direction
     * @param n Surface normal
     * @return Reflected ray direction
     */
    vec3 reflect(const vec3& v, const vec3& n) const {
      return v-2*dot(v,n)*n;
    }

    /**
     * Calculate refracted ray direction using Snell's law
     * @param v Incident ray direction
     * @param n Surface normal
     * @param ni_over_nt Ratio of refractive indices (n_incident / n_transmitted)
     * @param refracted Output parameter for refracted ray direction
     * @return true if refraction occurs, false if total internal reflection
     */
    bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) const {
      vec3 uv = unit_vector(v);
      float dt = dot(uv, n);
      
      // Calculate discriminant to check if refraction is possible
      // If discriminant < 0, we have total internal reflection
      float discriminant = 1 - ni_over_nt*ni_over_nt*(1-dt*dt);
      
      if (discriminant > 0) {
        // Apply Snell's law to calculate refracted direction
        refracted = ni_over_nt*(uv-n*dt) - n*sqrt(discriminant);
        return true;
      }
      else
        return false; // Total internal reflection occurs
    }

    /**
     * Schlick's approximation for Fresnel reflectance
     * Calculates probability of reflection vs refraction based on viewing angle
     * At grazing angles, even glass becomes very reflective (like looking at water)
     * @param cosine Cosine of angle between incident ray and surface normal
     * @return Probability of reflection [0,1]
     */
    float schlick(float cosine) const{
      // Calculate reflectance at normal incidence (perpendicular to surface)
      float r0 = (1-ref_idx)/(1+ref_idx);
      r0 = r0*r0;
      
      // Schlick's polynomial approximation for Fresnel equations
      // As angle increases (cosine decreases), reflectance increases
      return r0 + (1-r0)*pow(1-cosine, 5);
    }

    /**
     * Scatter function for dielectric (glass-like) materials
     * Randomly chooses between reflection and refraction based on Fresnel equations
     */
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {      
      vec3 outward_normal;   // Normal pointing toward incident medium
      vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
      float ni_over_nt;      // Ratio of refractive indices
      
      // Glass doesn't absorb light, so attenuation is white (no color filtering)
      attenuation = vec3(1,1,1);
      
      vec3 refracted;        // Will store refracted ray direction
      float reflect_prob;    // Probability of reflection vs refraction
      float cosine;          // Cosine of incident angle

      // Determine if ray is entering or exiting the dielectric material
      if (dot(r_in.direction(), rec.normal) > 0) {
        // Ray is exiting the material (going from glass to air)
        outward_normal = -rec.normal;  // Flip normal to point outward
        ni_over_nt = ref_idx;          // Glass to air ratio
        cosine = ref_idx * dot(r_in.direction(), rec.normal) / r_in.direction().length();
      }
      else {
        // Ray is entering the material (going from air to glass)
        outward_normal = rec.normal;   // Normal already points outward
        ni_over_nt = 1.0/ref_idx;      // Air to glass ratio
        cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
      }

      // Try to calculate refraction using Snell's law
      if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted)) {
        // Refraction is possible - calculate probability using Schlick approximation
        reflect_prob = schlick(cosine);
      }
      else {
        // Total internal reflection - ray must be reflected
        reflect_prob = 1;
      }

      // Randomly choose between reflection and refraction based on calculated probability
      if ((double)rand()/RAND_MAX < reflect_prob)  // drand48() returns random float [0,1)
        scattered = ray(rec.p, reflected);  // Reflect the ray
      else
        scattered = ray(rec.p, refracted);  // Refract the ray

      // Dielectric materials always scatter light (either reflect or refract)
      return true;
    }

    float ref_idx; // Refractive index of the material
};