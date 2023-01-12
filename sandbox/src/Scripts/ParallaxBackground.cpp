#include "pch.h"
#include "ParallaxBackground.h"

#include <cmath>

constexpr float TILING_FACTOR = 3.0f;

void ParallaxBackground::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "ParallaxFactor", &m_ParallaxFactor);
}

void ParallaxBackground::OnCreate()
{
	auto& sprite = GetComponent<SpriteComponent>();
	sprite.TilingFactor = TILING_FACTOR;
	sprite.Sprite->GetTexture()
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
	transform.Scale.x = viewSize * TILING_FACTOR;
	transform.Scale.y = viewSize * TILING_FACTOR;
}
