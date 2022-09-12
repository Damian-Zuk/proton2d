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
		if (offset != glm::vec2(0.0f)) 
		{
			m_Position += glm::vec3(offset.x, offset.y, 0.0f);
			CalculateViewProjection();
		}
	}

	void Camera::SetPosition(const glm::vec2& position)
	{
		glm::vec3 pos = { position.x, position.y, 0.0f };
		if (m_Position != pos)
		{
			m_Position = pos;
			CalculateViewProjection();
		}
	}

	void Camera::SetZoomLevel(float zoomLevel)
	{
		if (m_ZoomLevel != zoomLevel)
		{
			m_ZoomLevel = zoomLevel;
			CalculateViewProjection();
		}
	}

	void Camera::SetAspectRatio(float aspectRatio)
	{
		if (m_AspectRatio != aspectRatio)
		{
			m_AspectRatio = aspectRatio;
			CalculateViewProjection();
		}
	}

	void Camera::CalculateViewProjection()
	{
		m_ViewProjection = glm::inverse(glm::translate(glm::mat4(1.0f), m_Position)
			* glm::scale(glm::mat4(1.0f), glm::vec3(m_ZoomLevel * m_AspectRatio, m_ZoomLevel, m_ZoomLevel)));
	}

}
