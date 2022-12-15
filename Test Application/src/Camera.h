#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <mutex>

class Camera
{
public:
	Camera(float verticalFOV, float nearClip, float farClip);

	bool OnUpdate(float ts);
	void ChangeView(float verticalFOV, float nearClip, float farClip);

	inline const glm::vec3& GetPosition() const { return m_Position; }
	inline const glm::vec3& GetDirection() const { return m_ForwardDirection; }

	inline float GetRotationSpeed() const { return 0.3; }

	void CalcView(glm::mat4* inverse, glm::mat4* view = nullptr) const;
	void CalcProjection(glm::vec2 wh, glm::mat4* inverse, glm::mat4* projection = nullptr) const;

private:
	float m_VerticalFOV = 45.0f;
	float m_NearClip = 0.1f;
	float m_FarClip = 100.0f;

	glm::vec3 m_Position{ 0.0f, 0.0f, 0.0f };
	glm::vec3 m_ForwardDirection{ 0.0f, 0.0f, 0.0f };

	glm::vec2 m_LastMousePosition{ 0.0f, 0.0f };

};

struct CameraView {
	bool UpdateView(const Camera& c);
	bool UpdateProjection(const Camera& c, uint32_t w = 0, uint32_t h = 0);
	bool GenerateDirections(glm::vec3* r, uint32_t d = 1, uint32_t w = 0, uint32_t h = 0);

	uint32_t
		m_ViewWidth{ 0 }, m_ViewHeight{ 0 };
	glm::mat4	// view transforms into direction of camera, projection projects onto view plane
		m_InverseView{ 1.f }, m_InverseProjection{ 1.f };

};