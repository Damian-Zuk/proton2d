#pragma once

#include "proton/Graphics/Sprite.h"
#include "proton/Graphics/Camera.h"

namespace proton {

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera);
		static void EndScene();
		static void Flush();

		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
		static void DrawQuad(const glm::mat4& transform, const Shared<Sprite>& sprite, const glm::vec4& tintColor = glm::vec4(1.0f));
		static void DrawQuad(const glm::mat4& transform, const Shared<Texture>& texture, const TextureCoords& textureCoords, const glm::vec4& tintColor);

		static void SetClearColor(glm::vec4 color);
		static void Clear();
		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		static void SetMaxQuadsCount(uint32_t count);
		static uint32_t GetDrawCallsCount();
		
	private:
		static void InitQuadVertexArray();

		static void StartBatch();
		static void NextBatch();
	};

}
