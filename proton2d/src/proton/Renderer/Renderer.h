#pragma once

#include "proton/Renderer/Texture.h"
#include "proton/Renderer/Camera.h"
#include "proton/Renderer/VertexArray.h"

namespace proton {

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera);
		static void EndScene();
		static void Flush();

		// Base Draw Quad functions (color and texture)
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
		static void DrawQuad(const glm::mat4& transform, const Shared<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		// Primitives
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		
		static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Shared<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Shared<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		
		static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Shared<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Shared<Texture>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void Clear(glm::vec4 color);
		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
		};
		static void ResetStats();
		static Statistics GetStats();

	private:
		static void InitQuadVertexArray();
		static void DrawIndexed(const Shared<VertexArray>& vertexArray, uint32_t indexCount = 0);

		static void StartBatch();
		static void NextBatch();
	};

}
