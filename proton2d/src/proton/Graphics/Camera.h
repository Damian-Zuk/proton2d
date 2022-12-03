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
		Camera(const glm::vec2& position = glm::vec2(0.0f), float aspectRatio = -1.0f);
		virtual ~Camera() = default;

		void Move(const glm::vec2& offset);
		void SetPosition(const glm::vec2& position);
		void SetZoomLevel(float zoomLevel);
		void SetAspectRatio(float aspectRatio);

		float GetZoomLevel() const { return m_ZoomLevel; }
		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::mat4& GetViewProjection() const { return m_ViewProjectionMatrix; }
		const OrthoProjection& GetOrthoProjection() const { return m_Orthographic; }

	private:
		void CalculateViewProjection();

	private:
		glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);
		glm::vec3 m_Position = glm::vec3(0.0f);
		float m_ZoomLevel = 1.0f;
		float m_AspectRatio = 16.0f / 9.0f;

		OrthoProjection m_Orthographic;
		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1.0f;
		float m_OrthographicFar = 1.0f;
	};

}