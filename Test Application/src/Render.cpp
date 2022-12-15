#include "Render.h"

#include <execution>
#include <iterator>
//#include <iostream>

#include <imgui.h>

#include "Walnut/Random.h"


uint32_t vec2rgba(glm::vec4 c) {
	return
		(uint32_t)(c.a * 255.f) << 24 |
		(uint32_t)(c.b * 255.f) << 16 |
		(uint32_t)(c.g * 255.f) << 8 |
		(uint32_t)(c.r * 255.f);
}
uint32_t vec2rgba(glm::vec3 c, float a = 1.f) {
	return
		(uint32_t)(a * 255.f) << 24 |
		(uint32_t)(c.b * 255.f) << 16 |
		(uint32_t)(c.g * 255.f) << 8 |
		(uint32_t)(c.r * 255.f);
}
glm::vec4 rgba2vec(uint32_t* c) {
	uint8_t* _c = reinterpret_cast<uint8_t*>(c);
	return glm::vec4{
		_c[0] / 255.f,
		_c[1] / 255.f,
		_c[2] / 255.f,
		_c[3] / 255.f
	};
}

//bool Renderer::resize(uint32_t w, uint32_t h) {
//
//	this->render_interrupt = true;
//	
//	this->buffer_read_lock.lock();	// disallow both reading and writing while reallocating buffers
//	this->buffer_write_lock.lock();
//
//	if (this->image) {
//		if (this->image->GetWidth() == w && this->image->GetHeight() == h) {
//			this->buffer_read_lock.unlock();
//			this->buffer_write_lock.unlock();
//			return false;
//		}
//		this->image->Resize(w, h);
//	}
//	else {
//		this->image = std::make_shared<Walnut::Image>(w, h, Walnut::ImageFormat::RGBA);
//	}
//
//	delete[] this->buffer;
//	this->buffer = new uint32_t[w * h];
//	delete[] this->accumulated_samples;
//	this->accumulated_samples = new glm::vec3[w * h];
//	this->accumulated_frames = 1;
//
//	this->buffer_read_lock.unlock();
//	this->buffer_write_lock.unlock();
//
//	return true;
//
//}
//std::shared_ptr<Walnut::Image> Renderer::getOutput() const {
//	
//	if (this->properties.render_flags & RenderMode_Sync_Frame) {
//		std::scoped_lock l(this->frame_lock);	// blocks until frame is not being written to
//		return this->image;
//	}
//	if (this->buffer) {
//		this->buffer_read_lock.lock();
//		this->image->SetData(this->buffer);
//		this->buffer_read_lock.unlock();
//	}
//	return this->image;
//
//}
//std::shared_ptr<Walnut::Image> Renderer::getImmediateOutput() const {
//	
//	if (this->sync_lock.try_lock()) {
//		if (this->buffer_read_lock.try_lock()) {
//			if (this->buffer) {
//				this->image->SetData(this->buffer);	// only sets data if able to read buffer and frame is synced
//			}
//			this->buffer_read_lock.unlock();
//		}
//		this->sync_lock.unlock();
//	}
//	return this->image;
//
//}
bool Renderer::invokeGuiOptions() {
	Properties& p = this->properties;
	bool r = false;
	r |= ImGui::CheckboxFlags("Enable VSync", &p.render_flags, RenderMode_Sync_Frame);
	r |= ImGui::CheckboxFlags("Accumulate Frames", &p.render_flags, RenderMode_Accumulate);
	r |= ImGui::CheckboxFlags("Enable MultiSampled AA", &p.render_flags, RenderMode_MultiSample_AA);
	r |= ImGui::CheckboxFlags("Parallelize Rendering", &p.render_flags, RenderMode_Parallelize);
	r |= ImGui::CheckboxFlags("Render Unshaded", &p.render_flags, RenderMode_Unshaded);
	r |= ImGui::CheckboxFlags("Enable Recursive MultiSample", &p.render_flags, RenderMode_Recursive_Samples);
	r |= ImGui::DragInt("Max Bounces", &p.bounce_limit, 1.f, 1, 20, "%d", ImGuiSliderFlags_AlwaysClamp);
	r |= ImGui::DragInt("Total Pixel Samples", &p.pixel_samples, 1.f, 1, 500, "%d", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
	r |= ImGui::DragInt("Recursive Samples", &p.recursive_samples, 1.f, 1, 10, "%d", ImGuiSliderFlags_AlwaysClamp);
	r |= ImGui::DragInt("AntiAlias Samples", &p.antialias_samples, 1.f, 1, 10, "%d", ImGuiSliderFlags_AlwaysClamp);
	if (ImGui::Button("Reset Options")) {
		this->properties = Properties{};
		r = true;
	}
	return r;
}


class IndexIterator {
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = int64_t;
	using difference_type = int64_t;
	using pointer = int64_t*;
	using reference = int64_t&;

	inline IndexIterator(int64_t v = 0) : val(v) {}

	inline value_type operator*() { return this->val; }
	inline IndexIterator& operator++() {
		this->val++;
		return *this;
	}
	inline bool operator==(const IndexIterator& o) {
		return this->val == o.val;
	}
	inline bool operator!=(const IndexIterator& o) {
		return this->val != o.val;
	}

private:
	int64_t val;


};


void Renderer::render(const Scene& scene, RenderStack& stack) {

	this->render_state = RenderState_Render;
	int32_t flags_cache = this->properties.render_flags;

	if (flags_cache & RenderMode_Parallelize) {
		/*std::cout << cam.GetRayDirections().size() << std::endl;*/
		std::for_each(std::execution::par, IndexIterator(0), IndexIterator(stack.height),
			[&scene, &stack, &flags_cache, this](int64_t idx) {		// for each row...
				glm::vec3 clr;
				const int64_t end = stack.width * (idx + 1);
				for (idx *= stack.width; idx < end; idx++) {	// for each pixel in the row...
					while (this->render_state == RenderState_Pause) {
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
					}
					if (this->render_state == RenderState_Cancel) {
						return;
					}
					stack.move_lock.lock_shared();
					stack.resize_lock.lock_shared();
					/*int offset = 0;
					if (flags_cache & RenderMode_MultiSample_AA) {
						offset = (int)stack.accum_ratio[idx][3] % stack.depth;
					}*/
					Ray ray;
					ray.origin = stack.directions[0];
					clr = glm::vec3{ 0.f };
					if (flags_cache & RenderMode_Unshaded) {	// do this comparison outside the loop
						ray.direction = stack.directions[idx * stack.depth + 1];
						clr = evaluateRayAlbedo(scene, ray);
					}
					else {
						bool recursive = flags_cache & RenderMode_Recursive_Samples,
							accumulate = flags_cache & RenderMode_Accumulate,
							multisample = flags_cache & RenderMode_MultiSample_AA;

						if (recursive) {
							if (multisample) {
								for (size_t s = 0; s < this->properties.antialias_samples; s++) {
									clr += recursivelySampleRay(scene, ray, this->properties.recursive_samples, this->properties.bounce_limit);
								}
							}
							clr = recursivelySampleRay(scene, ray, this->properties.recursive_samples, this->properties.bounce_limit);
						} else {
							for (size_t s = 0; s < this->properties.pixel_samples; s++) {
								clr += evaluateRay(scene, ray, this->properties.bounce_limit);
							}
						}

						if (recursive) {
							if (multisample) {
								if (accumulate) {

								}
							}
							if (accumulate) {
								*(glm::vec3*)(stack.accum_ratio + idx) += 
									recursivelySampleRay(scene, ray, this->properties.recursive_samples, this->properties.bounce_limit);
								stack.accum_ratio[idx][3] += 1.f;
							}
						}
						else {
							for (size_t s = 0; s < this->properties.pixel_samples; s++) {
								clr += evaluateRay(scene, ray, this->properties.bounce_limit);
							}
							clr /= this->properties.pixel_samples;
						}
						clr = glm::clamp(clr, 0.f, 1.f);
						if (this->accumulated_frames == 1) {
							this->accumulated_samples[idx] = clr;
						}
						else {
							clr = (this->accumulated_samples[idx] += clr);
							clr /= this->accumulated_frames;
						}
					}
					stack.buffer[idx] = vec2rgba(glm::sqrt(clr), 1.f);
					stack.move_lock.unlock_shared();
					stack.resize_lock.unlock_shared();
				}
			}
		);
	} else {
		uint32_t sz = this->image->GetWidth() * this->image->GetHeight();
		for (uint32_t n = 0; n < sz; n++) {
			if (this->render_interrupt) {
				break;
			}
			Ray ray{
				cam.GetPosition(),
				rays[n]
			};
			glm::vec3 clr{ 0.f };
			if (flags_cache & RenderMode_Unshaded) {	// do this comparison outside the loop
				clr = evaluateRayAlbedo(scene, ray);
			} else {
				if (flags_cache & RenderMode_Recursive_Samples) {	// and this comparison
					for (size_t s = 0; s < this->properties.pixel_samples; s++) {
						clr += recursivelySampleRay(scene, ray, this->properties.pixel_samples, this->properties.bounce_limit);
					}
				} else {
					for (size_t s = 0; s < this->properties.pixel_samples; s++) {
						clr += evaluateRay(scene, ray, this->properties.bounce_limit);
					}
				}
				clr /= this->properties.pixel_samples;
				clr = glm::clamp(clr, 0.f, 1.f);
				if (this->accumulated_frames == 1) {
					this->accumulated_samples[n] = clr;
				} else {
					clr = (this->accumulated_samples[n] += clr);
					clr /= this->accumulated_frames;
				}
			}
			this->buffer[n] = vec2rgba(glm::sqrt(clr), 1.f);
		}
	}
	this->buffer_write_lock.unlock();
	if ((flags_cache & RenderMode_Accumulate) && (~flags_cache & RenderMode_Unshaded) && !this->render_interrupt) {
		this->accumulated_frames++;
	}
	if ((flags_cache & RenderMode_Sync_Frame) && !this->render_interrupt) {
		this->buffer_read_lock.lock();
		this->frame_lock.lock();

		this->image->SetData(this->buffer);
		
		this->buffer_read_lock.unlock();
		this->frame_lock.unlock();
	}

	/*if (flags_cache & RenderMode_Sync_Frame) {
		this->sync_lock.unlock();
	}*/

}

glm::vec3 Renderer::evaluateRayAlbedo(const Scene& s, const Ray& r) {
	Hit h;
	if (const Interactable* o = s.interacts(r, h)) {
		return o->albedo(h);
	}
	return s.albedo(h);
}
glm::vec3 Renderer::evaluateRay(const Scene& s, const Ray& r, size_t b) {
	Hit hit;
	if (const Interactable* obj = s.interacts(r, hit)) {
		float lum = obj->emmission(hit);
		glm::vec3 clr = obj->albedo(hit);
		if (b == 0 || ((clr.r + clr.g + clr.b) / 3.f * lum) >= 1.f) {
			return clr * lum;
		}
		Ray redirect;
		if (obj->redirect(r, hit, redirect)) {
			return clr * (evaluateRay(s, redirect, b - 1) + lum);
		}
	}
	return s.albedo(hit);
}
glm::vec3 Renderer::recursivelySampleRay(const Scene& scene, const Ray& ray, size_t samples, size_t b) {
	Hit hit;
	if (const Interactable* obj = scene.interacts(ray, hit)) {
		float lum = obj->emmission(hit);
		glm::vec3 clr = obj->albedo(hit);
		if (b == 0 || ((clr.r + clr.g + clr.b) / 3.f * lum) >= 1.f) {
			return clr * lum;
		}
		Ray redirect;
		glm::vec3 sum;
		for (size_t s = 0; s < samples; s++) {
			if (!obj->redirect(ray, hit, redirect)) {
				if (!s) {
					return scene.albedo(hit);
				}
				samples = s;
				break;
			}
			sum += recursivelySampleRay(scene, redirect, samples, b - 1);
		}
		sum /= samples;
		return clr * (sum + lum);
	}
	return scene.albedo(hit);
}