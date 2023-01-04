#include "pch.h"
#include "proton/Graphics/Flipbook.h"

namespace proton {

    Flipbook::Flipbook(const Shared<Sprite>& sprite)
        : m_Sprite(sprite)
    {
    }

    void Flipbook::SetSprite(const Shared<Sprite>& sprite)
    {
        m_Sprite = sprite;
    }

    void Flipbook::CreateAnimation(uint16_t index, uint16_t frameCount)
    {
        m_AnimationsFrameCount[index] = frameCount;
    }

    void Flipbook::SetAnimation(uint16_t index, bool mirror_x, bool mirror_y)
    {
        assert(m_AnimationsFrameCount.find(index) != m_AnimationsFrameCount.end() && "Animation not found");
        
        if (!m_Sprite)
            return;

        if (index != m_CurrentAnimationIndex)
        {
            m_CurrentAnimationIndex = index;
            m_CurrentAnimationFrameCount = m_AnimationsFrameCount[index];
            m_Sprite->SetTile(0, index);
        }

        SetMirrorFlip(mirror_x, mirror_y);
    }

    void Flipbook::SetMirrorFlip(bool mirror_x, bool mirror_y)
    {
        m_Sprite->MirrorFlip(mirror_x, mirror_y);
    }

    void Flipbook::SetPlayMode(FlipbookPlayMode mode)
    {
        m_PlayMode = mode;
        Replay();
    }

    void Flipbook::Replay()
    {
        m_Sprite->SetTile(0, m_CurrentAnimationIndex);
        m_CurrentFrame = 0;
    }

    float Flipbook::GetProgress()
    {
        if (!m_CurrentAnimationFrameCount)
            return 0.0f;
        return (float)m_CurrentFrame / m_CurrentAnimationFrameCount;
    }

    void Flipbook::PlayFrame(float ts)
    {
        if (!m_Sprite || !m_CurrentAnimationFrameCount)
            return;

        m_ElapsedTime += ts;

        if (m_ElapsedTime >= m_FrameTime)
        {
            if (m_PlayMode == FlipbookPlayMode::REPEAT)
            {
                m_CurrentFrame %= m_CurrentAnimationFrameCount;
                m_Sprite->SetTile(m_CurrentFrame, m_Sprite->GetTilePos().y);
            }
            else // if (m_PlayMode == FlipbookPlayMode::PLAY_ONCE)
            {
                if (m_CurrentFrame < m_CurrentAnimationFrameCount)
                    m_Sprite->SetTile(m_CurrentFrame, m_Sprite->GetTilePos().y);
                else
                    m_Sprite->SetTile(0, m_Sprite->GetTilePos().y);
            }

            m_ElapsedTime = 0.0f;
            m_CurrentFrame++;
        }
    }

    bool Flipbook::FinishedPlaying()
    {
        return m_CurrentFrame == m_CurrentAnimationFrameCount;
    }

    void Flipbook::SetFPS(uint16_t fps)
    {
        m_FPS = fps;
        m_FrameTime = 1.0f / fps;
    }

}