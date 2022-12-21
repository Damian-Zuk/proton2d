#pragma once
#include "pch.h"
#include "proton/Graphics/Camera.h"
#include "proton/Core/Application.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace proton {

	Camera::Camera()
		: m_AspectRatio(Application::Get().GetWindow().GetAspectRatio())
	{
		RecalculateProjection();
	}

	void Camera::SetZoomLevel(float zoomLevel)
	{
		if (m_ZoomLevel != zoomLevel)
		{
			m_ZoomLevel = zoomLevel;
			RecalculateProjection();
		}
	}

	void Camera::SetAspectRatio(float aspectRatio)
	{
		if (m_AspectRatio != aspectRatio)
		{
			m_AspectRatio = aspectRatio;
			RecalculateProjection();
		}
	}

	void Camera::RecalculateProjection()
	{
		m_Projection.Left = -m_OrthographicSize * m_AspectRatio * 0.5f * m_ZoomLevel;
		m_Projection.Right = m_OrthographicSize * m_AspectRatio * 0.5f * m_ZoomLevel;
		m_Projection.Bottom = -m_OrthographicSize * 0.5f * m_ZoomLevel;
		m_Projection.Top = m_OrthographicSize * 0.5f * m_ZoomLevel;

		m_ProjectionMatrix = glm::ortho(m_Projection.Left, m_Projection.Right,
			m_Projection.Bottom, m_Projection.Top, m_OrthographicNear, m_OrthographicFar);;
	}

}
