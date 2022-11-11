#pragma once

#include <memory>
#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "Walnut/Image.h"

#include "Camera.h"
#include "Scene.h"


class Renderer {
public:
	Renderer() = default;

	bool resize(uint32_t, uint32_t);
	void render(const Scene&, const Camera&);
	void renderUnshaded(const Scene&, const Camera&);

	inline std::shared_ptr<Walnut::Image> getOutput() const {
		this->image->SetData(this->buffer);
		return this->image;
	}
	inline void resetAccumulatedFrames() { this->accumulated_frames = 1; }
	inline void updateRandomRays(const Camera& c) {
		c.CalculateRandomDirections(this->rand_rays[0]);
		c.CalculateRandomDirections(this->rand_rays[1]);
		c.CalculateRandomDirections(this->rand_rays[2]);
	}

	bool
		accumulate = true;
	static inline glm::vec3
		SKY_COLOR{ 0.2f };
	static inline int
		MAX_BOUNCES{ 3U },
		SAMPLE_RAYS{ 5U };
	constexpr static inline float
		BRIGHTNESS_CONSTANT{ 100.f },
		NO_COLLIDE_DIST{ 1e-4f };

protected:
	glm::vec4 computePixel(size_t);
	glm::vec4 computeUnshaded(size_t);
	glm::vec3 evaluateRay(const Ray&, size_t = 0U);

private:
	std::shared_ptr<Walnut::Image> image;

	const Scene* active_scene{ nullptr };
	const Camera* active_camera{ nullptr };

	std::array<std::vector<glm::vec3>, 3> rand_rays;

	uint32_t* buffer = nullptr;
	glm::vec4* accumulated_samples = nullptr;
	
	uint32_t accumulated_frames = 1;


};