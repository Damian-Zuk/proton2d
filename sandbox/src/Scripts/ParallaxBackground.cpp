#include "pcheader.h"
#include "ParallaxBackground.h"

using namespace proton;

void ParallaxBackground::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "ParallaxFactor", &m_ParallaxFactor);
	RegisterField(ScriptFieldType::Float, "PositionOffset", &m_PositionOffset);
}

void ParallaxBackground::OnCreate()
{
	auto& sprite = GetComponent<SpriteComponent>().Sprite;
	auto& camera = GetScene()->GetPrimaryCamera();
	float zoomLevel = camera->GetZoomLevel();
	float viewSize = camera->GetOrthographicSize() * zoomLevel;

	m_SpriteAspectRatio = sprite.GetAspectRatio();
	m_CopiesCount = (uint32_t)ceil(camera->GetAspectRatio() / m_SpriteAspectRatio) + 1;
	m_Entity.DestroyChildEntities();

	// Create copies of image
	for (uint32_t i = 0; i < m_CopiesCount; i++)
	{
		Entity e = GetScene()->CreateEntity(m_Entity.GetTag() + "-" + std::to_string(i));
		e.AddComponent<SpriteComponent>().Sprite.SetTexture(sprite.GetTexture());
		auto& transform = e.GetTransform(); 
		transform.Position.z = GetTransform().Position.z;
		m_Copies.push_back(e); AddChild(e);
	}
}

void ParallaxBackground::OnUpdate(float ts)
{
	const auto& camera = GetScene()->GetPrimaryCamera();
	float zoomLevel = camera->GetZoomLevel();
	float viewSize = camera->GetOrthographicSize() * zoomLevel;

	glm::vec3 cameraPos = GetScene()->GetPrimaryCameraPosition();
	glm::vec2 scale = { viewSize * m_SpriteAspectRatio, viewSize };

	float offset = fmod(cameraPos.x * zoomLevel * m_ParallaxFactor, scale.x);
	float position = cameraPos.x - scale.x * m_CopiesCount / 2.0f + m_PositionOffset * zoomLevel;
	
	auto& transform = GetTransform();
	transform.Scale = scale;
	transform.Position.x = position - offset;
	transform.Position.y = cameraPos.y;

	for (Entity copy : m_Copies)
	{
		auto& transform = copy.GetTransform();
		position += scale.x;
		transform.Scale = scale;
		transform.Position.x = position - offset;
		transform.Position.y = cameraPos.y;
	}
}
