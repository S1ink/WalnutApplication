#pragma once

#include <vector>
#include <glm/glm.hpp>


struct Sphere {
	glm::vec3 position{ 0.f };
	float rad{ 0.5f };
	glm::vec3 albedo{ 1.f };

};

struct Scene {
	std::vector<Sphere> spheres;

};