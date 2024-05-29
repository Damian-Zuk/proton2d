#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/Panels/SceneHierarchyPanel.h"
#include "Proton/Editor/Panels/InspectorPanel.h"
#include "Proton/Editor/Panels/EditorGizmo2D.h"
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/EditorCamera.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Input.h"
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Utils/Utils.h"

#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>

namespace proton {

	SceneViewportPanel::~SceneViewportPanel()
	{
	}

	void SceneViewportPanel::OnCreate()
	{
		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		m_Framebuffer = MakeShared<Framebuffer>(fbSpec);
		m_Camera = MakeUnique<EditorCamera>();
		m_EditorGizmo = MakeUnique<EditorGizmo2D>();
		m_EditorGizmo->Init(this);
	}

	void SceneViewportPanel::OnImGuiRender()
	{
		// Scene Viewport
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		
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
				EditorLayer::Get()->CloseClientGameInstance(m_GameInstance->m_InstanceID);
				Scene* scene = Application::Get().GetGameInstance()->GetActiveScene();
				EditorLayer::GetSceneHierarchyPanel()->m_ActiveScene = scene;
				EditorLayer::GetInspectorPanel()->m_ActiveScene = scene;
				EditorLayer::SelectEntity({});
			}
		}

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();

		//EditorLayer::Get()->m_BlockEvents = !m_ViewportHovered;

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		if (m_ActiveScene)
		{
			uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
			ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		}
		else
		{
			ImGui::Dummy(ImVec2{ m_ViewportSize.x, m_ViewportSize.y });
		}
 
		HandleImGuiDragAndDrop();

		bool isFocused = ImGui::IsWindowFocused();
		if (isFocused != m_IsViewportFocused)
		{
			if (isFocused && EditorLayer::Get()->m_FocusedGameInstance != m_GameInstance)
			{
				EditorLayer::Get()->m_FocusedGameInstance = m_GameInstance;

				Scene* scene = m_GameInstance->GetActiveScene();
				EditorLayer::GetSceneHierarchyPanel()->m_ActiveScene = scene;
				EditorLayer::GetInspectorPanel()->m_ActiveScene = scene;
				EditorLayer::SelectEntity({});
			}
			m_IsViewportFocused = isFocused;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void SceneViewportPanel::OnUpdate(float ts)
	{
		if (!m_ActiveScene)
			return;

		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		// On viewport resize
		FramebufferSpecification spec = m_Framebuffer->GetSpecification();
		if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_ViewportAspectRatio = m_ViewportSize.x / m_ViewportSize.y;
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_Camera->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
			Renderer::SetViewport(0, 0, (uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		m_Framebuffer->Bind();
		Renderer::SetClearColor(m_ActiveScene->m_ClearColor);
		Renderer::Clear();

		// Update game instance
		m_GameInstance->OnUpdate(ts * Application::Get().GetTimeScale());

		// Get mouse position
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		m_MousePos = { (int)mx, (int)my };

		DrawCollidersAndSelectionOutline(ts);
		m_EditorGizmo->Render();
		m_Framebuffer->Unbind();

		const glm::vec2& cursor = m_ActiveScene->GetCursorWorldPosition();

		// Update editor camera
		m_Camera->OnUpdate(ts);

		// Move selected entity
		if (m_MoveSelectedEntity && m_SelectedEntity.IsValid())
		{
			auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
			
			glm::vec2 offset = {
				cursor.x + m_SelectionMouseOffset.x - transform.WorldPosition.x,
				cursor.y + m_SelectionMouseOffset.y - transform.WorldPosition.y
			};

			transform.LocalPosition.x += offset.x;
			transform.LocalPosition.y += offset.y;

			if (Input::IsKeyPressed(Key::LeftShift))
			{
				auto& relation = m_SelectedEntity.GetComponent<RelationshipComponent>();
				Entity current(relation.First, m_ActiveScene);
				while (current)
				{
					auto& r = current.GetComponent<RelationshipComponent>();
					auto& t = current.GetTransform();
					t.LocalPosition.x -= offset.x;
					t.LocalPosition.y -= offset.y;
					current = Entity(r.Next, m_ActiveScene);
				}
			}

			ImGui::SetMouseCursor(7);
		}

		// Move editor camera
		if (m_MoveEditorCamera)
		{
			glm::vec2 offset = m_CameraDragOffset - cursor;
			//PT_CORE_TRACE("{}, {}, {}", offset, m_CameraDragOffset, cursor);
			m_Camera->m_Position.x += offset.x;
			m_Camera->m_Position.y += offset.y;
			ImGui::SetMouseCursor(2);
		}
	}

	void SceneViewportPanel::OnEvent(Event& event)
	{
		if (!m_ActiveScene || !m_ViewportHovered)
			return;

		m_Camera->OnEvent(event);

		EventDispatcher dispatcher(event);

		// Dispatch mouse events
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
		{
			if (!m_IsMainViewport)
				return false;

			SceneState state = m_ActiveScene->GetSceneState();
			const glm::vec2& cursor = m_ActiveScene->GetCursorWorldPosition();

			// Mouse Button 1 (Right): Move editor camera
			if (e.GetMouseButton() == Mouse::Button1 && !m_MoveEditorCamera
				&& (state == SceneState::Stop || m_Camera->m_UseInRuntime ))
			{
				m_CameraDragOffset = cursor;
				m_MoveEditorCamera = true;
			}

			if (m_ActiveScene->IsSimulated() || !m_IsViewportFocused)
				return false;

			// Mouse Button 0 (Left): Select Entity
			else if (e.GetMouseButton() == Mouse::Button0)
			{
				Entity target; float transformMaxZ = 0.0f;

				for (auto& entity : m_ActiveScene->GetEntitiesOnCursorLocation())
				{
					if (!entity.HasAnyComponent<SpriteComponent, ResizableSpriteComponent,
						CircleRendererComponent, BoxColliderComponent, CircleColliderComponent>())
						continue;

					auto& transform = entity.GetComponent<TransformComponent>();
					if (!target || transform.WorldPosition.z > transformMaxZ)
					{
						target = entity;
						transformMaxZ = transform.WorldPosition.z;
					}
				}

				if (m_SelectedEntity && m_ActiveScene->IsCursorHoveringEntity(m_SelectedEntity))
				{
					// Discard selection
					target = m_SelectedEntity;
				}

				if (target && target == m_SelectedEntity)
				{
					m_MoveSelectedEntity = true;
					auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
					m_SelectionMouseOffset = glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y } - cursor;
				}

				EditorLayer::Get()->SelectEntity(target);
			}

			return false;
		});

		// Dispatch keyboard events
		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
		{
			if (!m_IsViewportFocused)
				return false;

			KeyCode key = e.GetKeyCode();

			if (key == Key::F3)
				m_ShowAllColliders = !m_ShowAllColliders;

			if (!m_IsMainViewport)
				return false;

			if (key == Key::F1)
				m_ShowSelectionOutline = !m_ShowSelectionOutline;

			if (key == Key::F2)
				m_ShowSelectionCollider = !m_ShowSelectionCollider;

			if (key == Key::Escape && m_SelectedEntity)
				EditorLayer::Get()->SelectEntity({});

			if (key == Key::Delete && m_SelectedEntity)
			{
				m_SelectedEntity.Destroy();
				EditorLayer::Get()->SelectEntity({});
			}

			if (key == Key::D && Input::IsKeyPressed(Key::LeftControl) && m_SelectedEntity)
			{
				Entity newEntity = m_ActiveScene->DuplicateEntity(m_SelectedEntity, Entity());
				auto& transform = newEntity.GetTransform();
				transform.LocalPosition.x += 0.1f;
				transform.LocalPosition.y += 0.1f;
				EditorLayer::Get()->SelectEntity(newEntity);
			}

			return false;
		});

		// Mouse evenets (button released)
		dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e)
		{
			if (e.GetMouseButton() == Mouse::Button0)
				m_MoveSelectedEntity = false;
			if (e.GetMouseButton() == Mouse::Button1)
				m_MoveEditorCamera = false;

			ImGui::SetMouseCursor(0);

			return false;
		});
	}

	void SceneViewportPanel::OnSelectEntity(Entity entity)
	{
		m_EditorGizmo->m_SelectedEntity = entity;
	}

	void SceneViewportPanel::OnSetActiveScene(Scene* scene)
	{
		m_EditorGizmo->m_ActiveScene = scene;
	}

	static constexpr glm::vec4 COLOR_WHITE = glm::vec4{ 1.0f };
	static constexpr glm::vec4 COLOR_YELLOW = { 0.8f, 0.8f, 0.2f, 1.0f };
	static constexpr glm::vec4 COLOR_COLLIDER = { 0.9f, 0.6f, 0.3f, 0.2f };
	static constexpr glm::vec4 COLOR_OUTLINE = { 0.95f, 0.25f, 0.18f, 0.75f}; 
	static constexpr glm::vec4 COLOR_SENSOR_OUTLINE = { 0.2f, 0.85f, 0.15f, 1.0f };

	static void DrawBoxCollider(TransformComponent& transform, BoxColliderComponent& bc)
	{
		glm::vec3 position = {
			transform.WorldPosition.x + bc.Offset.x,
			transform.WorldPosition.y + bc.Offset.y, 0.2f
		};
		glm::mat4 quadTransform = glm::translate(glm::mat4{ 1.0f }, { transform.WorldPosition.x, transform.WorldPosition.y, 0.2f })
			* glm::rotate(glm::mat4{ 1.0f }, glm::radians(transform.Rotation), { 0.0f, 0.0f, 1.0f })
			* glm::translate(glm::mat4{ 1.0f }, { bc.Offset.x, bc.Offset.y, 0.0f })
			* glm::scale(glm::mat4{ 1.0f }, { bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y, 1.0f });

		Renderer::DrawQuad(quadTransform, COLOR_COLLIDER);
		
		quadTransform[3].z += 0.001f;
		glm::vec4 sensorColor = bc.ContactCallback.ContactCount > 0 ? COLOR_YELLOW : COLOR_SENSOR_OUTLINE;
		Renderer::DrawRect(quadTransform, bc.IsSensor ? sensorColor : COLOR_OUTLINE);
	}

	static void DrawCircleCollider(TransformComponent& transform, CircleColliderComponent& cc, Camera& camera)
	{
		glm::vec3 position = {
			transform.WorldPosition.x + cc.Offset.x,
			transform.WorldPosition.y + cc.Offset.y, 0.2f
		};
		glm::vec3 scale = { cc.Radius * transform.Scale.x, cc.Radius * transform.Scale.x, 1.0f };
		glm::mat4 circleTransform = Math::GetTransform(position, scale, transform.Rotation);
		Renderer::DrawCircle(circleTransform, COLOR_COLLIDER);

		circleTransform[3].z += 0.001f;
		Renderer::DrawCircle(circleTransform, COLOR_OUTLINE,
			0.05f / glm::sqrt(1.0f / camera.GetZoomLevel() * 0.3f));

		position.z += 0.002f;
		glm::vec3 point = position;
		point.x += cc.Radius * transform.Scale.x / 2.0f * std::cos(glm::radians(transform.Rotation));
		point.y += cc.Radius * transform.Scale.x / 2.0f * std::sin(glm::radians(transform.Rotation));
		Renderer::DrawLine(position, point, COLOR_OUTLINE);
	}

	static void DrawSelectionOutline(Entity entity, glm::vec4 color, float ts)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		glm::vec3 position = { transform.WorldPosition.x, transform.WorldPosition.y, 0.21f };
		glm::vec3 scale = { transform.Scale.x + 0.07f, transform.Scale.y + 0.07f, 1.0f };
		glm::mat4 transformMatrix = Math::GetTransform(position, scale, transform.Rotation);

		if (entity.HasComponent<SpriteComponent>())
		{
			auto& sprite = entity.GetComponent<SpriteComponent>();
			scale.x *= sprite.Sprite.GetAspectRatio();
		}
		static float dashOffset = 0.0f;
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
		Camera& camera = m_ActiveScene->GetPrimaryCamera();
		Renderer::BeginScene(camera, m_ActiveScene->GetPrimaryCameraPosition());
		Renderer::SetLineWidth(2.5f);

		if (m_ShowAllColliders)
		{
			auto boxesView = m_ActiveScene->m_Registry.view<TransformComponent, BoxColliderComponent>();
			for (auto entity : boxesView)
			{
				auto [transform, bc] = boxesView.get<TransformComponent, BoxColliderComponent>(entity);
				DrawBoxCollider(transform, bc);
			}

			auto circlesView = m_ActiveScene->m_Registry.view<TransformComponent, CircleColliderComponent>();
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

		Renderer::EndScene();
	}

	void SceneViewportPanel::HandleImGuiDragAndDrop()
	{
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_PREFAB");
			if (payload && m_ActiveScene)
			{
				const wchar_t* path_wchar = (const wchar_t*)payload->Data;
				std::filesystem::path path(path_wchar);

				Entity entity = PrefabManager::Spawn(m_ActiveScene, path.string());
				auto& transform = entity.GetComponent<TransformComponent>();
				glm::vec2 cameraPos = m_ActiveScene->GetCursorWorldPosition();
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

}
#endif
