#version 450

#define MAX_SPHERES 10
#define MAX_LIGHTS 10

struct Sphere {
  vec3 position;
  float radius;
  vec3 color;
  vec3 normal;
};

struct Light {
  vec3 position;
  vec3 color;
};

layout (local_size_x = 16, local_size_y = 16) in;
layout (rgba8, binding = 0) uniform image2D outputImage;

uniform Sphere spheres[MAX_SPHERES];
uniform int numSpheres;

uniform Light lights[MAX_LIGHTS];
uniform int numLights;

void main() {
  ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
  vec3 rayDirection = normalize(vec3(storePos, 0) - vec3(0.5 * imageSize(outputImage), 0));

  vec3 color = vec3(0);
  for (int i = 0; i < numSpheres; i++) {
    Sphere sphere = spheres[i];
    vec3 sphereToRay = rayDirection - sphere.position;
    float a = dot(rayDirection, rayDirection);
    float b = 2.0 * dot(sphereToRay, rayDirection);
    float c = dot(sphereToRay, sphereToRay) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0 * a * c;
    if (discriminant > 0) {
      float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
      float t2 = (-b + sqrt(discriminant)) / (2.0 * a);
      float t = min(t1, t2);
      vec3 hitPos = rayDirection * t;

      // Calculate the normal at the hit position
      vec3 hitNormal = normalize(hitPos - sphere.position);

      for (int j = 0; j < numLights; j++) {
        Light light = lights[j];
        vec3 lightDir = normalize(light.position - hitPos);
        float lightDistance = distance(light.position, hitPos);
        vec3 lightColor = light.color / (lightDistance * lightDistance);

        // Phong shading
        vec3 halfwayDir = normalize(lightDir + rayDirection);
        float specular = pow(max(dot(hitNormal, halfwayDir), 0.0), 16.0);
        color += sphere.color * lightColor * (0.2 + 0.8 * max(dot(hitNormal, lightDir), 0.0)) + specular;
      }
    }
  }

  imageStore(outputImage, storePos, vec4(color, 1.0));
}
