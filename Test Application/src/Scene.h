#pragma once

#include <vector>
#include <memory>
#include <initializer_list>

#include <glm/glm.hpp>
#include <Walnut/Random.h>


inline static float sgn(float v) { return (int)(v > 0) - (int)(v < 0); }
inline static glm::vec3 center(std::initializer_list<glm::vec3> pts) {
	glm::vec3 ret;
	for (glm::vec3 pt : pts) {
		ret += pt;
	}
	return (ret /= pts.size());
}
inline static glm::vec3 randomWithinUnitSphere() {
	for (;;) {
		glm::vec3 v = Walnut::Random::Vec3(-1.f, 1.f);
		if (glm::dot(v, v) < 1) {
			return v;
		}
	}
}



struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;
};


struct Material {
	inline Material(float r = 0, float g = 0, float t = 0, float l = 0) :
		roughness(r), glossiness(g), transparency(t), luminance(l) {}
	float
		roughness{ 0.f },
		glossiness{ 0.f },
		transparency{ 0.f },
		refraction_index{ 1.5f },	// make sure to add this to constuctor
		luminance{ 0.f };

	virtual Ray scatter(const Ray& source, const Ray& normal) const;

	static Ray diffuse(const Ray& normal, float factor = 1.f);
	static Ray reflect(const Ray& source, const Ray& normal, float gloss = 0.f);
	static Ray refract(const Ray& source, const Ray& normal, float refr_index = 1.5f);
};
static const Material
	_DEFAULT_MAT{ 0.2f, 0.f, 0.f, 0.f },
	_LIGHT_SOURCE{ 1.f, 0.f, 0.f, 1.f };

struct Object {
	Object(
		glm::vec3 p = glm::vec3{ 0.f },
		glm::vec3 c = glm::vec3{ 1.f },
		const Material* m = &_DEFAULT_MAT
	) : position(p), albedo(c), mat(m) {}
	~Object() = default;

	glm::vec3 position{ 0.f }, albedo{ 1.f };
	const Material* mat;

	inline virtual glm::vec3 getColor(glm::vec3) const { return this->albedo; }
	inline virtual void moveTo(glm::vec3 p) { this->position = p; }
	inline virtual void invokeOptions() {}

	virtual float intersectionTime(const Ray&) const = 0;		// return time of intersection or 0 if none exists
	virtual glm::vec3 calcNormal(glm::vec3 hit, glm::vec3 raydir) const = 0;	// return normal vector at point of intersection

};


struct Sphere : public Object {
	Sphere(
		glm::vec3 p = glm::vec3{ 0.f },
		float r = 0.5f,
		glm::vec3 c = glm::vec3{ 1.f },
		const Material* m = &_DEFAULT_MAT
	) : Object{ p, c, m }, rad(r) {}

	float rad{ 0.5f };

	void invokeOptions() override;
	
	float intersectionTime(const Ray&) const override;
	inline glm::vec3 calcNormal(glm::vec3 hit, glm::vec3) const override
		{ return glm::normalize(hit - this->position); }
};
struct Triangle : public Object {
	Triangle(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
		glm::vec3 c, const Material* m = &_DEFAULT_MAT
	) : Object(center({ p1, p2, p3 }), c, m), p1(p1), p2(p2), p3(p3),
		e1(p2 - p1), e2(p3 - p1), norm(glm::normalize(glm::cross(this->e1, this->e2))) {}

	glm::vec3
		p1, p2, p3,
		e1, e2, norm;

	void moveTo(glm::vec3) override;
	void invokeOptions() override;

	float intersectionTime(const Ray&) const override;
	inline glm::vec3 calcNormal(glm::vec3, glm::vec3 rd) const override
		{ return this->norm * -sgn(glm::dot(this->norm, rd)); }
};
struct Quad : public Object {
	Quad(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4,
		glm::vec3 c = glm::vec3{ 1.f }, const Material* m = &_DEFAULT_MAT
	) : Object(center({ p1, p2, p3, p4 }), c, m), h1(p1, p2, p4, c, m), h2(p3, p2, p4, c, m) {}

	Triangle h1, h2;

	void moveTo(glm::vec3) override;
	void invokeOptions() override;

	float intersectionTime(const Ray&) const override;
	inline glm::vec3 calcNormal(glm::vec3 hit, glm::vec3 rd) const override
		{ return this->h1.calcNormal(hit, rd); }
};

enum ObjType {
	SPHERE = 0,
	TRIANGLE = 1,
	QUAD = 2
};

struct Scene {
	using Obj = std::unique_ptr<Object>;
	inline ~Scene() {
		for (Object* obj : this->objects) { delete obj; }
		for (Object* light : this->lights) { delete light; }
	}

	std::vector<Object*> objects;
	std::vector<int> obj_mats;
	std::vector<Material> materials;

	std::vector<Object*> lights;

};