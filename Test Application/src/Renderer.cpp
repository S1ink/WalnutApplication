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

	const Sphere* target;
	Ray ref = ray;
	float a, b, c, d, q, l, t;	// a, b, c, discriminent, hit time, "lightness", target hit time
	glm::vec3 o, hp, n, clr{0};
	for (size_t bounce = 0; bounce < 3; bounce++) {	// for up to 3 possible reflective bounces
		target = nullptr;
		t = std::numeric_limits<float>::max();
		for (const Sphere& obj : scene.spheres) {		// test interestion of all objects, get closest time
			o = ref.origin - obj.position;
			a = glm::dot(ref.direction, ref.direction);
			b = 2.f * glm::dot(o, ref.direction);
			c = glm::dot(o, o) - (obj.rad * obj.rad);
			d = (b * b) - 4.f * a * c;
			if (d < 0.f) { continue; }	// no interestion, continue to next obj

			float q = (-sqrtf(d) - b) / (2 * a);	// closest hit time
			// float q1 = (sqrtf(disc) - b) / (2 * a),	// secondary hit - other part of quad

			if (q < t && q > 0) {
				t = q;
				target = &obj;
			}
		}
		if (target == nullptr) { return glm::vec4{ clr, 1.f }; }	// no interestions, return whatever was previously stored in clr (default black)

		o = ref.origin - target->position;
		hp = o + ref.direction * t;
		n = glm::normalize(hp);		// normalized hit vector

		l = glm::max(glm::dot(n, -glm::normalize(this->light + target->position)), 0.f);
		clr += target->albedo * l * ((3 - bounce) / 3.f);

		if (bounce + 1 < 3) {
			glm::vec3 norm = glm::normalize(hp);
			ref.direction = (ref.direction - 2 * glm::dot(ref.direction, norm) * norm);
			ref.origin = (hp);
		}

	}
	return glm::vec4{ clr, 1.f };
	
}