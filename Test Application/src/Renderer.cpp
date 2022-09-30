#include "Renderer.h"

#include "Walnut/Random.h"


template<typename t>
t clamp(t n, t h, t l) {
	return n >= h ? h : n <= l ? l : n;
}

uint32_t vec2rgba(glm::vec4 c) {
	return
		(uint32_t)(c.a * 255.f) << 24 |
		(uint32_t)(c.b * 255.f) << 16 |
		(uint32_t)(c.g * 255.f) << 8 |
		(uint32_t)(c.r * 255.f);
}

void Renderer::resize(uint32_t w, uint32_t h) {
	if (this->image) {
		if (this->image->GetWidth() == w && this->image->GetHeight() == h) {
			return;
		}
		this->image->Resize(w, h);
	}
	else {
		this->image = std::make_shared<Walnut::Image>(w, h, Walnut::ImageFormat::RGBA);
	}

	delete[] this->buffer;
	this->buffer = new uint32_t[w * h];
	this->ratio = w / (float)h;
}
void Renderer::render(const Scene& scene, const Camera& cam) {

	Ray r{ cam.GetPosition(), glm::vec3{} };
	for (uint32_t y = 0; y < this->image->GetHeight(); y++) {
		for (uint32_t x = 0; x < this->image->GetWidth(); x++) {

			r.direction = cam.GetRayDirections()[x + y * this->image->GetWidth()];
			this->buffer[ x + y * this->image->GetWidth() ] =
				vec2rgba(glm::clamp(this->traceRay(scene, r), 0.f, 1.f));
		}
	}
	this->image->SetData(this->buffer);
}
glm::vec4 Renderer::traceRay(const Scene& scene, const Ray& ray) {

	constexpr size_t MAX_BOUNCES = 1;

	const Object* target;
	bool direct = false;
	Ray ref = ray;
	float q, t;	// hit time, "lightness", target hit time
	glm::vec3 hp, n, clr{0}, l;
	for (size_t bounce = 0; bounce < MAX_BOUNCES; bounce++) {	// for up to 3 possible reflective bounces
		target = nullptr;
		t = std::numeric_limits<float>::max();
		for (const Object* obj : scene.objects) {		// test interestion of all objects, get closest time
			q = obj->calcIntersection(ref);
			if (q < t && q > 0) {
				t = q;
				target = obj;
			}
		}
		for (const Object* light : scene.lights) {
			q = light->calcIntersection(ref);
			if (q < t && q > 0) {
				t = q;
				target = light;
				direct = true;
			}
		}
		if (!target) { return glm::vec4{ clr, 1.f }; }	// no interestions, return whatever was previously stored in clr (default black)

		hp = ref.origin + ref.direction * t;
		n = target->calcNormal(hp);

		if (!direct) {
			float luminance = 0;
			for (const Object* light : scene.lights) {
				//l += light->albedo * glm::max(glm::dot(n, -glm::normalize(hp - light->position)), 0.f) * (1.f / ((hp - light->position) * (hp - light->position)));
				float lm = glm::max(glm::dot(n, -glm::normalize(hp - light->position)), 0.f);
				l += light->albedo * lm;
				luminance += lm;
			}
			l /= scene.lights.size();
			clr += l + luminance * target->albedo;
		} else {
			clr = target->albedo * (float)pow(glm::dot(n, glm::normalize(ref.origin - hp)), 2) * 2.f;
			return glm::vec4{ clr, 1.f };
		}

		if (bounce + 1 < MAX_BOUNCES) {
			ref.direction = (ref.direction - 2 * glm::dot(ref.direction, n) * n);
			ref.origin = hp;
		}

	}
	return glm::vec4{ clr, 1.f };
	
}