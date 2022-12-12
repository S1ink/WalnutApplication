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

	if (moved)
	{
		RecalculateView();
		//RecalculateRayDirections();
		std::thread(&Camera::RecalculateRayDirections, this).detach();
	}

	return moved;
}

void Camera::OnResize(uint32_t width, uint32_t height, uint32_t depth)
{
	if (depth > 0) depth = 1;
	if (width == m_ViewportWidth && height == m_ViewportHeight)
	{
		if (depth != m_ArrayDepth)
		{
			//RecalculateRayDirections();
			std::thread(&Camera::RecalculateRayDirections, this).detach();
		}
		return;
	}

	m_ViewportWidth = width;
	m_ViewportHeight = height;

	RecalculateProjection();
	//RecalculateRayDirections();
	std::thread(&Camera::RecalculateRayDirections, this).detach();
}

void Camera::OnReview(float verticalFOV, float nearClip, float farClip)
{
	if (verticalFOV == m_VerticalFOV && nearClip == m_NearClip && farClip == m_NearClip)
		return;

	m_VerticalFOV = verticalFOV;
	m_NearClip = nearClip;
	m_FarClip = farClip;

	RecalculateProjection();
	//RecalculateRayDirections();
	std::thread(&Camera::RecalculateRayDirections, this).detach();
}

float Camera::GetRotationSpeed()
{
	return 0.3f;
}

void Camera::RecalculateProjection()
{
	m_Projection = glm::perspectiveFov(glm::radians(m_VerticalFOV), (float)m_ViewportWidth, (float)m_ViewportHeight, m_NearClip, m_FarClip);
	m_InverseProjection = glm::inverse(m_Projection);
}

void Camera::RecalculateView()
{
	m_View = glm::lookAt(m_Position, m_Position + m_ForwardDirection, glm::vec3(0, 1, 0));
	m_InverseView = glm::inverse(m_View);
}

void Camera::RecalculateRayDirections()
{
	m_RayAccess.lock();
	m_AA_RayAccess.lock();

	std::cout << "Calculating Ray Directions..." << std::endl;

	m_RayDirections.resize(m_ArrayDepth);
	float rx = 0.f, ry = 0.f;

	if (m_ArrayDepth == 0)	// theoretically pointless but still good to have
		m_RayAccess.unlock();
	for (size_t r = 0; r < m_ArrayDepth; r++)
	{
		m_RayDirections[r].resize(m_ViewportWidth * m_ViewportHeight);
		if (r != 0)
		{
			rx = Random::Float();
			ry = Random::Float();
		}

		for (uint32_t y = 0; y < m_ViewportHeight; y++)
		{
			for (uint32_t x = 0; x < m_ViewportWidth; x++)
			{
				glm::vec2 coord = { (x + rx) / (float)m_ViewportWidth, (y + ry) / (float)m_ViewportHeight };
				coord = coord * 2.0f - 1.0f; // -1 -> 1

				glm::vec4 target = m_InverseProjection * glm::vec4(coord.x, coord.y, 1, 1);
				m_RayDirections[r][x + y * m_ViewportWidth] =
					glm::vec3(m_InverseView * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0)); // World space
			}
		}

		std::cout << "Calculated Ray Directions for array " << r << std::endl;

		if (r == 0)
			m_RayAccess.unlock();
	}
	m_AA_RayAccess.unlock();
}

void Camera::CalculateRandomDirections(std::vector<glm::vec3>& rays) const
{
	rays.resize(m_ViewportWidth * m_ViewportHeight);

	float rx = Random::Float();
	float ry = Random::Float();

	for (uint32_t y = 0; y < m_ViewportHeight; y++)
	{
		for (uint32_t x = 0; x < m_ViewportHeight; x++)
		{
			glm::vec2 coord = { (x + rx) / (float)m_ViewportWidth, (y + ry) / (float)m_ViewportHeight };
			coord = coord * 2.0f - 1.0f; // -1 -> 1

			glm::vec4 target = m_InverseProjection * glm::vec4(coord.x, coord.y, 1, 1);
			rays[x + y * m_ViewportWidth] =
				glm::vec3(m_InverseView * glm::vec4(glm::normalize(glm::vec3(target) / target.w), 0));  // World space
		}
	}
}

void Camera::CalculateRandomDirections(std::vector<std::vector<glm::vec3>>& rays) const
{
	for (std::vector<glm::vec3>& rarr : rays) {
		this->CalculateRandomDirections(rarr);
	}
}

//void Camera::CalculateRandomDirectionsSimple(std::vector<glm::vec3>& rays) const
//{
//	glm::vec3 min_variance = m_RayDirections[1 + m_ViewportWidth] - m_RayDirections[0];	// the top-leftmost ray minus the one exactly 1 pixel diagonal
//	glm::vec3 rand = min_variance * Random::Float();	// some random ray that is inbetween the top 2 top-leftmost pixel directions
//}

const std::vector<glm::vec3>& Camera::ScopedRayAccess::GetRayDirections()
{
	if (!locked)
	{
		parent->m_RayAccess.lock();
		locked = true;
	}
	return parent->m_RayDirections[0];
}
const std::vector<std::vector<glm::vec3>>& Camera::ScopedRayAccess::GetAARayDirections()
{
	if (!aa_locked)
	{
		parent->m_AA_RayAccess.lock();
		aa_locked = true;
	}
	return parent->m_RayDirections;
}
void Camera::ScopedRayAccess::UnlockAccess()
{
	if (locked)
	{
		parent->m_RayAccess.unlock();
		locked = false;
	}
	if (aa_locked)
	{
		parent->m_AA_RayAccess.unlock();
		aa_locked = false;
	}
}
Camera::ScopedRayAccess::~ScopedRayAccess()
{
	UnlockAccess();
}