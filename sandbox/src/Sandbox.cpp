#include <Proton2D.h>

using namespace proton;

class MainLayer : public proton::Layer {
public:

	MainLayer() : Layer("MainLayer"), m_CameraController(16.0f / 9.0f) {}	
	~MainLayer() {}

	virtual void OnAttach() override 
	{
		m_PlayerTexture = CreateShared<Texture>("assets/Player.png");
	}

	virtual void OnDetach() override {}

	virtual void OnImGuiRender() override
	{
		auto stats = Renderer::GetStats();
		ImGui::Begin("Debug info");
		ImGui::Text("Zoom Level: %f", m_CameraController.GetCamera().GetZoomLevel());
		ImGui::Text("Quads: %i", stats.QuadCount);
		ImGui::Text("Draw calls: %i", stats.DrawCalls);
		ImGui::End();
	}
	
	virtual void OnUpdate(float ts) override 
	{
		m_CameraController.Update(ts);

		static auto speed = glm::vec2(0.15f, 0.23f);
		static auto pos = glm::vec2(0.0f, 0.0f);
		static float colorMod = 0.1f;
		static float colorModSpeed = 0.17f;
		
		pos += speed * ts;
		colorMod += colorModSpeed * ts;
		if (pos.x > 0.7f || pos.x < -0.7f) { speed.x *= -1; pos.x += speed.x * ts;}
		if (pos.y > 0.7f || pos.y < -0.7f) { speed.y *= -1; pos.y += speed.y * ts;}
		if (colorMod >= 0.7f || colorMod < 0.1f) { colorModSpeed *= -1; colorMod = colorMod >= 0.7f ? 0.7f : 0.1f; }
		
		Renderer::ResetStats();
		Renderer::Clear({ 0.1f, 0.12f, 0.16f, 1.0f });
		Renderer::BeginScene(m_CameraController.GetCamera());
		
		for (float y = -0.8f; y < 0.8f; y += 0.09f)
		{
			for (float x = -0.8f; x < 0.8f; x += 0.09f)
			{
				float r = std::min(std::max((x / (2.0f) + 0.6f) - colorMod / 3.0f, 0.0f), 1.0f);
				float b = std::min(std::max((y / (2.0f) + 0.6f) + colorMod / 5.0f, 0.0f), 1.0f);
				Renderer::DrawQuad({ x, y, 0.1f }, { 0.07f, 0.07f }, { r, colorMod, b, 1.0f });
			}
		}

		Renderer::DrawQuad(pos, { 0.5f, 0.5f }, m_PlayerTexture);
		Renderer::EndScene();
	}
	
	void OnEvent(proton::Event& event) 
	{
		m_CameraController.OnEvent(event);
	}

private:
	CameraController m_CameraController;
	Shared<Texture> m_PlayerTexture;
};


class Sandbox : public proton::Application
{
public:

	bool OnCreate() override
	{
		LOG_OK("Hello from SandBox :)");
		PushLayer(new MainLayer());
		return true;
	}

};

proton::Application* proton::CreateApp() {
	return new Sandbox();
}