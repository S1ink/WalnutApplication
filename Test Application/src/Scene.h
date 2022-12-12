#pragma once

#include <vector>
#include <memory>
#include <initializer_list>

#include <glm/glm.hpp>
#include <stb_image.h>
#include <Walnut/Random.h>


inline static float sgn(float v) { return (int)(v > 0) - (int)(v < 0); }
inline static glm::vec3 center(std::initializer_list<glm::vec3> pts) {
	glm::vec3 ret;
	for (glm::vec3 pt : pts) {
		ret += pt;
	}
	return (ret /= pts.size());
}
inline static glm::vec3 center(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	return (a += b += c) /= 3.f;
}
inline static glm::vec3 center(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
	return (a += b += c += d) /= 4.f;
}
template<typename t>
inline static t clamp(t n, t h, t l)
	{ return n >= h ? h : n <= l ? l : n; }


struct Ray {
	glm::vec3 origin{0.f};
	glm::vec3 direction{0.f};
};
struct Hit {
	bool reverse_intersect{false};	// the normal is on the "inside" of the surface
	float ptime{0.f};		// time along source ray
	Ray normal{};			// normal with origin at the hit point
	glm::vec2 uv{ -1.f };
};


class Interactable {
public:
	virtual const Interactable* interacts(	// returns the actual entity that was intersected with (useful for 'container' Interactable's)
		const Ray& source, Hit& hit,
		float t_min = 1e-5f,
		float t_max = std::numeric_limits<float>::infinity()
	) const = 0;
	virtual bool redirect(
		const Ray& source,
		const Hit& interaction, Ray& redirected
	) const = 0;

	inline virtual float emmission(Hit& hit) const { return 0.f; }
	inline virtual glm::vec3 albedo(Hit& hit) const { return glm::vec3{ 0.5f }; }

	inline virtual bool invokeGuiOptions() { return false; }	// should return true if anything was updated
};
class Material {
public:
	virtual bool redirect(const Ray& source, const Hit& interaction, Ray& redirected) const = 0;
	inline virtual bool invokeGuiOptions() { return false; }
};
class Texture {
public:
	virtual glm::vec3 albedo(glm::vec2 uv) const = 0;
	inline virtual bool invokeGuiOptions() { return false; }
};



class PhysicalBase : public Material {
public:
	inline PhysicalBase(
		float roughness = 1.f,
		float glossness = 0.f,
		float transparency = 0.f,
		float refr_index = 1.f
	) :
		roughness(roughness), glossiness(glossness), transparency(transparency), refraction_index(refr_index)
	{}

	static const std::unique_ptr<Material> DEFAULT;

	float roughness, glossiness, transparency, refraction_index;

	virtual bool redirect(const Ray& source, const Hit& interaction, Ray& redirected) const override;
	virtual bool invokeGuiOptions() override;

	static bool diffuse(const Ray& normal, Ray& redirect);
	static bool reflect(const Ray& source, const Hit& hit, Ray& redirect, float gloss = 0.f);
	static bool refract(const Ray& source, const Hit& hit, float refr_index, Ray& redirect, float gloss = 0.f);

};
class StaticColor : public Texture {
public:
	inline StaticColor(glm::vec3 albedo = glm::vec3{0.5f}) : color(albedo) {}

	static const std::unique_ptr<Texture> DEFAULT;

	glm::vec3 color;

	inline virtual glm::vec3 albedo(glm::vec2) const override { return this->color; }
	virtual bool invokeGuiOptions() override;

};
class ImageTexture : public Texture {
public:
	inline ImageTexture() :
		ImageTexture(nullptr, 0U, 0U) {}
	inline ImageTexture(uint8_t* d, uint32_t w, uint32_t h, uint8_t pb = 3U) :
		data(d), width(w), height(h), pixel_bytes(pb) {}
	inline ImageTexture(const char* f) :
		data(stbi_load(f, (int*)&this->height, (int*)&this->width, (int*)&this->pixel_bytes, this->pixel_bytes)), pixel_bytes(3U)
		{ if (!data) { this->height = this->width = 0U; } }
	inline ~ImageTexture()
		{ if(this->data) { free(this->data); } }

	virtual glm::vec3 albedo(glm::vec2) const override;
	virtual bool invokeGuiOptions() override;

	uint8_t* data, pixel_bytes{ 3U };
	uint32_t width, height;

};



class Sphere : public Interactable {
public:
	inline Sphere(
		glm::vec3 p = glm::vec3{ 0.f },
		float r = 0.5f,
		Material* m = PhysicalBase::DEFAULT.get(),
		Texture* t = StaticColor::DEFAULT.get(),
		float l = 0.f
	) :
		position(p), radius(r), luminance(l), mat(m), tex(t) 
	{}

	glm::vec3 position;
	float radius{ 0.5f }, luminance{ 0.f };
	Material* mat;
	Texture* tex;

	virtual const Interactable* interacts(
		const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override;
	virtual glm::vec3 albedo(Hit& hit) const override;
	inline virtual bool redirect(const Ray& source, const Hit& hit, Ray& redirected) const override
		{ return this->mat ? this->mat->redirect(source, hit, redirected) : false; }
	inline virtual float emmission(Hit& hit) const override
		{ return this->luminance; }

	virtual bool invokeGuiOptions() override;


};
class Triangle : public Interactable {
public:
	inline Triangle(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
		Material* m = PhysicalBase::DEFAULT.get(),
		Texture* t = StaticColor::DEFAULT.get(),
		float l = 0.f
	) :
		position(center(p1, p2, p3)), p1(p1), p2(p2), p3(p3),
		e1(p2 - p1), e2(p3 - p1), norm(glm::normalize(glm::cross(this->e1, this->e2))),
		luminance(l), mat(m), tex(t)
	{}

	glm::vec3 position;
	glm::vec3 p1, p2, p3, e1, e2, norm;
	float luminance;
	Material* mat;
	Texture* tex;

	void move(glm::vec3);
	
	virtual const Interactable* interacts(const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override;
	virtual glm::vec3 albedo(Hit& hit) const override;
	inline virtual bool redirect(const Ray& source, const Hit& hit, Ray& redirected) const override
		{ return this->mat ? this->mat->redirect(source, hit, redirected) : false; }
	inline virtual float emmission(Hit& hit) const override
		{ return this->luminance; }
	
	virtual bool invokeGuiOptions() override;


};
class Quad : public Interactable {
public:
	inline Quad(
		glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4,
		Material* m = PhysicalBase::DEFAULT.get(),
		Texture* t = StaticColor::DEFAULT.get(),
		float l = 0.f
	) :
		h1(p1, p2, p4, m, t, l), h2(p3, p2, p4, m, t, l)
	{
		h1.position = center(p1, p2, p3, p4);
	}

	Triangle h1, h2;

	void move(glm::vec3);
	
	virtual const Interactable* interacts(
		const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override;
	virtual glm::vec3 albedo(Hit& hit) const override;
	inline virtual bool redirect(const Ray& source, const Hit& hit, Ray& redirected) const override
		{ return this->h1.mat ? this->h1.mat->redirect(source, hit, redirected) : false; }
	inline virtual float emmission(Hit& hit) const override
		{ return this->h1.luminance; }
	virtual bool invokeGuiOptions() override;


};

/* TODO:
* Add Cube / Rect Prism (Rect3D)
* Add Size/Rotation wrappers for triangle/quad/^^^
* Improve tri/quad/^^^ GUI options for pos/rotation/size
*/


class Scene : public Interactable {
public:
	inline Scene(std::initializer_list<std::shared_ptr<Interactable>> objs) : objects(objs) {}

	glm::vec3 sky_color{0.2f};

	virtual const Interactable* interacts(
		const Ray& source, Hit& hit, float t_min = 1e-5f, float t_max = std::numeric_limits<float>::infinity()) const override;
	inline virtual bool redirect(const Ray& source, const Hit& interaction, Ray& redirected) const override
		{ return false; }
	inline virtual glm::vec3 albedo(Hit& hit) const
		{ return this->sky_color; }
	virtual bool invokeGuiOptions() override;

private:
	std::vector<std::shared_ptr<Interactable>> objects;

};

class MaterialManager {
public:
	MaterialManager() = default;

	/*template<typename Mat_t>
	void add(size_t);
	template<typename Mat_t>
	void addExisting(std::unique_ptr<Mat_t>&&);*/

	bool invokeGui();	// returns if any settings were updated --> this does not necessarily correlate to if anything within the scene changed

private:
	std::vector<std::unique_ptr<Material>> materials;

};
class TextureManager {
public:
	TextureManager() = default;

	bool invokeGui();	// returns if any settings were updated --> this does not necessarily correlate to if anything within the scene changed

private:
	std::vector<std::unique_ptr<Texture>> textures;

};