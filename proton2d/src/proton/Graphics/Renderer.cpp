#include "pch.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Graphics/VertexArray.h"
#include "proton/Graphics/Shader.h"
#include "proton/Graphics/UniformBuffer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

namespace proton {

	constexpr static glm::vec4 QuadVertexPositions[] = {
		{ -0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f, -0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f, 0.0f, 1.0f },
		{ -0.5f,  0.5f, 0.0f, 1.0f }
	};

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TextureCoords;
		float TextureIndex;
		float TilingFactor;
	};

	static struct RendererData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		Shared<VertexArray> QuadVertexArray;
		Shared<VertexBuffer> QuadVertexBuffer;
		Shared<Shader> QuadShader;
		
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;
		uint32_t QuadIndexCount = 0;

		std::array<Shared<Texture>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;
		
		Shared<UniformBuffer> CameraUniformBuffer;
	} data;

	static void OpenGLMessageCallback(unsigned source, unsigned type, unsigned id, unsigned severity, int length, const char* message, const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:          LOG_ERROR("[OpenGL Critical Error]", message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:        LOG_ERROR("[OpenGL Error]", message); return;
		case GL_DEBUG_SEVERITY_LOW:           LOG_WARN("[OpenGL Warning]", message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION:  LOG_INFO("[OpenGL Info]", message); return;
		}
	}

	void Renderer::Init()
	{
	#ifdef PROTON_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);

		InitQuadVertexArray();

		data.TextureSlots[0]     = CreateShared<Texture>(1, 1, true);
		data.QuadShader          = CreateShared<Shader>("assets/shaders/Quad2D.glsl");
		data.CameraUniformBuffer = CreateShared<UniformBuffer>((uint32_t)sizeof(glm::mat4), 0);
	}

	void Renderer::InitQuadVertexArray()
	{
		data.QuadVertexArray  = CreateShared<VertexArray>();
		data.QuadVertexBuffer = CreateShared<VertexBuffer>((uint32_t)(data.MaxVertices * sizeof(QuadVertex)));

		data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "Position"      },
			{ ShaderDataType::Float4, "Color"         },
			{ ShaderDataType::Float2, "TextureCoords" },
			{ ShaderDataType::Float,  "TextureIndex"  },
			{ ShaderDataType::Float,  "TilingFactor"  }
		});

		data.QuadVertexArray->AddVertexBuffer(data.QuadVertexBuffer);
		data.QuadVertexBufferBase = new QuadVertex[data.MaxVertices];

		uint32_t* quadIndices = new uint32_t[data.MaxIndices];
		uint32_t offset = 0;

		for (uint32_t i = 0; i < data.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;
			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;
			offset += 4;
		}

		data.QuadVertexArray->SetIndexBuffer(CreateShared<IndexBuffer>(quadIndices, data.MaxIndices));
		delete[] quadIndices;
	}

	void Renderer::Shutdown()
	{
		delete[] data.QuadVertexBufferBase;
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		data.CameraUniformBuffer->SetData(&camera.GetViewProjection(), sizeof(glm::mat4));
		StartBatch();
	}

	void Renderer::EndScene()
	{
		Flush();
	}

	void Renderer::StartBatch()
	{
		data.QuadIndexCount = 0;
		data.QuadVertexBufferPtr = data.QuadVertexBufferBase;
		data.TextureSlotIndex = 1;
	}

	void Renderer::Flush()
	{
		if (data.QuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)data.QuadVertexBufferPtr - (uint8_t*)data.QuadVertexBufferBase);
			data.QuadVertexBuffer->SetData(data.QuadVertexBufferBase, dataSize);

			for (uint32_t i = 0; i < data.TextureSlotIndex; i++)
				data.TextureSlots[i]->Bind(i);

			data.QuadShader->Bind();
			data.QuadVertexArray->Bind();
			glDrawElements(GL_TRIANGLES, data.QuadIndexCount, GL_UNSIGNED_INT, nullptr);
		}
	}

	void Renderer::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		if (data.QuadIndexCount >= RendererData::MaxIndices)
			NextBatch();

		constexpr glm::vec2 textureCoords[] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};

		constexpr uint16_t QuadVertexCount = 4;
		for (uint16_t i = 0; i < QuadVertexCount; i++)
		{
			data.QuadVertexBufferPtr->Position = transform * QuadVertexPositions[i];
			data.QuadVertexBufferPtr->Color = color;
			data.QuadVertexBufferPtr->TextureIndex = 0.0f;
			data.QuadVertexBufferPtr->TilingFactor = 1.0f;
			data.QuadVertexBufferPtr->TextureCoords = textureCoords[i];
			data.QuadVertexBufferPtr++;
		}

		data.QuadIndexCount += 6;
	}

	void Renderer::DrawQuad(const glm::mat4& transform, const Shared<Sprite>& sprite, float tilingFactor, const glm::vec4& tintColor)
	{
		if (data.QuadIndexCount >= RendererData::MaxIndices)
			NextBatch();

		uint32_t textureIndex = 0;
		for (uint32_t i = 1; i < data.TextureSlotIndex; i++)
		{
			if (*data.TextureSlots[i] == *sprite->GetTexture())
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (data.TextureSlotIndex >= RendererData::MaxTextureSlots)
				NextBatch();

			textureIndex = data.TextureSlotIndex;
			data.TextureSlots[data.TextureSlotIndex] = sprite->GetTexture();
			data.TextureSlotIndex++;
		}

		auto& textureCoords = sprite->GetTextureCoords();

		constexpr uint16_t QuadVertexCount = 4;
		for (uint16_t i = 0; i < QuadVertexCount; i++)
		{
			data.QuadVertexBufferPtr->Position = transform * QuadVertexPositions[i];
			data.QuadVertexBufferPtr->Color = tintColor;
			data.QuadVertexBufferPtr->TextureIndex = static_cast<float>(textureIndex);
			data.QuadVertexBufferPtr->TextureCoords = textureCoords[i];
			data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			data.QuadVertexBufferPtr++;
		}

		data.QuadIndexCount += 6;
	}

	void Renderer::Clear(glm::vec4 color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

}
