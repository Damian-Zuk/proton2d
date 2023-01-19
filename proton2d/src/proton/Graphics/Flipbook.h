#pragma once

#include "proton/Graphics/Sprite.h"

#include <unordered_map>

namespace proton {

	enum class FlipbookPlayMode
	{
		REPEAT = 0, PLAY_ONCE
	};

	class Flipbook
	{
	public:
		Flipbook() = default;
		Flipbook(Sprite* sprite);

		void SetSprite(Sprite* sprite);

		// index - spritesheet Y tile pos (from image bottom)
		void CreateAnimation(uint16_t index, uint16_t frameCount);
		// index - spritesheet Y tile pos (from image bottom)
		void SetAnimation(uint16_t index, bool mirror_x = false, bool mirror_y = false);
		void SetMirrorFlip(bool mirror_x = false, bool mirror_y = false);
		void SetPlayMode(FlipbookPlayMode mode);
		void Replay();

		float GetProgress();
		// Use for FlipbookPlayMode::PLAY_ONCE
		bool FinishedPlaying();
		
		void SetFPS(uint16_t fps);
		uint16_t GetFPS() const { return m_FPS; }

	private:
		void PlayFrame(float ts);

		Sprite* m_Sprite;
		
		std::unordered_map<uint16_t, uint16_t> m_AnimationsFrameCount;
		uint16_t m_CurrentAnimationIndex = -1;
		uint16_t m_CurrentAnimationFrameCount = 0;

		FlipbookPlayMode m_PlayMode = FlipbookPlayMode::REPEAT;

		uint16_t m_FPS = 60;
		uint16_t m_CurrentFrame = 0;

		float m_FrameTime = 1.0f / m_FPS;
		float m_ElapsedTime = 0.0f;

		friend class Scene;
	};
}