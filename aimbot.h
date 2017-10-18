#ifndef __AIMBOT_H
#define __AIMBOT_H

#include <chrono>

#include <windows.h>

#include "SDK.hpp"
#include "config.h"
#include "util.h"

class Aimbot
{
    public:
    float m_LastAimDistance = 0.0f;

    void Run(Config& cfg, SDK::APlayerController* playerController)
    {
        auto now = m_Timer.now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastTime);
        m_LastTime = now;

        if (playerController == nullptr || playerController->AcknowledgedPawn == nullptr)
        {
            return;
        }

        if (GetAsyncKeyState(AIMBOT_KEY) & 0x8000)
        {
            if (m_TargetPlayer == nullptr)
            {
                m_TargetPlayer = Util::GetClosestVisiblePlayer(cfg, playerController);
            }
        }
        else
        {
            m_TargetPlayer = nullptr;
        }

        if (m_TargetPlayer != nullptr)
        {
            auto pawn = static_cast<SDK::AFortPawn*>(m_TargetPlayer);
            if (pawn->bIsDBNO)
            {
                m_TargetPlayer = nullptr;
                return;
            }

            SDK::FVector playerLoc;
            Util::Engine::GetBoneLocation(static_cast<SDK::ACharacter*>(m_TargetPlayer)->Mesh, &playerLoc, eBone::BONE_CHEST);

            auto dist = Util::GetDistance(playerController->AcknowledgedPawn->RootComponent->Location, playerLoc);
            m_LastAimDistance = dist;

            if (dist < cfg.m_MinAimbotHeadshotDistance)
            {
                Util::Engine::GetBoneLocation(static_cast<SDK::ACharacter*>(m_TargetPlayer)->Mesh, &playerLoc, eBone::BONE_HEAD);
            }

            Util::LookAt(cfg, playerController, playerLoc, deltaTime.count() / 1000.0f);
        }
    }

    SDK::AActor* GetTargetPlayer() const
    {
        return m_TargetPlayer;
    }

    private:
    SDK::AActor* m_TargetPlayer = nullptr;
    std::chrono::high_resolution_clock m_Timer;
    std::chrono::high_resolution_clock::time_point m_LastTime;
};

#endif
