#include <iostream>
#include <fstream>
#include <cfloat>
#include "ray.h"
#include "hitable.h"
#include "camera.h"
#include "material.h"

/**
 * Creates a complex random scene with multiple spheres of different materials
 * This generates the classic "Ray Tracing in One Weekend" cover scene
 */
hitable_list randomScene(){
  hitable_list world;

  // Create a large ground sphere (acts as the floor)
  auto ground_material = make_shared<lambertian>(vec3(0.5, 0.5, 0.5));
	world.add(make_shared<sphere>(vec3(0,-1000,0), 1000, ground_material));

  // Generate a grid of small spheres with random materials
  for (int a=-11; a<11; a++)
    for (int b=-11; b<11; b++) {
      float choose_mat = random_float(); // Random value to determine material type
      vec3 center(a+random_float(0,0.9), 0.2, b+random_float(0,0.9)); // Slightly randomized position

      // Skip spheres that are too close to the main dielectric sphere
      if ((center-vec3(4,0.2,0)).length() < 0.9)
        continue;

      shared_ptr<material> mat;
      // 80% chance for diffuse (lambertian) material
      if (choose_mat < 0.8)
        mat = make_shared<lambertian>(random_vec3());
      // 15% chance for metal material
      else if (choose_mat < 0.95)
        mat = make_shared<metal>(random_vec3(0,0.5));
      // 5% chance for glass (dielectric) material
      else
        mat = make_shared<dielectric>(1.5);

      // Add small sphere with chosen material
      world.add(make_shared<sphere>(center, 0.2, mat));
    }

  // Add three large focal spheres with different materials
  
  // Large glass sphere in center
  auto material1 = make_shared<dielectric>(1.5);
  world.add(make_shared<sphere>(vec3(0, 1, 0), 1.0, material1));

  // Large diffuse sphere on the left (brownish color)
  auto material2 = make_shared<lambertian>(vec3(0.4, 0.2, 0.1));
  world.add(make_shared<sphere>(vec3(-4, 1, 0), 1.0, material2));

  // Large metal sphere on the right (goldish color)
  auto material3 = make_shared<metal>(vec3(0.7, 0.6, 0.5));
  world.add(make_shared<sphere>(vec3(4, 1, 0), 1.0, material3));
  
  return world;
}

/**
 * Recursively calculates the color of a ray by tracing it through the scene
 * @param r The ray to trace
 * @param world The scene containing all objects
 * @param depth Current recursion depth (for limiting ray bounces)
 * @return The final color contribution of this ray
 */
vec3 color(const ray& r, const hitable& world, int depth){
  hit_record rec;
  
  // Limit ray bounces to prevent infinite recursion and improve performance
  if (depth > 50)
    return vec3(0,0,0); // Return black if too many bounces

  // Check if ray hits any object in the scene
  // Use small epsilon (0.0001) to avoid "shadow acne" from floating point precision
  if (world.hit(r, 0.0001, FLT_MAX, rec)){
    ray scattered;      // The new ray direction after scattering
    vec3 attenuation;   // How much light is absorbed vs reflected
    
    // Ask the material how it scatters the incoming ray
    if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
      // Recursively trace the scattered ray and multiply by attenuation
      return attenuation*color(scattered, world, depth+1);
    
    // If material doesn't scatter (absorbs all light), return black
    return vec3(0,0,0);
  }

  // Ray didn't hit anything, so render the background gradient
  vec3 unit_direction = unit_vector(r.direction());
  float t = 0.5*(unit_direction.y() + 1.0); // Convert y from [-1,1] to [0,1]
  // Linear interpolation between white and light blue based on ray's y direction
  return (1.0-t)*vec3(1.,1.,1.) + t*vec3(0.5,0.7,1.0);
}

int main()
{
  // Set up output file for PPM format image
  std::ofstream f;
  f.open("output.ppm", std::ios::out);

  // Define rendering parameters
  const int nx = 600;    // Image width in pixels
  const int ny = 400;    // Image height in pixels
  const int ns = 100;    // Number of samples per pixel (for anti-aliasing)
  
  // Set up camera with position, target, up vector, field of view, and aspect ratio
	camera cam(vec3(13,2,3), vec3(0,0,0), vec3(0,1,0), 20, float(nx)/float(ny));
	// Alternative simple camera setup (commented out):
	// camera cam(vec3(0,0,0), vec3(0,0,-1), vec3(0,1,0), 90, float(nx)/ny);

  // Define simple test scene A: two gray spheres
  hitable_list worldA;
  worldA.add(make_shared<sphere>(vec3(0,0,-1),0.5, make_shared<lambertian>(vec3(0.8,0.8,0.8))));
  worldA.add(make_shared<sphere>(vec3(0,-100.5,-1),100, make_shared<lambertian>(vec3(0.3,0.3,0.3))));

  // Define test scene B: multiple spheres with different materials
  hitable_list worldB;
  worldB.add(make_shared<sphere>(vec3(0,0,-1),0.5, make_shared<lambertian>(vec3(0.8,0.3,0.3)))); // Red diffuse
  worldB.add(make_shared<sphere>(vec3(0,-100.5,-1),100, make_shared<lambertian>(vec3(0.8,0.8,0)))); // Yellow ground
  worldB.add(make_shared<sphere>(vec3(1,0,-1),0.5, make_shared<metal>(vec3(0.8,0.6,0.2))));        // Gold metal
  worldB.add(make_shared<sphere>(vec3(-1,0,-1),0.5, make_shared<dielectric>(1.5)));                // Glass

  // Define complex random scene C
  hitable_list worldC = randomScene();

  // Choose which world to render (currently set to the complex random scene)
  hitable_list world = worldC;
  
  // Begin rendering process
  std::cout << "Started rendering..." << std::endl;
  
  // Write PPM header: format, dimensions, max color value
  f << "P3\n" << nx << " " << ny << "\n255\n";
  
  // Render each pixel, starting from top row (j=ny-1) and going down
  for (int j=ny-1; j>=0; j--)
  {
    // Progress indicator
    std::cout << "\rScanlines remaining: " << j << ' ' << std::flush;
    
    // Render each pixel in the current row
    for (int i=0; i<nx; i++)
    {
      vec3 col(0,0,0); // Accumulated color for this pixel
      
      // Multi-sampling for anti-aliasing: take multiple samples per pixel
      #pragma omp parallel for reduction(+:col) // OpenMP parallel processing
      for (int s=0; s<ns; s++)
      {
        // Add random offset to pixel coordinates for sampling
        float u = (i + random_float())/nx; // Horizontal coordinate [0,1]
        float v = (j + random_float())/ny; // Vertical coordinate [0,1]
        
        // Generate ray from camera through this pixel sample
        ray r = cam.get_ray(u,v);
        
        // Trace the ray and accumulate color
        col += color(r, world, 0);
      }
      
      // Average all samples for this pixel
      col /= ns;
      
      // Apply gamma correction (square root) and clamp values
      col[0] = clamp(sqrt(col[0]), 0, 0.999);
      col[1] = clamp(sqrt(col[1]), 0, 0.999);
      col[2] = clamp(sqrt(col[2]), 0, 0.999);
      
      // Convert from [0,1] to [0,255] for PPM format
      col *= 256;
      
      // Write RGB values to file
      f << int(col.e[0]) << " " << int(col.e[1]) << " " << int(col.e[2]) << "\n";
    }
  }
  
  // Clean up and notify completion
  f.close();
  std::cout << "Rendering done!" << std::endl;
}