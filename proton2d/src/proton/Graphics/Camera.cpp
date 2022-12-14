#pragma once
#include "pch.h"
#include "proton/Graphics/Camera.h"
#include "proton/Core/Application.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace proton {

	Camera::Camera(const glm::vec2& position, float aspectRatio)
		: m_Position({ position.x, position.y, 0.0f }),
		m_AspectRatio(aspectRatio > 0.0f ? aspectRatio : Application::Get().GetWindow().GetAspectRatio())
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
		glm::mat4 viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), m_Position));
		
		m_Orthographic.Left = -m_OrthographicSize * m_AspectRatio * 0.5f * m_ZoomLevel;
		m_Orthographic.Right = m_OrthographicSize * m_AspectRatio * 0.5f * m_ZoomLevel;
		m_Orthographic.Bottom = -m_OrthographicSize * 0.5f * m_ZoomLevel;
		m_Orthographic.Top = m_OrthographicSize * 0.5f * m_ZoomLevel;

		glm::mat4 projectionMatrix = glm::ortho(m_Orthographic.Left, m_Orthographic.Right,
			m_Orthographic.Bottom, m_Orthographic.Top, m_OrthographicNear, m_OrthographicFar);

		m_ViewProjectionMatrix = projectionMatrix * viewMatrix;
	}

}
