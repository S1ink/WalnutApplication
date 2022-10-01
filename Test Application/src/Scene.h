#pragma once

#include <vector>
#include <memory>
#include <initializer_list>
#include <glm/glm.hpp>


inline float sgn(float v) { return (int)(v > 0) - (int)(v < 0); }

struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};

struct Object {
	Object(glm::vec3 p = glm::vec3{ 0.f }, glm::vec3 c = glm::vec3{ 1.f }, float a = 0.5f) :
		position(p), albedo(c), alpha(a) {}
	~Object() = default;

	glm::vec3 position{ 0.f }, albedo{ 1.f };
	float alpha{ 0.5 };			// represents reflectivity for objects or "brightness" for light sources

	virtual float calcIntersection(const Ray&) const = 0;		// return time of intersection or 0 if none exists
	virtual glm::vec3 calcNormal(glm::vec3 hit, glm::vec3 raydir) const = 0;	// return normal vector at point of intersection
};

struct Sphere : public Object {
	Sphere(glm::vec3 p = glm::vec3{0.f}, glm::vec3 c = glm::vec3{ 1.f }, float rad = 0.5f, float a = 0.5f) :
		Object{ p, c, a }, rad(rad) {}

	float rad{ 0.5f };
	
	inline float calcIntersection(const Ray& ray) const override {
		glm::vec3 o = ray.origin - this->position;
		float
			a = glm::dot(ray.direction, ray.direction),
			b = 2.f * glm::dot(o, ray.direction),
			c = glm::dot(o, o) - (this->rad * this->rad),
			d = (b * b) - 4.f * a * c;
		if (d < 0.f) { return 0; }
		return (sqrt(d) + b) / (-2.f * a);
	}
	inline glm::vec3 calcNormal(glm::vec3 hit, glm::vec3) const override {
		return glm::normalize(hit - this->position);
	}
};

struct Triangle : public Object {
	inline static glm::vec3 center(std::initializer_list<glm::vec3> pts) {
		glm::vec3 ret;
		for (glm::vec3 pt : pts) {
			ret += pt;
		}
		return (ret /= pts.size());
	}

	Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 c, float a = 0.5f) :
		Object(center({p1, p2, p3}),c, a),
		p1(p1), p2(p2), p3(p3), e1(p2 - p1), e2(p3 - p1),
		norm(glm::normalize(glm::cross(this->e1, this->e2))) {}

	glm::vec3 p1, p2, p3, e1, e2, norm;

	inline float calcIntersection(const Ray& ray) const override {
		constexpr float EPSILON = 1e-5f;
		glm::vec3 h, s, q;
		float a, f, u, v;

		h = glm::cross(ray.direction, this->e2);
		a = glm::dot(this->e1, h);
		if (a > -EPSILON && a < EPSILON) { return 0; }

		f = 1.f / a;
		s = ray.origin - p1;
		u = f * glm::dot(s, h);
		if (u < 0.f || u > 1.f) { return 0; }

		q = glm::cross(s, this->e1);
		v = f * glm::dot(ray.direction, q);
		if (v < 0.f || u + v > 1.f) { return 0; }

		float t = f * glm::dot(this->e2, q);
		if (t > EPSILON) {
			return t;
		}
		return 0.f;
	}
	inline glm::vec3 calcNormal(glm::vec3 hit, glm::vec3 rd) const override {
		return this->norm * -sgn(glm::dot(this->norm, rd));
	}
};

struct Scene {
	using Obj = std::unique_ptr<Object>;
	inline ~Scene() {
		for (Object* obj : this->objects) {
			delete obj;
		}
		for (Object* light : this->lights) {
			delete light;
		}
	}

	std::vector<Object*> objects;
	std::vector<Object*> lights;

};