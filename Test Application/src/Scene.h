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
//inline static glm::vec3 randomWithinUnitSphere() {	// use Walnut::Random::InUnitSphere()
//	for (;;) {
//		glm::vec3 v = Walnut::Random::Vec3(-1.f, 1.f);
//		if (glm::dot(v, v) < 1) {
//			return v;
//		}
//	}
//}

class Material;

struct Ray {
	glm::vec3 origin{0.f};
	glm::vec3 direction{0.f};
};
struct Hit {
	bool reverse_intersect{false};	// the normal is on the "inside" of the surface
	float ptime{0.f};		// time along source ray
	Ray normal{};			// normal with origin at the hit point
	const Material* surface{nullptr};	// the surface that was hit
};


class Interactable {
public:
	virtual bool interacts(
		const Ray& source, Hit& hit,
		float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()
	) const = 0;
	inline virtual void invokeGuiOptions() {}
};
class Material {
public:
	virtual bool redirect(const Ray& source, const Hit& interaction, Ray& redirected) const = 0;
	virtual float gamma() const { return 0.f; }
	inline virtual void invokeGuiOptions() {}
};
//class Texture {
//public:
//
//};



class PhysicalBase : public Material {
public:
	inline PhysicalBase(
		float roughness = 1.f,
		float glossness = 0.f,
		float transparency = 0.f,
		float luminance = 0.f,
		float refr_index = 1.f
	) :
		roughness(roughness), glossiness(glossness), transparency(transparency), luminance(luminance), refraction_index(refr_index)
	{}

	static const std::unique_ptr<Material> DEFAULT, LIGHT;

	float roughness, glossiness, transparency, refraction_index, luminance;

	virtual bool redirect(const Ray& source, const Hit& interaction, Ray& redirected) const override;
	inline virtual float gamma() const { return this->luminance; }
	virtual void invokeGuiOptions() override;

	static bool diffuse(const Ray& normal, Ray& redirect);
	static bool reflect(const Ray& source, const Hit& hit, Ray& redirect, float gloss = 0.f);
	static bool refract(const Ray& source, const Hit& hit, float refr_index, Ray& redirect, float gloss = 0.f);

};

//class Material {
//public:
//	Material(float r = 1.f, float g = 0.f, float t = 0.f, float l = 0.f, float ir = 1.f);
//	Material(const Material&);
//	virtual ~Material();
//		
//	float
//		roughness, glossiness,
//		transparency, refraction_index,
//		luminance;
//
//	static const Material
//		_DEFAULT, _LIGHT;
//
//	inline size_t idx() const { return this->index; }
//	virtual Ray scatter(const Ray& source, const Hit& hit) const;
//	virtual void invokeGui();
//
//	static Ray diffuse(const Ray& normal, float factor = 1.f);
//	static Ray reflect(const Ray& source, const Hit& hit, float gloss = 0.f);
//	static Ray refract(const Ray& source, const Hit& hit, float refr_index = 1.5f, float gloss = 0.f);
//	
//	static void invokeManagerGui();
//	inline static Material* getMat(size_t i) { return materials.at(i); }
//	inline static size_t numMats() { return materials.size(); }
//
//private:
//	inline static std::vector<Material*> materials;
//	size_t index{(size_t)-1};
//
//};




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
		const Material* m = PhysicalBase::DEFAULT.get()
	) :
		position(p), albedo(c), mat(m), rad(r)
	{}

	glm::vec3 position, albedo;
	float rad{ 0.5f };
	const Material* mat;

	virtual bool interacts(const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override;
	virtual void invokeGuiOptions() override;


};
class Triangle : public Interactable {
public:
	Triangle(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 c,
		const Material* m = PhysicalBase::DEFAULT.get()
	) :
		position(center({ p1, p2, p3 })), albedo(c), mat(m), p1(p1), p2(p2), p3(p3),
		e1(p2 - p1), e2(p3 - p1), norm(glm::normalize(glm::cross(this->e1, this->e2)))
	{}

	glm::vec3 position, albedo;
	glm::vec3 p1, p2, p3, e1, e2, norm;
	const Material* mat;

	void move(glm::vec3);
	
	virtual bool interacts(const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override;
	virtual void invokeGuiOptions() override;


};
class Quad : public Interactable {
public:
	Quad(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4,
		glm::vec3 c = glm::vec3{ 1.f },
		const Material* m = PhysicalBase::DEFAULT.get()
	) :
		h1(p1, p2, p4, c, m), h2(p3, p2, p4, c, m)
	{}

	Triangle h1, h2;

	void move(glm::vec3);
	
	virtual bool interacts(const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override
		{ return this->h1.interacts(source, hit, t_min, t_max) || this->h2.interacts(source, hit, t_min, t_max); }
	virtual void invokeGuiOptions() override;


};


class Scene : public Interactable {
public:
	inline Scene(std::initializer_list<std::shared_ptr<Interactable>> objs) : objects(objs) {}

	glm::vec3 sky_color{0.2f};

	virtual bool interacts(const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override;
	virtual void invokeGuiOptions() override;
	inline glm::vec3 skyColor(const Ray& ray) const { return this->sky_color; }

private:
	std::vector<std::shared_ptr<Interactable>> objects;

};