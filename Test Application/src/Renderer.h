#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "Walnut/Image.h"

#include "Camera.h"
#include "Scene.h"


//struct Sphere : glm::vec4 {
//	inline float radius() const { return this->w; }
//	inline glm::vec3 position() const { return glm::vec3(*this); }
//};

struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

class Renderer {
public:
	Renderer() = default;

	void resize(uint32_t, uint32_t);
	void render(const Scene&, const Camera&);

	inline std::shared_ptr<Walnut::Image> getOutput() const { return this->image; }

protected:
	glm::vec4 traceRay(const Scene&, const Ray&);

private:
	std::shared_ptr<Walnut::Image> image;
	uint32_t* buffer = nullptr;
	const glm::vec3
		light{-1.f, -1.f, -1.f},
		color{ 0, 1, 0 }
	;
	float ratio;


};