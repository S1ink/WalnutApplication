#include "Scene.h"

#include <string>

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>


Material::Material(float r, float g, float t, float l, float ir) :
	roughness(r), glossiness(g), transparency(t), refraction_index(ir), luminance(l)
{
	Material::materials.emplace_back(this);
	this->index = Material::materials.size() - 1;
}
Material::Material(const Material& o) :
	roughness(o.roughness), glossiness(o.glossiness),
	transparency(o.transparency), refraction_index(o.refraction_index),
	luminance(o.luminance)
{
	Material::materials.emplace_back(this);
	this->index = Material::materials.size() - 1;
}
Material::~Material() {
	Material::materials.erase(Material::materials.begin() + this->index);
	for (size_t i = this->index; i < Material::materials.size(); i++) {
		Material::materials[i]->index = i;
	}
}
void Material::invokeGui() {
	ImGui::DragFloat("Roughness", &this->roughness, 0.005, 0.f, 1.f);
	ImGui::DragFloat("Glossiness", &this->glossiness, 0.005, 0.f, 1.f);
	ImGui::DragFloat("Transparency", &this->transparency, 0.005, 0.f, 1.f);
	ImGui::DragFloat("Refraction Index", &this->refraction_index, 0.005, 0.5, 10.f);
	ImGui::DragFloat("Luminance", &this->luminance, 0.005, 0.f, 10.f);
}
void Material::invokeManagerGui() {
	size_t i = 0;
	for (Material* mat : materials) {
		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(("Mat " + std::to_string(i)).c_str())) {
			mat->invokeGui();
		}
		ImGui::PopID();
		i++;
	}
	if (ImGui::Button("Add Material")) {
		Material* m = new Material;
	}
}

Ray Material::scatter(const Ray& src, const Interaction& hit) const {
	float seed = Walnut::Random::Float();
	if (seed < this->roughness) {
		return diffuse(hit.hit_normal);
	} else if(seed < this->transparency) {
		return refract(src, hit, this->refraction_index, this->glossiness);
	} else {
		return reflect(src, hit, this->glossiness);
	}
}
Ray Material::diffuse(const Ray& n, float f) {
	return Ray{
		n.origin, (n.direction + glm::normalize(randomWithinUnitSphere()))
	};
}
Ray Material::reflect(const Ray& src, const Interaction& hit, float g) {
	return Ray{
		hit.hit_normal.origin,
		glm::reflect(src.direction, hit.hit_normal.direction) + (g * randomWithinUnitSphere())
	};
}
inline static float reflectance(float cos, float ir) {
	return
		pow( ((1.f - ir) / (1.f + ir)), 2)
			+ (1.f - ir) * pow((1.f - cos), 5)
	;
}
Ray Material::refract(const Ray& src, const Interaction& hit, float ir, float g) {
	float cos_theta = fmin(glm::dot(-src.direction, hit.hit_normal.direction), 1.0);
	float sin_theta = sqrt(1.f - cos_theta * cos_theta);
	ir = hit.front_intersect ? (1.f / ir) : ir;
	if (ir * sin_theta > 1.f || reflectance(cos_theta, ir) > Walnut::Random::Float()) {
		return reflect(src, hit, g);
	}
	glm::vec3 r_out_perp = ir * (src.direction + cos_theta * hit.hit_normal.direction);
	glm::vec3 r_out_para = -(float)sqrt(fabs(1.0 - glm::dot(r_out_perp, r_out_perp))) * hit.hit_normal.direction;
	glm::vec3 refr = r_out_perp + r_out_para;
	return Ray{ hit.hit_normal.origin, refr };
}


bool Sphere::calcIntersection(const Ray& r, Interaction& h, float t_min, float t_max) {
	glm::vec3 o = r.origin - this->position;
	float
		a = glm::dot(r.direction, r.direction),
		b = 2.f * glm::dot(o, r.direction),
		c = glm::dot(o, o) - (this->rad * this->rad),
		d = (b * b) - 4.f * a * c;
	if (d < 0) { return false; }
	h.ptime = (sqrt(d) + b) / (-2.f * a);
	if (h.ptime < t_min || h.ptime > t_max) { return false; }
	h.hit_normal.origin = r.origin + r.direction * h.ptime;
	h.hit_normal.direction = glm::normalize(h.hit_normal.origin - this->position);
	float dot = glm::dot(h.hit_normal.direction, r.direction);
	h.front_intersect = (dot <= 0.f);
	h.hit_normal.direction *= -sgn(dot);
	// h.hit_normal.origin += h.hit_normal.direction * 1e-5f;	// no collide
	return true;
}
bool Triangle::calcIntersection(const Ray& r, Interaction& hr, float t_min, float t_max) {
	constexpr float EPSILON = 1e-5f;
	glm::vec3 h, s, q;
	float a, f, u, v;

	h = glm::cross(r.direction, this->e2);
	a = glm::dot(this->e1, h);
	if (a > -EPSILON && a < EPSILON) { return false; }
	f = 1.f / a;
	s = r.origin - p1;
	u = f * glm::dot(s, h);
	if (u < 0.f || u > 1.f) { return false; }
	q = glm::cross(s, this->e1);
	v = f * glm::dot(r.direction, q);
	if (v < 0.f || u + v > 1.f) { return false; }

	hr.ptime = f * glm::dot(this->e2, q);
	if (hr.ptime <= EPSILON || hr.ptime < t_min || hr.ptime > t_max) { return false; }
	hr.hit_normal.origin = r.origin + r.direction * hr.ptime;
	hr.hit_normal.direction = this->norm * -sgn(glm::dot(this->norm, r.direction));
	// h.hit_normal.origin += h.hit_normal.direction * 1e-5f;	// no collide
	hr.front_intersect = true;
	return true;
}

void Triangle::move(glm::vec3 p) {
	p = p - center({ this->p1, this->p2, this->p3 });
	this->p1 += p;
	this->p2 += p;
	this->p3 += p;
}
void Quad::move(glm::vec3 p) {
	p = p - center({ this->h1.p1, this->h1.p2, this->h2.p1, this->h1.p3 });
	this->h1.p1 += p;
	this->h2.p1 += p;
	this->h1.p2 += p;
	this->h2.p2 += p;
	this->h1.p3 += p;
	this->h2.p3 += p;
}

void Sphere::invokeGuiOptions() {
	ImGui::DragFloat3("Position", glm::value_ptr(this->position), 0.05);
	ImGui::DragFloat("Radius", &this->rad, 0.05);
	ImGui::ColorEdit3("Albedo", glm::value_ptr(this->albedo));
	int v = this->mat->idx();
	if (ImGui::DragInt("Material ID", &v, 1.f, 0, Material::numMats() - 1, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		this->mat = Material::getMat(v);
	}
}
void Triangle::invokeGuiOptions() {
	if (ImGui::DragFloat3("Position", glm::value_ptr(this->position), 0.05)) {
		this->move(this->position);
	}
	ImGui::ColorEdit3("Albedo", glm::value_ptr(this->albedo));
	int v = this->mat->idx();
	if (ImGui::DragInt("Material ID", &v, 1.f, 0, Material::numMats() - 1, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		this->mat = Material::getMat(v);
	}
}
void Quad::invokeGuiOptions() {
	if (ImGui::DragFloat3("Position", glm::value_ptr(this->h1.position), 0.05)) {
		this->move(this->h1.position);
	}
	ImGui::ColorEdit3("Albedo", glm::value_ptr(this->h1.albedo));
	int v = this->h1.mat->idx();
	if (ImGui::DragInt("Material ID", &v, 1.f, 0, Material::numMats() - 1, "%d", ImGuiSliderFlags_AlwaysClamp)) {
		this->h1.mat = Material::getMat(v);
	}
}