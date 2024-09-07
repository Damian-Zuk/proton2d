#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/Panels/SceneHierarchyPanel.h"
#include "Proton/Editor/Panels/InspectorPanel.h"
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Tools/EditorCamera.h"

#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Input.h"
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Graphics/Renderer/Framebuffer.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Utils/Utils.h"

#include "Proton/Network/NetworkManager.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace proton {

	SceneViewportPanel::~SceneViewportPanel()
	{
		if (!m_IsMainViewport)
			return;
	
		EditorLayer* editorLayer = EditorLayer::Get();
		if (editorLayer->GetFocusedViewportPanel() == this)
		{
			editorLayer->m_GameInstanceContext = &editorLayer->m_MainGameInstance;
		}
	}

	void SceneViewportPanel::OnCreate()
	{
		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;

		m_Framebuffer = MakeUnique<Framebuffer>(fbSpec);
		m_Camera = MakeUnique<EditorCamera>(this);
		m_GameInstance = m_EditorGameInstance->Instance.get();
	}

	void SceneViewportPanel::SetActiveScene(Scene* scene, bool sceneManagerCall) const
	{
		//PT_CORE_ASSERT(!scene || scene->m_GameInstance == m_GameInstance, "GameInstance mismatch");
		
		if (!sceneManagerCall)
		{
			m_GameInstance->m_SceneManager->SetActiveScene(scene);
		}
	}

	void SceneViewportPanel::SetSelectedEntity(Entity entity, bool sceneManagerCall)
	{
		//PT_CORE_ASSERT(!entity || entity.m_Scene->m_GameInstance == m_GameInstance, "GameInstance mismatch");
		//PT_CORE_ASSERT(!entity || entity.m_Scene == GetActiveScene(), "Scene mismatch");
		
		m_SelectedEntity = entity;
	}

	Scene* SceneViewportPanel::GetActiveScene() const
	{
		return m_GameInstance->GetActiveScene();
	}

	Entity SceneViewportPanel::GetSelectedEntity() const
	{
		return m_SelectedEntity;
	}

	void SceneViewportPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2{ 320, 280 });

		EditorLayer* editorLayer = EditorLayer::Get();

		if (m_IsMainViewport)
		{
			ImGui::Begin(m_ImGuiWindowName.c_str());
		}
		else
		{
			bool open = true;
			ImGui::Begin(m_ImGuiWindowName.c_str(), &open);
			if (!open)
			{
				editorLayer->CloseGameInstance(m_EditorGameInstance->ID);
			}
		}

		ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
		ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImVec2 viewportOffset = ImGui::GetWindowPos();

		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		if (m_GameInstance->GetActiveScene())
		{
			uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
			ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		}
		else
		{
			ImGui::Dummy({ m_ViewportSize.x, m_ViewportSize.y });
		}

		// Handle viewport focus change (game instance context)
		bool focused = ImGui::IsWindowFocused();
		if (focused != m_IsViewportFocused)
		{
			if (focused)
			{
				editorLayer->m_GameInstanceContext = m_GameInstance->m_EditorGameInstance;
			}
			m_IsViewportFocused = focused;
		}

		m_IsViewportHovered = ImGui::IsWindowHovered();

		HandleImGuiDragAndDrop();

		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	void SceneViewportPanel::OnUpdate(float ts)
	{
		Scene* scene = m_GameInstance->GetActiveScene();
		if (!scene)
			return;

		// Handle viewport resize
		const FramebufferSpecification& spec = m_Framebuffer->GetSpecification();

		if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f) // zero sized framebuffer is invalid
		{
			if (spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y)
			{
				m_ViewportAspectRatio = m_ViewportSize.x / m_ViewportSize.y;
				m_Camera->SetViewportSize(m_ViewportSize);
				m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
				scene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
				Renderer::SetViewport(0, 0, (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			}
			else if (scene->m_ViewportSize.x <= 0.0f || scene->m_ViewportSize.y <= 0.0f)
			{
				scene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			}
		}

		// Get mouse position
		ImVec2 mousePos = ImGui::GetMousePos();
		m_MousePos = {
			(int)mousePos.x - m_ViewportBounds[0].x,
			(int)mousePos.y - m_ViewportBounds[0].y
		};

		// Update editor camera
		m_Camera->OnUpdate(ts);

		// Render scene to framebuffer
		m_Framebuffer->Bind();
		m_GameInstance->OnUpdate(ts * Application::Get().GetTimeScale());
		DrawCollidersAndSelectionOutline(ts);
		m_Framebuffer->Unbind(); 

		// Move selected entity
		if (m_MoveSelectedEntity && m_SelectedEntity.IsValid())
		{
			auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
			
			const glm::vec2& cursor = scene->GetCursorWorldPosition();
			glm::vec2 offset = {
				cursor.x + m_SelectionMouseOffset.x - transform.WorldPosition.x,
				cursor.y + m_SelectionMouseOffset.y - transform.WorldPosition.y
			};

			if (offset.x != 0.0f || offset.y != 0.0f)
			{
				transform.LocalPosition.x += offset.x;
				transform.LocalPosition.y += offset.y;

				if (Input::IsKeyPressed(Key::LeftShift))
				{
					auto& relation = m_SelectedEntity.GetComponent<RelationshipComponent>();
					Entity current(relation.First, scene);
					while (current)
					{
						auto& r = current.GetComponent<RelationshipComponent>();
						auto& t = current.GetTransform();
						t.LocalPosition.x -= offset.x;
						t.LocalPosition.y -= offset.y;
						current = Entity(r.Next, scene);
					}
				}
			}

			ImGui::SetMouseCursor(7);
		}

		if (m_IsViewportFocused && m_IsViewportHovered)
		{
			if (Input::IsKeyPressed(Key::F) && Input::IsKeyPressed(Key::LeftControl))
			{
				if (m_SelectedEntity.IsValid() && m_SelectedEntity.HasComponent<RigidbodyComponent>())
				{
					if (b2Body* body = m_SelectedEntity.GetRuntimeBody())
					{
						const auto& cursor = GetActiveScene()->GetCursorWorldPosition();
						//body->SetTransform({ cursor.x, cursor.y }, 0.0f);
						m_SelectedEntity.SetRigidbodyTransform({ cursor.x, cursor.y }, 0.0f);
						body->SetLinearVelocity({ 0.0f, 0.0f });
					}
				}
			}
		}
	}

	void SceneViewportPanel::HandleImGuiDragAndDrop()
	{
		Scene* scene = GetActiveScene();
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_PREFAB");
			if (payload && scene)
			{
				const wchar_t* path_wchar = (const wchar_t*)payload->Data;
				std::filesystem::path path(path_wchar);

				Entity entity = PrefabManager::Spawn(scene, path.string());
				auto& transform = entity.GetComponent<TransformComponent>();
				glm::vec2 cameraPos = scene->GetCursorWorldPosition();
				entity.SetWorldPosition({ cameraPos.x, cameraPos.y, transform.WorldPosition.z });
			}
			payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_SCENE");
			if (payload)
			{
				const wchar_t* path_wchar = (const wchar_t*)payload->Data;
				std::filesystem::path path(path_wchar);
				std::string sceneFilepath = path.string();
				m_GameInstance->m_SceneManager->Load(sceneFilepath);
				m_GameInstance->m_SceneManager->SetActiveScene(sceneFilepath);
			}

			ImGui::EndDragDropTarget();
		}
	}

	void SceneViewportPanel::OnEvent(Event& event)
	{
		PROFILE_FUNCTION();
		Scene* scene = m_GameInstance->GetActiveScene();
		if (!scene)
			return;

		EventDispatcher dispatcher(event);

		// ------------------------ Dispatch mouse event ------------------------
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
		{
			if ((scene->IsSimulated() && !Input::IsKeyPressed(Key::LeftControl) && !Input::IsKeyPressed(Key::LeftShift)) || !m_IsViewportFocused)
				return false;

			// Mouse Button 0 (Left): Select / Move Entity
			if (e.GetMouseButton() == Mouse::Button0 && m_IsViewportHovered)
			{
				// Move selected entity
				if (m_SelectedEntity && scene->IsCursorHoveringEntity(m_SelectedEntity))
				{
					m_MoveSelectedEntity = true;
					const glm::vec2& cursor = scene->GetCursorWorldPosition();
					auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
					m_SelectionMouseOffset = glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y } - cursor;
					return false;
				}
				// Select entity
				Entity target; float positionMaxZ = 0.0f;

				for (auto& entity : scene->GetEntitiesOnCursorLocation())
				{
					if (!entity.HasAnyComponent<SpriteComponent, ResizableSpriteComponent, TextComponent,
						CircleRendererComponent, BoxColliderComponent, CircleColliderComponent>())
						continue;

					auto& transform = entity.GetComponent<TransformComponent>();
					if (!target || transform.WorldPosition.z > positionMaxZ)
					{
						target = entity;
						positionMaxZ = transform.WorldPosition.z;
					}
				}

				EditorLayer::Get()->SelectEntity(target);
			}

			return false;
		});

		// ------------------------ Dispatch keyboard event ------------------------
		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
		{
			if (!m_IsViewportFocused || !m_IsViewportHovered)
				return false;

			KeyCode key = e.GetKeyCode();

			if (key == Key::F1)
				m_ShowSelectionOutline = !m_ShowSelectionOutline;

			if (key == Key::F2)
				m_ShowSelectionCollider = !m_ShowSelectionCollider;

			if (key == Key::F3)
				m_ShowAllColliders = !m_ShowAllColliders;

			if (key == Key::F4)
				m_Camera->m_UseInRuntime = !m_Camera->m_UseInRuntime;


			if (m_SelectedEntity)
			{
				if (key == Key::Escape)
					EditorLayer::Get()->SelectEntity({});

				if (key == Key::Delete)
					m_SelectedEntity.Destroy();

				if (key == Key::D && Input::IsKeyPressed(Key::LeftControl) && !GetActiveScene()->IsSimulated())
				{
					Entity newEntity = scene->DuplicateEntity(m_SelectedEntity, Entity());
					auto& transform = newEntity.GetTransform();
					transform.LocalPosition.x += 0.1f;
					transform.LocalPosition.y += 0.1f;
					EditorLayer::Get()->SelectEntity(newEntity);
				}
			}

			return false;
		});

		// ------------------------ Mouse event (button released) ------------------------
		dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e)
		{
			if (e.GetMouseButton() == Mouse::Button0)
				m_MoveSelectedEntity = false;

			ImGui::SetMouseCursor(0);
			return false;
		});

		m_Camera->OnEvent(event);
	}

	static constexpr glm::vec4 COLOR_WHITE = glm::vec4{ 1.0f };
	static constexpr glm::vec4 COLOR_YELLOW = { 0.8f, 0.8f, 0.2f, 1.0f };
	static constexpr glm::vec4 COLOR_LIGHT_RED = { 0.9f, 0.38f, 0.3f, 1.0f };
	static constexpr glm::vec4 COLOR_MAGENTA = { 1.0f, 0.0f, 1.0f, 1.0f };
	static constexpr glm::vec4 COLOR_CYAN = { 0.0f, 0.987f, 1.0f, 1.0f };
	static constexpr glm::vec4 COLOR_ORANGE = { 0.9f, 0.6f, 0.3f, 0.2f };
	static constexpr glm::vec4 COLOR_RED = { 0.95f, 0.25f, 0.18f, 0.75f}; 
	static constexpr glm::vec4 COLOR_GREEN = { 0.2f, 0.85f, 0.15f, 1.0f };

	static void DrawBoxCollider(TransformComponent& transform, BoxColliderComponent& bc)
	{
		glm::mat4 quadTransform = glm::translate(glm::mat4{ 1.0f }, { transform.WorldPosition.x, transform.WorldPosition.y, 0.2f })
			* glm::rotate(glm::mat4{ 1.0f }, glm::radians(transform.Rotation), { 0.0f, 0.0f, 1.0f })
			* glm::translate(glm::mat4{ 1.0f }, { bc.Offset.x, bc.Offset.y, 0.0f })
			* glm::scale(glm::mat4{ 1.0f }, { bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y, 1.0f });
		
		quadTransform[3].z += 0.001f;
		glm::vec4 sensorColor = bc.ContactCallback.ContactCount > 0 ? COLOR_YELLOW : COLOR_GREEN;
		Renderer::DrawRect(quadTransform, bc.IsSensor ? sensorColor : COLOR_RED);
	}

	static void DrawCircleCollider(TransformComponent& transform, CircleColliderComponent& cc, Camera& camera)
	{
		glm::vec3 position = {
			transform.WorldPosition.x + cc.Offset.x,
			transform.WorldPosition.y + cc.Offset.y, 0.2f
		};
		glm::vec3 scale = { cc.Radius * transform.Scale.x, cc.Radius * transform.Scale.x, 1.0f };
		glm::mat4 circleTransform = Math::GetTransform(position, scale, transform.Rotation);
		Renderer::DrawCircle(circleTransform, COLOR_ORANGE);

		circleTransform[3].z += 0.001f;
		Renderer::DrawCircle(circleTransform, COLOR_RED,
			0.05f / glm::sqrt(1.0f / camera.GetZoomLevel() * 0.3f));

		position.z += 0.002f;
		glm::vec3 point = position;
		point.x += cc.Radius * transform.Scale.x / 2.0f * std::cos(glm::radians(transform.Rotation));
		point.y += cc.Radius * transform.Scale.x / 2.0f * std::sin(glm::radians(transform.Rotation));
		Renderer::DrawLine(position, point, COLOR_RED);
	}

	static void DrawSelectionOutline(Entity entity, glm::vec4 color, float ts)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		static float dashOffset = 0.0f;
		glm::mat4 transformMatrix;

		if (entity.HasComponent<TextComponent>())
		{
			auto& textComponent = entity.GetComponent<TextComponent>();

			float fontSize = transform.Scale.x;
			glm::vec2 textSize = textComponent.FontAsset->CalculateTextSize(&textComponent, transform.Scale);
			glm::vec3 position = {
				transform.WorldPosition.x,
				transform.WorldPosition.y,
				0.21f
			};

			glm::vec3 scale = { textSize.x + 0.1f, textSize.y + 0.1f, 1.0f };
			transformMatrix = Math::GetTransform(position, scale, transform.Rotation);
		}
		else
		{
			glm::vec3 position = { transform.WorldPosition.x, transform.WorldPosition.y, 0.21f };
			glm::vec3 scale = { transform.Scale.x + 0.07f, transform.Scale.y + 0.07f, 1.0f };
			transformMatrix = Math::GetTransform(position, scale, transform.Rotation);

			if (entity.HasComponent<SpriteComponent>())
			{
				auto& sprite = entity.GetComponent<SpriteComponent>();
				scale.x *= sprite.Sprite.GetAspectRatio();
			}
		}
		dashOffset += 0.4f * ts;
		Renderer::DrawDashedRect(transformMatrix, color, 2.0f, dashOffset);
	}

	static void DrawEntityColliders(Entity entity, Camera& camera)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& bc = entity.GetComponent<BoxColliderComponent>();
			DrawBoxCollider(transform, bc);
		}

		if (entity.HasComponent<CircleColliderComponent>())
		{
			auto& cc = entity.GetComponent<CircleColliderComponent>();
			DrawCircleCollider(transform, cc, camera);
		}

		auto& hierarchy = entity.GetComponent<RelationshipComponent>();
		Entity current(hierarchy.First, entity.GetScene());
		while (current)
		{
			auto& h = current.GetComponent<RelationshipComponent>();
			DrawEntityColliders(current, camera);
			current = Entity(h.Next, entity.GetScene());
		}
	}

	void SceneViewportPanel::DrawCollidersAndSelectionOutline(float ts)
	{
		Scene* scene = GetActiveScene();
		Camera& camera = scene->GetPrimaryCamera();
		Renderer::BeginScene(camera, scene->GetPrimaryCameraPosition());
		Renderer::SetLineWidth(2.5f);

		if (m_ShowAllColliders)
		{
			auto boxesView = scene->m_Registry.view<TransformComponent, BoxColliderComponent>();
			for (auto entity : boxesView)
			{
				auto [transform, bc] = boxesView.get<TransformComponent, BoxColliderComponent>(entity);
				DrawBoxCollider(transform, bc);
			}

			auto circlesView = scene->m_Registry.view<TransformComponent, CircleColliderComponent>();
			for (auto entity : circlesView)
			{
				auto [transform, cc] = circlesView.get<TransformComponent, CircleColliderComponent>(entity);
				DrawCircleCollider(transform, cc, camera);
			}
		}
		else if (m_ShowSelectionCollider && m_SelectedEntity.IsValid())
		{
			DrawEntityColliders(m_SelectedEntity, camera);
		}
		
		if (m_ShowSelectionOutline && m_SelectedEntity.IsValid())
		{
			DrawSelectionOutline(m_SelectedEntity, m_MoveSelectedEntity ? COLOR_YELLOW : COLOR_WHITE, ts);
		}

		NetworkManager* netManager = scene->GetGameInstance()->GetNetworkManager();
		if (m_SelectedEntity.IsValid() && m_SelectedEntity.IsNetworked())
		{
			auto& transform = m_SelectedEntity.GetTransform();
			auto& net = m_SelectedEntity.GetComponent<NetworkComponent>();
			auto& netTransform = net.NetTransform;
			
			if (m_TraceEntitySync && netManager->IsNetModeClient())
			{
				auto& last = netTransform.LastAuthoritativeTransform;
				auto& pred = netTransform.PredictedTransform;

				// Last server transform state
				Renderer::DrawRect(Math::GetTransform({ last.Position.x, last.Position.y, 0.201f },
					{ transform.Scale.x,transform.Scale.y }, last.Rotation), COLOR_CYAN);

				// Predicted transform state
				if (netTransform.Method == NetSyncMethod::Prediction || netTransform.Method == NetSyncMethod::Extrapolation)
				{
					bool reconcileStarted = netTransform.IsReconciling(NetTransform::Components::Position);
					Renderer::DrawRect(Math::GetTransform({ pred.Position.x, pred.Position.y, 0.202f },
						{ transform.Scale.x,transform.Scale.y }, last.Rotation),
						reconcileStarted ? COLOR_LIGHT_RED : COLOR_GREEN);
				}
			}
			
			if (m_ShowCullDistance)
			{
				glm::vec3 position = { transform.WorldPosition.x, transform.WorldPosition.y, 0.2f };
				glm::vec3 scale = { netTransform.CullDistance * 2.0f, netTransform.CullDistance * 2.0f, 1.0f };
				glm::mat4 circleTransform = Math::GetTransform(position, scale, transform.Rotation);
				Renderer::DrawCircle(circleTransform, COLOR_MAGENTA, 0.01f);

			}
		}

		Renderer::EndScene();
	}

}
#endif // PT_EDITOR
