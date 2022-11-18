#include "Renderer.h"

#include "Walnut/Random.h"


uint32_t vec2rgba(glm::vec4 c) {
	return
		(uint32_t)(c.a * 255.f) << 24 |
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

bool Renderer::resize(uint32_t w, uint32_t h) {
	if (this->image) {
		if (this->image->GetWidth() == w && this->image->GetHeight() == h) {
			return false;
		}
		this->image->Resize(w, h);
	}
	else {
		this->image = std::make_shared<Walnut::Image>(w, h, Walnut::ImageFormat::RGBA);
	}

	delete[] this->buffer;
	this->buffer = new uint32_t[w * h];
	delete[] this->accumulated_samples;
	this->accumulated_samples = new glm::vec4[w * h];

	return true;

}
void Renderer::render(const Scene& scene, const Camera& cam) {

	this->active_scene = &scene;
	this->active_camera = &cam;

	uint32_t sz = this->image->GetWidth() * this->image->GetHeight();
	for (uint32_t n = 0; n < sz; n++) {
		/*glm::vec4 clr = this->computePixel(n);
		Ray r;
		r.origin = this->active_camera->GetPosition();
		for (size_t s = 0; s < 3; s++) {
			r.direction = this->rand_rays[s][n];
			clr += glm::vec4{ this->evaluateRay(r), 1.f };
		}
		clr /= 4;*/
		glm::vec4 clr{ 0.f };
		for (size_t s = 0; s < SAMPLE_RAYS; s++) {
			clr += this->computePixel(n);
		}
		clr /= SAMPLE_RAYS;
		clr = glm::clamp(clr, 0.f, 1.f);
		if (this->accumulated_frames == 1) {
			this->accumulated_samples[n] = clr;
		} else {
			clr = (this->accumulated_samples[n] += clr);
			clr /= this->accumulated_frames;
		}
		this->buffer[n] = vec2rgba(glm::sqrt(clr));
	}
	if (this->accumulate) {
		this->accumulated_frames++;
	}

}
void Renderer::renderUnshaded(const Scene& scene, const Camera& cam) {

	this->active_scene = &scene;
	this->active_camera = &cam;

	uint32_t sz = this->image->GetWidth() * this->image->GetHeight();
	for (uint32_t n = 0; n < sz; n++) {
		this->buffer[n] =
			vec2rgba(glm::clamp(
				this->computeUnshaded(n),
				0.f, 1.f
			));
	}

}

glm::vec4 Renderer::computePixel(size_t n) {
	Ray ray{
		this->active_camera->GetPosition(),
		this->active_camera->GetRayDirections()[n]
	};
	return glm::vec4{ this->evaluateRay(ray), 1.f };
}
glm::vec4 Renderer::computeUnshaded(size_t n) {
	Ray ray{
		this->active_camera->GetPosition(),
		this->active_camera->GetRayDirections()[n]
	};
	Hit hit;
	if(const Interactable* obj = this->active_scene->interacts(ray, hit)) {
		return glm::vec4{ obj->albedo(hit), 1.f };
	}
	return glm::vec4{ this->active_scene->albedo(hit), 1.f };
}

glm::vec3 Renderer::evaluateRay(const Ray& r, size_t b) {
	if (b > MAX_BOUNCES) { return glm::vec3{}; }
	Hit hit;
	if (const Interactable* obj = this->active_scene->interacts(r, hit)) {
		float lum = obj->emmission(hit);
		glm::vec3 clr = obj->albedo(hit);
		if (((clr.r + clr.g + clr.b) / 3.f * lum) >= 1.f) {
			return clr * lum;
		}
		Ray redirect;
		if (obj->redirect(r, hit, redirect)) {
			return clr * (this->evaluateRay(redirect, b + 1) + lum);
		}
	}
	return this->active_scene->albedo(hit);
}