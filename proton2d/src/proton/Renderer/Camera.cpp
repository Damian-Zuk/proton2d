#pragma once
#include "pch.h"
#include "proton/Renderer/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace proton {

	Camera::Camera(float aspectRatio, const glm::vec2& position)
		: m_Position({ position.x, position.y, 0.0f }), m_AspectRatio(aspectRatio)
	{
		CalculateViewProjection();
	}

	void Camera::Move(const glm::vec2& offset)
	{
		m_Position += glm::vec3(offset.x, offset.y, 0.0f );
		CalculateViewProjection();
	}

	void Camera::SetPosition(const glm::vec2& position)
	{
		m_Position = { position.x, position.y, 0.0f };
		CalculateViewProjection();
	}

	void Camera::Zoom(float zoomOffset)
	{
		m_ZoomLevel += zoomOffset * m_ZoomLevel;
		m_ZoomLevel = std::min(std::max(m_ZoomLevel, 0.05f), 10.0f);
		CalculateViewProjection();
	}

	void Camera::SetZoomLevel(float zoomLevel)
	{
		m_ZoomLevel = std::min(std::max(zoomLevel, 0.05f), 10.0f);
		CalculateViewProjection();
	}

	float Camera::GetZoomLevel() const
	{
		return m_ZoomLevel;
	}
	const glm::vec3& Camera::GetPosition() const
	{
		return m_Position;
	}
	const glm::mat4& Camera::GetViewProjection() const
	{
		return m_ViewProjection;
	}

	void Camera::CalculateViewProjection()
	{
		m_ViewProjection = glm::inverse(glm::translate(glm::mat4(1.0f), m_Position) 
			* glm::scale(glm::mat4(1.0f), glm::vec3(m_ZoomLevel)));
	}

}
