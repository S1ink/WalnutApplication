#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "Walnut/Image.h"

#include "Camera.h"
#include "Scene.h"



class Renderer {
public:
	Renderer() = default;

	void resize(uint32_t, uint32_t);
	void render(const Scene&, const Camera&);
	void renderUnshaded(const Scene&, const Camera&);

	inline std::shared_ptr<Walnut::Image> getOutput(bool has_moved = true) const {
		/*size_t area = this->image->GetHeight() * this->image->GetWidth();
		if (has_moved) {
			memcpy(this->multisample, this->buffer, area);
		} else {
			for (size_t i = 0; i < area; i++) {
				uint64_t d = this->multisample[i] + this->buffer[i];
				this->multisample[i] = d / 2;
			}
		}*/
		this->enable_resampling = !has_moved;
		if (!this->enable_resampling) { this->consecutive_frames = 0; }
		this->image->SetData(this->buffer);
		return this->image;
	}

	static inline glm::vec3 SKY_COLOR{ 0.2f };
	static inline int
		MAX_BOUNCES{ 3U },
		SAMPLE_RAYS{ 5U };

protected:
	//glm::vec4 traceRay(const Scene&, const Ray&);
	
	/*struct RayResult {
		bool is_source{ false };
		int32_t objectid{ -1 };
		float distance{ -1 };
		glm::vec3
			w_position,
			w_normal;
	};*/
	glm::vec4 computePixel(size_t);
	glm::vec4 computeUnshaded(size_t);
	glm::vec3 evaluateRay(const Ray&, size_t = 0U);
	//Interactable* traceRay(const Ray&, Hit&);


private:
	/*constexpr static inline glm::vec3 SKY_COLOR{ 0.f };
	constexpr static inline size_t
		MAX_BOUNCES{ 3U },
		SAMPLE_RAYS{ 10U };*/
	constexpr static inline float
		BRIGHTNESS_CONSTANT{ 100.f },
		NO_COLLIDE_DIST{ 1e-4f };

	std::shared_ptr<Walnut::Image> image;

	const Scene* active_scene{ nullptr };
	const Camera* active_camera{ nullptr };

	uint32_t* buffer = nullptr;
	mutable uint32_t consecutive_frames{ 0 };
	mutable bool enable_resampling{true};
	//uint32_t* multisample = nullptr;


};