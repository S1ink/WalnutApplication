#pragma once

#include <memory>
#include <array>
#include <vector>
#include <thread>
#include <shared_mutex>
#include <atomic>
//#include <shared_mutex>

#include <glm/glm.hpp>

#include "Walnut/Image.h"

#include "Camera.h"
#include "Scene.h"


struct RenderStack {
	glm::vec3* directions{ nullptr };	// for size width * height * depth + 1, the first vec3 is the cam origin
	glm::vec4* accum_ratio{ nullptr };	// vec3 for accum color, extra float for dividend
	uint32_t* buffer{ nullptr };	// output pixel buffer

	uint32_t width{ 0 }, height{ 0 }, depth{ 0 };	// dimensions of buffers
	std::shared_mutex move_lock, resize_lock;	// mutex for camera move (only reallocs rays) and mutex for resize (reallocs everything)
};

class Renderer {
	struct Properties;
public:
	Renderer() = default;
	Renderer(const Properties& p) : properties(p) {}

	void render(const Scene&, RenderStack&);

	//std::shared_ptr<Walnut::Image> getOutput() const;
	//std::shared_ptr<Walnut::Image> getImmediateOutput() const;

	bool invokeGuiOptions();

	inline void pauseRender() { this->render_state = RenderState_Pause; }
	inline void resumeRender() { this->render_state = RenderState_Render; }
	inline void cancelRender() { this->render_state = RenderState_Cancel; }

	/*inline void resetRender() {
		this->render_interrupt = true;
		this->accumulated_frames = 1;
	}
	inline void resetAccumulation() {
		this->accumulated_frames = 1;
	}*/

	//inline void resetAccumulatedFrames() { this->accumulated_frames = 1; }
	//inline void updateRandomRays(const Camera& c) { c.CalculateRandomDirections(this->aa_rays); }

	enum {
		RenderState_Render = 0,
		RenderState_Cancel = 1,
		RenderState_Pause = 2
	};
	enum {
//		RenderMode_Raw = 0,
		RenderMode_Sync_Frame = 1 << 0,
		RenderMode_Accumulate = 1 << 1,
		RenderMode_MultiSample_AA = 1 << 2,
		RenderMode_Parallelize = 1 << 3,
		RenderMode_Unshaded = 1 << 4,
		RenderMode_Recursive_Samples = 1 << 5
	};
	struct Properties {
		int32_t
			render_flags{ RenderMode_Accumulate },
			bounce_limit{ 5U },
			pixel_samples{ 5U },
			recursive_samples{ 3U },
			antialias_samples{ 3U }/*,
			cpu_threads{ std::thread::hardware_concurrency() * 0.75f }*/;
	} properties;

	static glm::vec3 evaluateRayAlbedo(const Scene&, const Ray&);	// for rendering without shading
	static glm::vec3 evaluateRay(const Scene&, const Ray&, size_t = 1);		// trace the ray through the scene for x number of bounces
	static glm::vec3 recursivelySampleRay(const Scene&, const Ray&, size_t, size_t = 1);	// samples at each redirect (much more complex, but much more visually robust)
																	// ^ how many recursive samples
private:
	std::atomic_int render_state{ RenderState_Render };


};