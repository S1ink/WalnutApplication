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
	
	struct RayResult {
		bool is_source{ false };
		int32_t objectid;
		float distance;
		glm::vec3
			w_position,
			w_normal;
	};
	glm::vec4 computePixel(size_t);
	RayResult traceRay(const Ray&);
	RayResult traceClosest(const Ray&, float, int32_t, bool = false);
	RayResult traceMiss(const Ray&);


private:
	std::shared_ptr<Walnut::Image> image;

	const Scene* active_scene{ nullptr };
	const Camera* active_camera{ nullptr };

	uint32_t* buffer = nullptr;

	Ray raybuff;


};