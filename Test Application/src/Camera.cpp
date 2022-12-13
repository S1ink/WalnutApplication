#include "Camera.h"

#include <thread>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Walnut/Input/Input.h"
#include "Walnut/Random.h"


using namespace Walnut;

Camera::Camera(float verticalFOV, float nearClip, float farClip)
	: m_VerticalFOV(verticalFOV), m_NearClip(nearClip), m_FarClip(farClip)
{
	m_ForwardDirection = glm::vec3(0, 0, -1);
	m_Position = glm::vec3(0, 0, 3);
}

bool Camera::OnUpdate(float ts)
{
	glm::vec2 mousePos = Input::GetMousePosition();
	glm::vec2 delta = (mousePos - m_LastMousePosition) * 0.002f;
	m_LastMousePosition = mousePos;

	if (!Input::IsMouseButtonDown(MouseButton::Right))
	{
		Input::SetCursorMode(CursorMode::Normal);
		return false;
	}

	Input::SetCursorMode(CursorMode::Locked);

	bool moved = false;

	constexpr glm::vec3 upDirection(0.0f, 1.0f, 0.0f);
	glm::vec3 rightDirection = glm::cross(m_ForwardDirection, upDirection);

	float speed = 5.0f;
	if (Input::IsKeyDown(KeyCode::LeftShift) || Input::IsKeyDown(KeyCode::RightShift)) {
		speed *= 5;
	}

	// Movement
	if (Input::IsKeyDown(KeyCode::W))
	{
		m_Position += m_ForwardDirection * speed * ts;
		moved = true;
	}
	else if (Input::IsKeyDown(KeyCode::S))
	{
		m_Position -= m_ForwardDirection * speed * ts;
		moved = true;
	}
	if (Input::IsKeyDown(KeyCode::A))
	{
		m_Position -= rightDirection * speed * ts;
		moved = true;
	}
	else if (Input::IsKeyDown(KeyCode::D))
	{
		m_Position += rightDirection * speed * ts;
		moved = true;
	}
	if (Input::IsKeyDown(KeyCode::Q))
	{
		m_Position -= upDirection * speed * ts;
		moved = true;
	}
	else if (Input::IsKeyDown(KeyCode::E))
	{
		m_Position += upDirection * speed * ts;
		moved = true;
	}

	// Rotation
	if (delta.x != 0.0f || delta.y != 0.0f)
	{
		float pitchDelta = delta.y * GetRotationSpeed();
		float yawDelta = delta.x * GetRotationSpeed();

		glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, rightDirection),
			glm::angleAxis(-yawDelta, glm::vec3(0.f, 1.0f, 0.0f))));
		m_ForwardDirection = glm::rotate(q, m_ForwardDirection);

		moved = true;
	}

	return moved;
}

void Camera::ChangeView(float verticalFOV, float nearClip, float farClip)
{
	if (verticalFOV == m_VerticalFOV && nearClip == m_NearClip && farClip == m_NearClip)
		return;

	m_VerticalFOV = verticalFOV;
	m_NearClip = nearClip;
	m_FarClip = farClip;
}

void Camera::CalcProjection(glm::vec2 wh, glm::mat4* inverse, glm::mat4* projection) const
{
	if (projection)
	{
		*projection = glm::perspectiveFov(glm::radians(m_VerticalFOV), wh.x, wh.y, m_NearClip, m_FarClip);
		if (inverse)
			*inverse = glm::inverse(*projection);
	}
	else if (inverse)
		*inverse = glm::inverse(glm::perspectiveFov(glm::radians(m_VerticalFOV), wh.x, wh.y, m_NearClip, m_FarClip));

}

void Camera::CalcView(glm::mat4* inverse, glm::mat4* view) const
{
	if (view)
	{
		*view = glm::lookAt(m_Position, m_Position + m_ForwardDirection, glm::vec3(0, 1, 0));
		if (inverse)
			*inverse = glm::inverse(*view);
	}
	else if (inverse)
		*inverse = glm::inverse(glm::lookAt(m_Position, m_Position + m_ForwardDirection, glm::vec3(0, 1, 0)));

}


bool CameraView::UpdateView(const Camera& c)
{
	c.CalcView(&m_InverseView);
	return true;
}
bool CameraView::UpdateProjection(const Camera& c, uint32_t w, uint32_t h)
{
	if (w == 0 || h == 0)
	{
		c.CalcProjection({ w, h }, &m_InverseProjection);
		return true;
	}
	else if (w == m_ViewWidth && h == m_ViewHeight)
		return false;
	
	m_ViewWidth = w;
	m_ViewHeight = h;

	c.CalcProjection({ w, h }, &m_InverseProjection);
	return true;
}


//void Camera::RecalculateRayDirections()
//{
//	m_RayAccess.lock();
//	m_AA_RayAccess.lock();
//
//	std::cout << "Calculating Ray Directions..." << std::endl;
//
//	m_RayDirections.resize(m_ArrayDepth);
//	float rx = 0.f, ry = 0.f;
//
//	if (m_ArrayDepth == 0)	// theoretically pointless but still good to have
//		m_RayAccess.unlock();
//	for (size_t r = 0; r < m_ArrayDepth; r++)
//	{
//		m_RayDirections[r].resize(m_ViewportWidth * m_ViewportHeight);
//		if (r != 0)
//		{
//			rx = Random::Float();
//			ry = Random::Float();
//		}
//
//		for (uint32_t y = 0; y < m_ViewportHeight; y++)
//		{
//			for (uint32_t x = 0; x < m_ViewportWidth; x++)
//			{
//				glm::vec2 coord = { (x + rx) / (float)m_ViewportWidth, (y + ry) / (float)m_ViewportHeight };
//				coord = coord * 2.0f - 1.0f; // -1 -> 1
//
//				glm::vec4 target = m_InverseProjection * glm::vec4(coord.x, coord.y, 1, 1);
//				m_RayDirections[r][x + y * m_ViewportWidth] =
//					glm::vec3(m_InverseView * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0)); // World space
//			}
//		}
//
//		std::cout << "Calculated Ray Directions for array " << r << std::endl;
//
//		if (r == 0)
//			m_RayAccess.unlock();
//	}
//	m_AA_RayAccess.unlock();
//}