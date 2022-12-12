#pragma once

#include <memory>
#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
//#include <shared_mutex>

#include <glm/glm.hpp>

#include "Walnut/Image.h"

#include "Camera.h"
#include "Scene.h"


class Renderer {
	struct Properties;
public:
	Renderer() = default;
	Renderer(const Properties& p) : properties(p) {}

	bool resize(uint32_t, uint32_t);
	void render(const Scene&, const Camera&);

	std::shared_ptr<Walnut::Image> getOutput() const;
	//std::shared_ptr<Walnut::Image> getImmediateOutput() const;

	bool invokeGuiOptions();

	inline void resetRender() {
		this->render_interrupt = true;
		this->accumulated_frames = 1;
	}
	inline void resetAccumulation() {
		this->accumulated_frames = 1;
	}

	//inline void resetAccumulatedFrames() { this->accumulated_frames = 1; }
	//inline void updateRandomRays(const Camera& c) { c.CalculateRandomDirections(this->aa_rays); }

	enum {
//		RenderMode_Raw = 0,
		RenderMode_Sync_Frame = 1 << 0,
		RenderMode_Accumulate = 1 << 1,
		RenderMode_AA_Random = 1 << 2,
		RenderMode_Parallelize = 1 << 3,
		RenderMode_Unshaded = 1 << 4,
		RenderMode_Recursive_Samples = 1 << 5
	};
	struct Properties {
		int32_t
			render_flags{ RenderMode_Accumulate },
			bounce_limit{ 5U },
			pixel_samples{ 5U },
			aa_random_rays{ 3U },
			recursive_samples{ 3U }/*,
			cpu_threads{ std::thread::hardware_concurrency() * 0.75f }*/;
	} properties;

	static glm::vec3 evaluateRayAlbedo(const Scene&, const Ray&);	// for rendering without shading
	static glm::vec3 evaluateRay(const Scene&, const Ray&, size_t = 1);		// trace the ray through the scene for x number of bounces
	static glm::vec3 recursivelySampleRay(const Scene&, const Ray&, size_t, size_t = 1);	// samples at each redirect (much more complex, but much more visually robust)


private:
	std::shared_ptr<Walnut::Image> image;

	//std::vector<std::vector<glm::vec3>> aa_rays;	// randomized directions for antialiasing samples

	mutable std::mutex buffer_read_lock, buffer_write_lock, frame_lock;
	//std::shared_mutex buffer_write_lock;
	std::atomic_bool render_interrupt{ false };

	uint32_t* buffer = nullptr;
	glm::vec3* accumulated_samples = nullptr;
	
	uint32_t accumulated_frames = 1;


};