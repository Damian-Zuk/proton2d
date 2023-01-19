#include "pch.h"
#include "ParallaxBackground.h"

#include <cmath>

void ParallaxBackground::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "ParallaxFactor", &m_ParallaxFactor);
	RegisterField(ScriptFieldType::Float, "TilingFactor", &m_TilingFactor);
}

void ParallaxBackground::OnCreate()
{
	auto& sprite = GetComponent<SpriteComponent>();
	sprite.TilingFactor = m_TilingFactor;
	sprite.Sprite.GetTexture()
		->SetWrapMode(TextureWrapMode::Repeat, TextureWrapMode::ClampToBorder);
}

void ParallaxBackground::OnUpdate(float ts)
{
	auto cameraPos = GetScene()->GetPrimaryCameraPosition();
	auto& camera = GetScene()->GetPrimaryCamera();
	float orthoSize = camera->GetOrthographicSize();
	float zoomLevel = camera->GetZoomLevel();
	float aspectRatio = camera->GetAspectRatio();
	float viewSize = orthoSize * zoomLevel;
	float offset = fmod(cameraPos.x * m_ParallaxFactor * zoomLevel, viewSize * aspectRatio);
	
	auto& transform = GetTransform();
	transform.Position.x = cameraPos.x - offset;
	transform.Position.y = cameraPos.y + viewSize;
	transform.Scale.x = viewSize * m_TilingFactor;
	transform.Scale.y = viewSize * m_TilingFactor;
}
