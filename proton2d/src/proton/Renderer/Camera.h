#pragma once

#include <glm/glm.hpp>

namespace proton {

	class Camera
	{
	public:
		Camera(float aspectRatio, const glm::vec2& position = glm::vec2(0.0f));
		virtual ~Camera() = default;

		void Move(const glm::vec2& offset);
		void SetPosition(const glm::vec2& position);
		void SetZoomLevel(float zoomLevel);
		void SetAspectRatio(float aspectRatio);

		float GetZoomLevel() const { return m_ZoomLevel; }
		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::mat4& GetViewProjection() const { return m_ViewProjection; }

	private:
		void CalculateViewProjection();

	private:
		glm::mat4 m_ViewProjection = glm::mat4(1.0f);
		glm::vec3 m_Position;
		float m_ZoomLevel = 1.0f;
		float m_AspectRatio;
	};

}