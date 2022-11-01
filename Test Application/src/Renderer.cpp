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
}
void Renderer::render(const Scene& scene, const Camera& cam) {

	this->active_scene = &scene;
	this->active_camera = &cam;

	uint32_t sz = this->image->GetWidth() * this->image->GetHeight();
	for (uint32_t n = 0; n < sz; n++) {
		this->buffer[n] =
			vec2rgba(glm::clamp(
				this->computePixel(n),
				0.f, 1.f
			));
	}
	this->image->SetData(this->buffer);
	/*for (uint32_t y = 0; y < this->image->GetHeight(); y++) {
		for (uint32_t x = 0; x < this->image->GetWidth(); x++) {
			this->buffer[ x + y * this->image->GetWidth() ] =
				vec2rgba(glm::clamp(
					this->computePixel(x + y * this->image->GetWidth()),
					0.f, 1.f
				));
		}
	}*/

}

glm::vec4 Renderer::computePixel(size_t n) {
	this->raybuff.origin = this->active_camera->GetPosition();
	this->raybuff.direction = this->active_camera->GetRayDirections()[n];

	float amount = 1.f;
	glm::vec3 clr{ 0.f };
	for (size_t b = 0; b < MAX_BOUNCES; b++) {
		RayResult result = this->traceRay(this->raybuff);
		if (result.distance == -1) {
			clr += SKY_COLOR * amount;
			break;
		}
		if (result.is_source) {
			float directness = (float)pow(glm::dot(result.w_normal, -this->raybuff.direction) * 1.25f, 2);
			clr += this->active_scene->lights[result.objectid]->albedo * directness * amount;
			this->raybuff.origin = result.w_position + this->raybuff.direction * NO_COLLIDE_DIST;
			if (directness > 1) {
				amount /= directness;
			}
			continue;
		}
		float lightness = 0.f;
		glm::vec3
			no_collide = result.w_position + result.w_normal * NO_COLLIDE_DIST,
			to_light;
		for (const Object* light : this->active_scene->lights) {
			to_light = glm::normalize(result.w_position - light->position) + this->active_scene->objects[result.objectid]->roughness * Walnut::Random::Vec3(-0.5f, 0.5f);
			float ldist = glm::distance(result.w_position, light->position);
			if (this->traceRay(Ray{ result.w_position, -to_light }).is_source) {
				lightness += glm::max(glm::dot(result.w_normal, -to_light), 0.f) / pow(ldist, 2) * light->roughness * BRIGHTNESS_CONSTANT;
			}
		}
		clamp(lightness, 1.f, 0.f);
		clr += this->active_scene->objects[result.objectid]->albedo * lightness * amount;
		amount *= 0.7;
		this->raybuff.origin = no_collide;
		this->raybuff.direction = glm::reflect(this->raybuff.direction, result.w_normal
			+ this->active_scene->objects[result.objectid]->roughness * Walnut::Random::Vec3(-0.5f, 0.5f)
		);
	}
	return glm::vec4{ clr, 1.f };
}
Renderer::RayResult Renderer::traceRay(const Ray& r) {
	int32_t hit_idx = -1;
	bool islight = false;
	float q, t = std::numeric_limits<float>::max();
	for (size_t i = 0; i < this->active_scene->objects.size(); i++) {
		q = this->active_scene->objects[i]->calcIntersection(r);
		if (q < t && q > 0) {
			t = q;
			hit_idx = (int32_t)i;
		}
	}
	for (size_t i = 0; i < this->active_scene->lights.size(); i++) {
		q = this->active_scene->lights[i]->calcIntersection(r);
		if (q < t && q > 0) {
			t = q;
			hit_idx = (uint32_t)i;
			islight = true;
		}
	}
	if (hit_idx == -1) { return this->traceMiss(r); }
	return this->traceClosest(r, t, hit_idx, islight);
}
Renderer::RayResult Renderer::traceClosest(const Ray& r, float time, int32_t idx, bool light) {
	RayResult result;
	result.is_source = light;
	result.distance = time;
	result.objectid = idx;
	result.w_position = r.origin + r.direction * time;
	result.w_normal = light ?
		this->active_scene->lights[idx]->calcNormal(result.w_position, r.direction) :
		this->active_scene->objects[idx]->calcNormal(result.w_position, r.direction);
	return result;
}
Renderer::RayResult Renderer::traceMiss(const Ray& r) {
	RayResult ret;
	ret.distance = -1;
	return ret;
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
		n = target->calcNormal(hp, ref.direction);

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