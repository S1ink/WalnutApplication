#include "Scene.h"

#include <string>
#include <iostream>

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>


const std::unique_ptr<Material>
	PhysicalBase::DEFAULT{ std::make_unique<PhysicalBase>() };
const std::unique_ptr<Texture>
	StaticColor::DEFAULT{ std::make_unique<StaticColor>() };

//Material::Material(float r, float g, float t, float l, float ir) :
//	roughness(r), glossiness(g), transparency(t), refraction_index(ir), luminance(l)
//{
//	Material::materials.emplace_back(this);
//	this->index = Material::materials.size() - 1;
//}
//Material::Material(const Material& o) :
//	roughness(o.roughness), glossiness(o.glossiness),
//	transparency(o.transparency), refraction_index(o.refraction_index),
//	luminance(o.luminance)
//{
//	Material::materials.emplace_back(this);
//	this->index = Material::materials.size() - 1;
//}
//Material::~Material() {
//	Material::materials.erase(Material::materials.begin() + this->index);
//	for (size_t i = this->index; i < Material::materials.size(); i++) {
//		Material::materials[i]->index = i;
//	}
//}

void PhysicalBase::invokeGuiOptions() {
	ImGui::DragFloat("Roughness", &this->roughness, 0.005, 0.f, 1.f);
	ImGui::DragFloat("Glossiness", &this->glossiness, 0.005, 0.f, 1.f);
	ImGui::DragFloat("Transparency", &this->transparency, 0.005, 0.f, 1.f);
	ImGui::DragFloat("Refraction Index", &this->refraction_index, 0.005, 0.5, 10.f);
	//ImGui::DragFloat("Luminance", &this->luminance, 0.005, 0.f, 10.f);
}
//void Material::invokeManagerGui() {
//	size_t i = 0;
//	for (Material* mat : materials) {
//		ImGui::PushID(i);
//		if (ImGui::CollapsingHeader(("Mat " + std::to_string(i)).c_str())) {
//			mat->invokeGui();
//		}
//		ImGui::PopID();
//		i++;
//	}
//	if (ImGui::Button("Add Material")) {
//		Material* m = new Material;
//	}
//}

bool PhysicalBase::redirect(const Ray& src, const Hit& hit, Ray& out) const {
	//if (this->luminance >= 1.f) { return false; }
	float seed = Walnut::Random::Float();
	if (seed < this->roughness) {
		return diffuse(hit.normal, out);
	} else if(seed < this->transparency) {
		return refract(src, hit, this->refraction_index, out, this->glossiness);
	} else {
		return reflect(src, hit, out, this->glossiness);
	}
}
bool PhysicalBase::diffuse(const Ray& n, Ray& out) {
	out.origin = n.origin;
	out.direction = n.direction + Walnut::Random::InUnitSphere();
	if (fabs(out.direction.x) < 1e-5f && fabs(out.direction.y) < 1e-5f && fabs(out.direction.z) < 1e-5f) { out.direction = n.direction; }
	return true;
}
bool PhysicalBase::reflect(const Ray& src, const Hit& hit, Ray& out, float g) {
	out.origin = hit.normal.origin;
	out.direction = glm::reflect(src.direction, hit.normal.direction) + (g * Walnut::Random::InUnitSphere());
	return glm::dot(out.direction, hit.normal.direction) > 0;
}
inline static float reflectance(float cos, float ir) {
	return
		pow( ((1.f - ir) / (1.f + ir)), 2)
			+ (1.f - ir) * pow((1.f - cos), 5)
	;
}
bool PhysicalBase::refract(const Ray& src, const Hit& hit, float ir, Ray& out, float g) {
	float cos_theta = fmin(glm::dot(-src.direction, hit.normal.direction), 1.0);
	float sin_theta = sqrt(1.f - cos_theta * cos_theta);
	ir = hit.reverse_intersect ? ir : (1.f / ir);
	if (ir * sin_theta > 1.f || reflectance(cos_theta, ir) > Walnut::Random::Float()) {
		return reflect(src, hit, out, g);
	}
	glm::vec3 r_out_perp = ir * (src.direction + cos_theta * hit.normal.direction);
	glm::vec3 r_out_para = -(float)sqrt(fabs(1.0 - glm::dot(r_out_perp, r_out_perp))) * hit.normal.direction;
	out.direction = r_out_perp + r_out_para;
	out.origin = hit.normal.origin;
	return true;
}

void StaticColor::invokeGuiOptions() {
	ImGui::ColorEdit3("Albedo", glm::value_ptr(this->color));
}


const Interactable* Sphere::interacts(const Ray& r, Hit& h, float t_min, float t_max) const {
	glm::vec3 o = r.origin - this->position;
	float
		a = glm::dot(r.direction, r.direction),
		b = 2.f * glm::dot(o, r.direction),
		c = glm::dot(o, o) - (this->radius * this->radius),
		d = (b * b) - 4.f * a * c;
	if (d < 0) { return nullptr; }
	h.ptime = (sqrt(d) + b) / (-2.f * a);
	if (h.ptime < t_min || h.ptime > t_max) { return nullptr; }
	h.normal.origin = r.origin + r.direction * h.ptime;
	h.normal.direction = glm::normalize(h.normal.origin - this->position);
	if (h.reverse_intersect = (glm::dot(h.normal.direction, r.direction) > 0.f)) {
		h.normal.direction *= -1;
	}
	//h.surface = this->mat;
	// h.hit_normal.origin += h.hit_normal.direction * 1e-5f;	// no collide
	return this;
}
glm::vec3 Sphere::albedo(Hit& hit) const {
	if (!this->tex) {
		return glm::vec3{ 0.f };
	}
	if (hit.uv == glm::vec2{ -1.f }) {
		hit.uv = glm::vec2{
			(atan2(-hit.normal.direction.z, hit.normal.direction.x) + glm::pi<float>()) / glm::two_pi<float>(),
			acos(-hit.normal.direction.y) / glm::pi<float>()
		};
	}
	return this->tex->albedo(hit.uv);
}

const Interactable* Triangle::interacts(const Ray& r, Hit& hr, float t_min, float t_max) const {
	constexpr float EPSILON = 1e-5f;
	glm::vec3 h, s, q;
	float a, f, u, v;

	h = glm::cross(r.direction, this->e2);
	a = glm::dot(this->e1, h);
	if (a > -EPSILON && a < EPSILON) { return nullptr; }
	f = 1.f / a;
	s = r.origin - p1;
	u = f * glm::dot(s, h);
	if (u < 0.f || u > 1.f) { return nullptr; }
	q = glm::cross(s, this->e1);
	v = f * glm::dot(r.direction, q);
	if (v < 0.f || u + v > 1.f) { return nullptr; }

	hr.ptime = f * glm::dot(this->e2, q);
	if (hr.ptime <= EPSILON || hr.ptime < t_min || hr.ptime > t_max) { return nullptr; }
	hr.normal.origin = r.origin + r.direction * hr.ptime;
	hr.normal.direction = this->norm * -sgn(glm::dot(this->norm, r.direction));
	// h.hit_normal.origin += h.hit_normal.direction * 1e-5f;	// no collide
	hr.reverse_intersect = false;
	//hr.surface = this->mat;
	return this;
}
glm::vec3 Triangle::albedo(Hit& hit) const {
	if (!this->tex) {
		return glm::vec3{ 0.f };
	}
	if (hit.uv == glm::vec2{ -1.f }) {
		hit.uv = glm::vec2{};	// figure this out
	}
	return this->tex->albedo(hit.uv);
}

const Interactable* Quad::interacts(const Ray& source, Hit& hit, float t_min, float t_max) const {
	const Interactable* v = this->h1.interacts(source, hit, t_min, t_max);
	return v ? v : this->h2.interacts(source, hit, t_min, t_max);
}
glm::vec3 Quad::albedo(Hit& hit) const {
	glm::vec3 a = this->h1.albedo(hit);
	if (a == glm::vec3{ 0.f }) {
		hit.uv = glm::vec2{ -1.f };
		return this->h2.albedo(hit);
	}
	return a;
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
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_PTR_PTR")) {
			if (payload->DataSize == sizeof(Material*)) {
				this->mat = *((Material**)(payload->Data));
			}
		}
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_PTR_PTR")) {
			if (payload->DataSize == sizeof(Texture*)) {
				this->tex = *((Texture**)(payload->Data));
			}
		}
		ImGui::EndDragDropTarget();
	}
	ImGui::DragFloat3("Position", glm::value_ptr(this->position), 0.05);
	ImGui::DragFloat("Radius", &this->radius, 0.05);
	ImGui::DragFloat("Luminance", &this->luminance, 0.05, 0, 100);
	if (this->mat && ImGui::TreeNode("Material Editor")) {
		this->mat->invokeGuiOptions();
		ImGui::TreePop();
		ImGui::Separator();
	}
	if (this->tex && ImGui::TreeNode("Texture Editor")) {
		this->tex->invokeGuiOptions();
		ImGui::TreePop();
		ImGui::Separator();
	}
	//ImGui::Button("Aquire Property");
}
void Triangle::invokeGuiOptions() {
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_PTR_PTR")) {
			if (payload->DataSize == sizeof(Material*)) {
				this->mat = *((Material**)(payload->Data));
			}
		}
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_PTR_PTR")) {
			if (payload->DataSize == sizeof(Texture*)) {
				this->tex = *((Texture**)(payload->Data));
			}
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::DragFloat3("Position", glm::value_ptr(this->position), 0.05)) {
		this->move(this->position);
	}
	ImGui::DragFloat("Luminance", &this->luminance, 0.05, 0, 100);
	if (this->mat && ImGui::TreeNode("Material Editor")) {
		this->mat->invokeGuiOptions();
		ImGui::TreePop();
		ImGui::Separator();
	}
	if (this->tex && ImGui::TreeNode("Texture Editor")) {
		this->tex->invokeGuiOptions();
		ImGui::TreePop();
		ImGui::Separator();
	}
}
void Quad::invokeGuiOptions() {
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MATERIAL_PTR_PTR")) {
			if (payload->DataSize == sizeof(Material*)) {
				this->h1.mat = this->h2.mat = *((Material**)(payload->Data));
			}
		}
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_PTR_PTR")) {
			if (payload->DataSize == sizeof(Texture*)) {
				this->h1.tex = this->h2.tex = *((Texture**)(payload->Data));
			}
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::DragFloat3("Position", glm::value_ptr(this->h1.position), 0.05)) {
		this->move(this->h1.position);
	}
	if (ImGui::DragFloat("Luminance", &this->h1.luminance, 0.05, 0, 100)) {
		this->h2.luminance = this->h1.luminance;
	}
	if (this->h1.mat && ImGui::TreeNode("Material Editor")) {
		this->h1.mat->invokeGuiOptions();
		ImGui::TreePop();
		ImGui::Separator();
	}
	if (this->h1.tex && ImGui::TreeNode("Texture Editor")) {
		this->h1.tex->invokeGuiOptions();
		ImGui::TreePop();
		ImGui::Separator();
	}
}


const Interactable* Scene::interacts(const Ray& r, Hit& h, float tmin, float tmax) const {
	Hit temp;
	const Interactable* ret = nullptr;
	h.ptime = tmax;
	for (const std::shared_ptr<Interactable>& obj : this->objects) {
		if (const Interactable* i = obj->interacts(r, temp, tmin, h.ptime)) {
			h.reverse_intersect = temp.reverse_intersect;
			h.ptime = temp.ptime;
			h.normal = temp.normal;
			ret = i;
		}
	}
	return ret;
}
void Scene::invokeGuiOptions() {
	size_t i = 0;
	for (std::shared_ptr<Interactable>& obj : this->objects) {
		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(("Obj " + std::to_string(i)).c_str())) {
			obj->invokeGuiOptions();
		}
		ImGui::PopID();
		i++;
	}
}

void MaterialManager::invokeGui() {
	for (size_t i = 0; i < this->materials.size(); i++) {
		std::unique_ptr<Material>& mat = this->materials[i];
		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(("Mat " + std::to_string(i)).c_str())) {
			if (ImGui::BeginDragDropSource()) {
				Material* m = mat.get();
				ImGui::SetDragDropPayload("MATERIAL_PTR_PTR", &m, sizeof(Material*));
				ImGui::Text("Drag to Object to Apply");
				ImGui::EndDragDropSource();
			}
			mat->invokeGuiOptions();
			if (ImGui::Button("Delete")) {
				//this->materials.erase(this->materials.begin() + i);
				//i--;
			}
		} else if (ImGui::BeginDragDropSource()) {
			Material* m = mat.get();
			ImGui::SetDragDropPayload("MATERIAL_PTR_PTR", &m, sizeof(Material*));
			ImGui::Text("Drag to Object to Apply");
			ImGui::EndDragDropSource();
		}
		ImGui::PopID();
		i++;
	}
	if (ImGui::Button("Add PhysicalBase")) {
		this->materials.emplace_back(std::make_unique<PhysicalBase>());
	}
}
void TextureManager::invokeGui() {
	for (size_t i = 0; i < this->textures.size(); i++) {
		std::unique_ptr<Texture>& tex = this->textures[i];
		ImGui::PushID(i);
		if (ImGui::CollapsingHeader(("Texture " + std::to_string(i)).c_str())) {
			if (ImGui::BeginDragDropSource()) {
				Texture* t = tex.get();
				ImGui::SetDragDropPayload("TEXTURE_PTR_PTR", &t, sizeof(Texture*));
				ImGui::Text("Drag to Object to Apply");
				ImGui::EndDragDropSource();
			}
			tex->invokeGuiOptions();
			if (ImGui::Button("Delete")) {
				//this->textures.erase(this->textures.begin() + i);
				//i--;
			}
		} else if (ImGui::BeginDragDropSource()) {
			Texture* t = tex.get();
			ImGui::SetDragDropPayload("TEXTURE_PTR_PTR", &t, sizeof(Texture*));
			ImGui::Text("Drag to Object to Apply");
			ImGui::EndDragDropSource();
		}
		ImGui::PopID();
	}
	if (ImGui::Button("Add Staticly Colored Texture")) {
		this->textures.emplace_back(std::make_unique<StaticColor>());
	}
}