#pragma once

#include <glm/glm.hpp>

namespace proton {

	struct OrthoProjection
	{
		float Left, Right, Top, Bottom;
	};

	class Camera
	{
	public:
		Camera();
		virtual ~Camera() = default;

		void SetViewportSize(const glm::vec2& viewportSize);
		void SetZoomLevel(float zoomLevel);

		float GetZoomLevel() const { return m_ZoomLevel; }
		float GetAspectRatio() const { return m_AspectRatio; }
		float GetOrthographicSize() const { return m_OrthographicSize; }
		const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }
		const OrthoProjection& GetOrthoProjection() const { return m_Projection; }

	private:
		void RecalculateProjection();

	private:
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		
		float m_AspectRatio;
		float m_ZoomLevel = 1.0f;

		OrthoProjection m_Projection;
		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1.0f;
		float m_OrthographicFar = 1.0f;

		friend class InfoPanel;
		friend class ResizableSprite;
	};

}
