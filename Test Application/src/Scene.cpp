#include "Scene.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>


float Sphere::intersectionTime(const Ray& r) const {
	glm::vec3 o = r.origin - this->position;
	float
		a = glm::dot(r.direction, r.direction),
		b = 2.f * glm::dot(o, r.direction),
		c = glm::dot(o, o) - (this->rad * this->rad),
		d = (b * b) - 4.f * a * c;
	if (d < 0.f) { return 0; }
	return (sqrt(d) + b) / (-2.f * a);
}
float Triangle::intersectionTime(const Ray& r) const {
	constexpr float EPSILON = 1e-5f;
	glm::vec3 h, s, q;
	float a, f, u, v;

	h = glm::cross(r.direction, this->e2);
	a = glm::dot(this->e1, h);
	if (a > -EPSILON && a < EPSILON) { return 0; }

	f = 1.f / a;
	s = r.origin - p1;
	u = f * glm::dot(s, h);
	if (u < 0.f || u > 1.f) { return 0; }

	q = glm::cross(s, this->e1);
	v = f * glm::dot(r.direction, q);
	if (v < 0.f || u + v > 1.f) { return 0; }

	float t = f * glm::dot(this->e2, q);
	if (t > EPSILON) {
		return t;
	}
	return 0.f;
}
float Quad::intersectionTime(const Ray& r) const {
	float t = this->h1.intersectionTime(r);
	if (t != 0.f) { return t; }
	else return this->h2.intersectionTime(r);
}

void Triangle::moveTo(glm::vec3 p) {
	p = p - center({ this->p1, this->p2, this->p3 });
	this->p1 += p;
	this->p2 += p;
	this->p3 += p;
}
void Quad::moveTo(glm::vec3 p) {
	p = p - center({ this->h1.p1, this->h1.p2, this->h2.p1, this->h1.p3 });
	this->h1.p1 += p;
	this->h2.p1 += p;
	this->h1.p2 += p;
	this->h2.p2 += p;
	this->h1.p3 += p;
	this->h2.p3 += p;
}

void Sphere::invokeOptions() {
	ImGui::DragFloat3("Position", glm::value_ptr(this->position), 0.05);
	ImGui::DragFloat("Size", &this->rad, 0.05);
	ImGui::ColorEdit3("Albedo", glm::value_ptr(this->albedo));
}
void Triangle::invokeOptions() {
	if (ImGui::DragFloat3("Position", glm::value_ptr(this->position), 0.05)) {
		this->moveTo(this->position);
	}
	ImGui::ColorEdit3("Albedo", glm::value_ptr(this->albedo));
}
void Quad::invokeOptions() {
	if (ImGui::DragFloat3("Position", glm::value_ptr(this->position), 0.05)) {
		this->moveTo(this->position);
	}
	ImGui::ColorEdit3("Albedo", glm::value_ptr(this->albedo));
}