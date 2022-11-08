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
	//delete[] this->multisample;
	this->buffer = new uint32_t[w * h];
	//this->multisample = new uint32_t[w * h];
}
void Renderer::render(const Scene& scene, const Camera& cam) {

	this->active_scene = &scene;
	this->active_camera = &cam;

	uint32_t sz = this->image->GetWidth() * this->image->GetHeight();
	for (uint32_t n = 0; n < sz; n++) {
		glm::vec4 clr{0.f};
		for (size_t s = 0; s < SAMPLE_RAYS; s++) {
			clr += this->computePixel(n);
		}
		clr /= SAMPLE_RAYS;
		clr = glm::clamp(
			glm::sqrt(clr),
			0.f, 1.f
		);
		this->buffer[n] = vec2rgba(clr);
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
	RayResult result = this->traceRay(ray);
	if (result.distance == -1) { return glm::vec4{ SKY_COLOR, 1.f }; }
	if (result.is_source) { return glm::vec4{ this->active_scene->lights[result.objectid]->albedo, 1.f }; }
	return glm::vec4{ this->active_scene->objects[result.objectid]->albedo, 1.f };
}
//glm::vec3 Renderer::evaluateRay(const Ray& r, size_t bounce, float a) {
//	float amount = a;
//	float roughness = 0.f;
//	RayResult result;
//	Ray raybuff = r;
//	glm::vec3 clr{ 0.f };
//	for (size_t b = bounce; b < MAX_BOUNCES; b++) {
//		if (roughness > 0.f) {
//			glm::vec3 sum;
//			Ray sample;
//			for (size_t s = 0; s < SAMPLE_RAYS; s++) {
//				sample = raybuff;
//				sample.direction += (roughness * Walnut::Random::Vec3(-0.5f, 0.5f));
//				sum += this->evaluateRay(sample, b, amount);
//			}
//			sum /= SAMPLE_RAYS;
//			return glm::vec4{ sum + clr, 1.f };
//		}
//		else {
//			result = this->traceRay(raybuff);
//			if (result.distance == -1) {
//				clr += SKY_COLOR * amount;
//				break;
//			}
//			if (result.is_source) {
//				float directness = (float)pow(glm::dot(result.w_normal, -raybuff.direction) * 1.25f, 2);
//				clr += this->active_scene->lights[result.objectid]->albedo * directness * amount;
//				raybuff.origin = result.w_position + raybuff.direction * NO_COLLIDE_DIST;
//				if (directness > 1) {
//					amount /= directness;
//				}
//				continue;
//			}
//			float lightness = 0.f;
//			glm::vec3
//				no_collide = result.w_position + result.w_normal * NO_COLLIDE_DIST,
//				to_light;
//			for (const Object* light : this->active_scene->lights) {
//				to_light = glm::normalize(result.w_position - light->position);
//					//+ this->active_scene->objects[result.objectid]->mat->roughness * Walnut::Random::Vec3(-0.5f, 0.5f);
//				float ldist = glm::distance(result.w_position, light->position);
//				if (this->traceRay(Ray{ result.w_position, -to_light }).is_source) {
//					lightness += glm::max(glm::dot(result.w_normal, -to_light), 0.f) / pow(ldist, 2) * light->mat->luminance * BRIGHTNESS_CONSTANT;
//				}
//			}
//			clamp(lightness, 1.f, 0.f);
//			clr += this->active_scene->objects[result.objectid]->albedo * lightness * amount;
//			amount *= 0.7;
//			raybuff.origin = no_collide;
//			raybuff.direction = glm::reflect(raybuff.direction, result.w_normal);
//			//+ this->active_scene->objects[result.objectid]->mat->roughness * Walnut::Random::Vec3(-0.5f, 0.5f));
//			roughness = this->active_scene->objects[result.objectid]->mat->roughness;
//		}
//	}
//	return clr;
//}
//glm::vec3 Renderer::evaluateRay(const Ray& r, size_t b, float a) {
//	glm::vec3 clr{ 0.f };
//	if (b > MAX_BOUNCES) { return clr; }
//	RayResult result = this->traceRay(r);
//	if (result.distance == -1) {	// sky "hit"
//		return SKY_COLOR;
//	}	// object hit
//	// add reflective value - multiplied by (1.f - roughness)
//	Object* obj = this->active_scene->objects[result.objectid];
//	float roughness = obj->mat->roughness;
//	Ray reflected{
//		result.w_position + result.w_normal * NO_COLLIDE_DIST,
//		glm::reflect(r.direction, result.w_normal)
//	};
//	if (roughness < 1.f) {
//		clr += this->evaluateRay(reflected, b + 1) * (1.f - roughness);
//	}
//	// add absorbed value - multiplied by (roughness)
//	if (roughness > 0.f) {
//		glm::vec3 sum{1.f};
//		Ray sample = reflected;
//		float lightness = 0.f;
//		for (const Object* o : this->active_scene->objects) {
//			if (o == obj || o->mat->luminance == 0.f) { continue; }
//			glm::vec3 direct = glm::normalize(o->position - result.w_position);
//			float dist = glm::distance(result.w_position, o->position);
//			sample.direction = direct;
//			if (this->active_scene->objects[this->traceRay(sample).objectid] == o) {
//				sum *= o->albedo;
//				lightness += glm::max(glm::dot(result.w_normal, direct), 0.f) / pow(dist, 2) * o->mat->luminance * BRIGHTNESS_CONSTANT;
//			}
//		}
//		clr += sum * obj->albedo * roughness * lightness;
//		sum = glm::vec3{ 0.f };
//		for (size_t s = 0; s < SAMPLE_RAYS; s++) {
//			sample.direction = reflected.direction + (roughness * Walnut::Random::Vec3(-0.5f, 0.5f));
//			sum += this->evaluateRay(sample, b + 1);
//		}
//		sum /= SAMPLE_RAYS;
//		clr += obj->albedo * sum * roughness;
//	}
//	// TODO: add diffusion calculation
//	glm::vec3 sum;
//	Ray sample{ reflected.origin, result.w_normal };
//	for (size_t s = 0; s < SAMPLE_RAYS; s++) {
//		sample.direction = result.w_normal + Walnut::Random::Vec3(-1.f, 1.f);
//		sum += this->evaluateRay(sample, b + 1);
//	}
//	sum /= SAMPLE_RAYS;
//	clr += obj->albedo * sum;
//	// add emmissive value
//	clr *= (1.f - obj->mat->luminance);
//	clr += obj->albedo * obj->mat->luminance;
//	return clr;
//}
glm::vec3 Renderer::evaluateRay(const Ray& r, size_t b, float a) {
	if (b > MAX_BOUNCES) {
		return glm::vec3{};
	}
	RayResult result = this->traceRay(r);
	if (result.distance == -1) {
		return SKY_COLOR;
	}
	Object* o = this->active_scene->objects[result.objectid];
	if (o->mat->luminance >= 1.f) {
		return o->albedo * o->mat->luminance;
	}
	return
		0.5f * o->albedo * this->evaluateRay(o->mat->scatter(r, Ray{ result.w_position, result.w_normal }), b + 1)
			+ o->albedo * o->mat->luminance/* * BRIGHTNESS_CONSTANT / (float)pow(result.distance, 2)*/
	;
}
Renderer::RayResult Renderer::traceRay(const Ray& r) {
	int32_t hit_idx = -1;
	bool islight = false;
	float q, t = std::numeric_limits<float>::max();
	for (size_t i = 0; i < this->active_scene->objects.size(); i++) {
		q = this->active_scene->objects[i]->intersectionTime(r);
		if (q < t && q > 0) {
			t = q;
			hit_idx = (int32_t)i;
		}
	}
	for (size_t i = 0; i < this->active_scene->lights.size(); i++) {
		q = this->active_scene->lights[i]->intersectionTime(r);
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


//glm::vec4 Renderer::traceRay(const Scene& scene, const Ray& ray) {
//
//	constexpr size_t MAX_BOUNCES = 1;
//
//	const Object* target;
//	bool direct = false;
//	Ray ref = ray;
//	float q, t;	// hit time, "lightness", target hit time
//	glm::vec3 hp, n, clr{0}, l;
//	for (size_t bounce = 0; bounce < MAX_BOUNCES; bounce++) {	// for up to 3 possible reflective bounces
//		target = nullptr;
//		t = std::numeric_limits<float>::max();
//		for (const Object* obj : scene.objects) {		// test interestion of all objects, get closest time
//			q = obj->intersectionTime(ref);
//			if (q < t && q > 0) {
//				t = q;
//				target = obj;
//			}
//		}
//		for (const Object* light : scene.lights) {
//			q = light->intersectionTime(ref);
//			if (q < t && q > 0) {
//				t = q;
//				target = light;
//				direct = true;
//			}
//		}
//		if (!target) { return glm::vec4{ clr, 1.f }; }	// no interestions, return whatever was previously stored in clr (default black)
//
//		hp = ref.origin + ref.direction * t;
//		n = target->calcNormal(hp, ref.direction);
//
//		if (!direct) {
//			float luminance = 0;
//			for (const Object* light : scene.lights) {
//				//l += light->albedo * glm::max(glm::dot(n, -glm::normalize(hp - light->position)), 0.f) * (1.f / ((hp - light->position) * (hp - light->position)));
//				float lm = glm::max(glm::dot(n, -glm::normalize(hp - light->position)), 0.f);
//				l += light->albedo * lm;
//				luminance += lm;
//			}
//			l /= scene.lights.size();
//			clr += l + luminance * target->albedo;
//		} else {
//			clr = target->albedo * (float)pow(glm::dot(n, glm::normalize(ref.origin - hp)), 2) * 2.f;
//			return glm::vec4{ clr, 1.f };
//		}
//
//		if (bounce + 1 < MAX_BOUNCES) {
//			ref.direction = (ref.direction - 2 * glm::dot(ref.direction, n) * n);
//			ref.origin = hp;
//		}
//
//	}
//	return glm::vec4{ clr, 1.f };
//	
//}