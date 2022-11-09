#pragma once

#include <vector>
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
struct Interaction {
	bool front_intersect;
	float ptime;	// time along source ray
	Ray hit_normal;	// normal with source at the hit point
};


class Interactable {
public:
	inline virtual glm::vec3 getAlbedo(glm::vec2 uv = glm::vec2{}) const { return glm::vec3{ 1.f }; }
	inline virtual float getLuminance(glm::vec2 uv = glm::vec2{}) const { return 0.f; }
	inline virtual void invokeGuiOptions() {}
	virtual bool calcIntersection(
		const Ray& source, Interaction& hit,
		float t_min = 0.f, float t_max = std::numeric_limits<float>::infinity()
	) = 0;
	virtual void calcInteraction(
		const Ray& source, const Interaction& hit, Ray& redirected
	) = 0;

};

class Material {
public:
	Material(float r = 1.f, float g = 0.f, float t = 0.f, float l = 0.f, float ir = 1.f);
	Material(const Material&);
	virtual ~Material();
		
	float
		roughness, glossiness,
		transparency, refraction_index,
		luminance;
	inline size_t idx() const { return this->index; }
	virtual Ray scatter(const Ray& source, const Interaction& hit) const;
	virtual void invokeGui();

	static Ray diffuse(const Ray& normal, float factor = 1.f);
	static Ray reflect(const Ray& source, const Interaction& hit, float gloss = 0.f);
	static Ray refract(const Ray& source, const Interaction& hit, float refr_index = 1.5f, float gloss = 0.f);
	
	static void invokeManagerGui();
	inline static Material* getMat(size_t i) { return materials.at(i); }
	inline static size_t numMats() { return materials.size(); }

private:
	inline static std::vector<Material*> materials;
	size_t index{(size_t)-1};

};
static const Material
	_MAT_DEFAULT{ 1.f, 0.f, 0.f, 0.f },
	_MAT_LIGHT{ 1.f, 0.f, 0.f, 5.f };




//struct Object {
//	Object(
//		glm::vec3 p = glm::vec3{ 0.f },
//		glm::vec3 c = glm::vec3{ 1.f },
//		const Material* m = &_MAT_DEFAULT
//	) : position(p), albedo(c), mat(m) {}
//	~Object() = default;
//
//	glm::vec3 position{ 0.f }, albedo{ 1.f };
//	const Material* mat;
//
//	inline virtual glm::vec3 getColor(glm::vec3) const { return this->albedo; }
//	inline virtual void moveTo(glm::vec3 p) { this->position = p; }
//	inline virtual void invokeOptions() {}
//
//	virtual float intersectionTime(const Ray&) const = 0;		// return time of intersection or 0 if none exists
//	virtual glm::vec3 calcNormal(glm::vec3 hit, glm::vec3 raydir) const = 0;	// return normal vector at point of intersection
//
//};


class Sphere : public Interactable {
public:
	inline Sphere(
		glm::vec3 p = glm::vec3{ 0.f },
		float r = 0.5f,
		glm::vec3 c = glm::vec3{ 1.f },
		const Material* m = &_MAT_DEFAULT
	) : position(p), albedo(c), mat(m), rad(r) {}

	glm::vec3 position, albedo;
	float rad{ 0.5f };
	const Material* mat;

	inline virtual glm::vec3 getAlbedo(glm::vec2 uv = glm::vec2{}) const override
		{ return this->albedo; }
	inline virtual float getLuminance(glm::vec2 uv = glm::vec2{}) const override
		{ return this->mat->luminance; }
	void invokeGuiOptions() override;
	
	virtual bool calcIntersection(
		const Ray& source, Interaction& hit,
		float t_min = 0.f, float t_max = std::numeric_limits<float>::infinity()
	) override;
	inline virtual void calcInteraction(const Ray& source, const Interaction& hit, Ray& redirected) override
		{ redirected = this->mat->scatter(source, hit); }


};
class Triangle : public Interactable {
public:
	Triangle(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
		glm::vec3 c, const Material* m = &_MAT_DEFAULT
	) : position(center({ p1, p2, p3 })), albedo(c), mat(m), p1(p1), p2(p2), p3(p3),
		e1(p2 - p1), e2(p3 - p1), norm(glm::normalize(glm::cross(this->e1, this->e2))) {}

	glm::vec3 position, albedo;
	glm::vec3
		p1, p2, p3,
		e1, e2, norm;
	const Material* mat;

	void move(glm::vec3);
	inline virtual glm::vec3 getAlbedo(glm::vec2 uv = glm::vec2{}) const override
		{ return this->albedo; }
	inline virtual float getLuminance(glm::vec2 uv = glm::vec2{}) const override
		{ return this->mat->luminance; }
	void invokeGuiOptions() override;

	virtual bool calcIntersection(
		const Ray& source, Interaction& hit,
		float t_min = 0.f, float t_max = std::numeric_limits<float>::infinity()
	) override;
	inline virtual void calcInteraction(const Ray& source, const Interaction& hit, Ray& redirected) override
		{ redirected = this->mat->scatter(source, hit); }
	/*inline glm::vec3 calcNormal(glm::vec3, glm::vec3 rd) const override
		{ return this->norm * -sgn(glm::dot(this->norm, rd)); }*/


};
class Quad : public Interactable {
public:
	Quad(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4,
		glm::vec3 c = glm::vec3{ 1.f }, const Material* m = &_MAT_DEFAULT
	) : h1(p1, p2, p4, c, m), h2(p3, p2, p4, c, m) {}

	Triangle h1, h2;

	void move(glm::vec3);
	inline virtual glm::vec3 getAlbedo(glm::vec2 uv = glm::vec2{}) const override
		{ return this->h1.albedo; }
	inline virtual float getLuminance(glm::vec2 uv = glm::vec2{}) const override
		{ return this->h1.mat->luminance; }
	void invokeGuiOptions() override;

	inline virtual bool calcIntersection(
		const Ray& source, Interaction& hit,
		float t_min = 0.f, float t_max = std::numeric_limits<float>::infinity()
	) override {
		return this->h1.calcIntersection(source, hit, t_min, t_max) || this->h2.calcIntersection(source, hit, t_min, t_max);
	}
	inline virtual void calcInteraction(const Ray& source, const Interaction& hit, Ray& redirected) override
		{ redirected = this->h1.mat->scatter(source, hit); }


};


struct Scene {
	inline Scene(std::initializer_list<Interactable*> objs) :
		objects(objs) {}
	inline ~Scene() {
		for (Interactable* obj : this->objects) { delete obj; }
	}

	std::vector<Interactable*> objects;

};