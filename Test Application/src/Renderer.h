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

class Renderer {
public:
	Renderer() = default;

	void resize(uint32_t, uint32_t);
	void render(const Scene&, const Camera&);

	inline std::shared_ptr<Walnut::Image> getOutput() const { return this->image; }

protected:
	glm::vec4 traceRay(const Scene&, const Ray&);
	/*struct HitInfo {
		float distance;
		glm::vec3 world_pos;
		glm::vec3 world_normal;

		uint32_t obj_index;
	};

	glm::vec4 compPixel();

	HitInfo traceRay(const Ray&);
	HitInfo closestHit(const Ray&, float, uint32_t);
	HitInfo miss(const Ray&);*/


private:
	std::shared_ptr<Walnut::Image> image;

	const Scene* active_scene{ nullptr };
	const Camera* active_camera{ nullptr };

	uint32_t* buffer = nullptr;
	float ratio;


};