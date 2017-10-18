#ifndef __UTIL_H
#define __UTIL_H

#include <windows.h>

#include <sstream>
#include <string>

#include "SDK.hpp"
#include "global.h"

#undef max
#define M_PI 3.14159265358979323846f

namespace Util
{
    std::wstring DistanceToString(float distance)
    {
        auto meters = distance * 0.01f;
        std::wstringstream ss;

        if (meters < 1000.0f)
        {
            ss << (int)meters << "m";
        }
        else
        {
            ss.precision(3);
            ss << meters / 1000.0f << "km";
        }

        return ss.str();
    }

    bool DataCompare(PBYTE pData, PBYTE bSig, char* szMask)
    {
        for (; *szMask; ++szMask, ++pData, ++bSig)
        {
            if (*szMask == 'x' && *pData != *bSig)
                return false;
        }
        return (*szMask) == 0;
    }

    PBYTE FindPattern(PBYTE dwAddress, DWORD dwSize, PBYTE pbSig, char* szMask, long offset)
    {
        size_t length = strlen(szMask);
        for (size_t i = NULL; i < dwSize - length; i++)
        {
            if (DataCompare(dwAddress + i, pbSig, szMask))
                return dwAddress + i + offset;
        }
        return 0;
    }

	namespace Vector
	{
        SDK::FVector Add(SDK::FVector point1, SDK::FVector point2)
        {
            SDK::FVector vector{ 0, 0, 0 };
            vector.X = point1.X + point2.X;
            vector.Y = point1.Y + point2.Y;
            vector.Z = point1.Z + point2.Z;
            return vector;
        }

		SDK::FVector Subtract(SDK::FVector point1, SDK::FVector point2)
		{
			SDK::FVector vector{ 0, 0, 0 };
			vector.X = point1.X - point2.X;
			vector.Y = point1.Y - point2.Y;
			vector.Z = point1.Z - point2.Z;
			return vector;
		}

		SDK::FVector Square(SDK::FVector vector)
		{
			return SDK::FVector{ vector.X * vector.X, vector.Y * vector.Y, vector.Z * vector.Z };
		}
	}

	namespace Vector2D
	{
		SDK::FVector2D Subtract(SDK::FVector2D point1, SDK::FVector2D point2)
		{
			SDK::FVector2D vector{ 0, 0 };
			vector.X = point1.X - point2.X;
			vector.Y = point1.Y - point2.Y;
			return vector;
		}
	}

	namespace Engine
	{
		DWORD_PTR w2sAddress;
		bool WorldToScreen(SDK::APlayerController* m_Player, SDK::FVector WorldPosition, SDK::FVector2D* ScreenPosition)
		{
			return reinterpret_cast<char(__fastcall*)(SDK::APlayerController*, SDK::FVector, SDK::FVector2D *, char)>(w2sAddress)(m_Player, WorldPosition, ScreenPosition, 0);
		}

		DWORD_PTR boneAddress;
		SDK::FMatrix* GetBoneMatrix(SDK::USkeletalMeshComponent* mesh, SDK::FMatrix* result, int boneid)
		{
			return reinterpret_cast<SDK::FMatrix*(__fastcall*)(SDK::USkeletalMeshComponent*, SDK::FMatrix*, int)>(boneAddress)(mesh, result, boneid);
		}

		void GetBoneLocation(SDK::USkeletalMeshComponent* mesh, SDK::FVector* result, int boneid)
		{
			SDK::FMatrix vMatrix;
			SDK::FMatrix *vTempMatrix = GetBoneMatrix(mesh, &vMatrix, boneid);
			*result = vMatrix.WPlane;
		}
	}

	bool IsLocalPlayer(SDK::AActor* player)
	{
		if (Global::m_LocalPlayer->PlayerController->AcknowledgedPawn == nullptr)
		{
			return true;
		}
		return (static_cast<SDK::APawn*>(player) == Global::m_LocalPlayer->PlayerController->AcknowledgedPawn);
	}

    bool IsTeammate(SDK::AActor* player)
    {
        if (!player)
        {
            return false;
        }

        if (!player->IsA(SDK::AFortPawn::StaticClass()))
        {
            return false;
        }

        auto myPawn = Global::m_LocalPlayer->PlayerController->AcknowledgedPawn;
        if (!myPawn)
        {
            return false;
        }

        auto otherPawn = static_cast<SDK::AFortPawn*>(player);
        if (!myPawn->PlayerState || !otherPawn->PlayerState)
        {
            return false;
        }

        if (!myPawn->PlayerState->IsA(SDK::AFortPlayerStateAthena::StaticClass()) || !otherPawn->PlayerState->IsA(SDK::AFortPlayerStateAthena::StaticClass()))
        {
            return false;
        }

        auto state = static_cast<SDK::AFortPlayerStateAthena*>(myPawn->PlayerState);
        auto state2 = static_cast<SDK::AFortPlayerStateAthena*>(otherPawn->PlayerState);

        return state->TeamIndex.GetValue() == state2->TeamIndex.GetValue();
    }

	float GetDistance(SDK::FVector x, SDK::FVector y)
	{
		auto z = Vector::Subtract(x, y);
		return sqrt(z.X * z.X + z.Y * z.Y + z.Z * z.Z);
	}

	float GetDistance2D(SDK::FVector2D point1, SDK::FVector2D point2)
	{
		SDK::FVector2D heading = Vector2D::Subtract(point2, point1);
		float distanceSquared;
		float distance;

		distanceSquared = heading.X * heading.X + heading.Y * heading.Y;
		distance = sqrt(distanceSquared);

		return distance;
	}

	bool IsInFOV(SDK::APlayerController* m_Player, SDK::FVector position, float fov)
	{
		int screenSizeX, screenSizeY;
		m_Player->GetViewportSize(&screenSizeX, &screenSizeY);
		SDK::FVector2D centerScreen{ (float)screenSizeX / 2, (float)screenSizeY / 2 };
		SDK::FVector2D screenPos;
		if (Engine::WorldToScreen(m_Player, position, &screenPos))
		{
			float dist = GetDistance2D(centerScreen, screenPos);
			if (dist < fov)
				return true;
		}
		return false;
	}

    SDK::AActor* GetClosestVisiblePlayer(Config& cfg, SDK::APlayerController* playerController)
    {
        if (!playerController->AcknowledgedPawn)
        {
            return nullptr;
        }

        auto localPos = playerController->AcknowledgedPawn->RootComponent->Location;

        std::vector<SDK::AActor*> candidates;

        auto actors = Global::m_PersistentLevel->AActors;
        for (int i = 0; i < actors.Num(); i++)
        {
            auto actor = Global::m_PersistentLevel->AActors[i];
            if (actor == nullptr || !actor->IsA(SDK::AFortPawn::StaticClass()) || actor->RootComponent == nullptr || IsLocalPlayer(actor))
            {
                continue;
            }
            
            if (IsTeammate(actor))
            {
                continue;
            }

            auto pawn = static_cast<SDK::AFortPawn*>(actor);
            if (pawn->GetName().find("PlayerPawn_Athena_C") == std::string::npos)
            {
                continue;
            }

            if (actor->IsA(SDK::AFortPlayerStateZone::StaticClass()))
            {
                auto stateZone = static_cast<SDK::AFortPlayerStateZone*>(actor);
                if (stateZone->CurrentHealth <= 0.0f || pawn->bIsDBNO)
                {
                    continue;
                }
            }

            candidates.push_back(actor);
        }

        float distance = std::numeric_limits<float>::max();
        SDK::AActor* closestPlayer = nullptr;

        int screenSizeX, screenSizeY;
        playerController->GetViewportSize(&screenSizeX, &screenSizeY);
        SDK::FVector2D centerScreen {(float)screenSizeX * 0.5f, (float)screenSizeY * 0.5f};

        distance = cfg.m_AimbotFieldOfViewPixels;
        for (auto candidate : candidates)
        {
            SDK::FVector playerLoc;
            Util::Engine::GetBoneLocation(static_cast<SDK::ACharacter*>(candidate)->Mesh, &playerLoc, 66);

            SDK::FVector2D screenPos;
            if (Engine::WorldToScreen(playerController, playerLoc, &screenPos))
            {
                float dist = GetDistance2D(centerScreen, screenPos);
                if (dist >= cfg.m_MaxAimbotLockDistance)
                {
                    continue;
                }

                if (dist < 24.0f)
                {
                    closestPlayer = candidate;
                    break;
                }
                else if (dist < distance)
                {
                    closestPlayer = candidate;
                    distance = dist;
                }
            }
        }

        return closestPlayer;
    }

	void LookAt(Config& cfg, SDK::APlayerController* m_Player, SDK::FVector position, float deltaTime)
	{
        int screenSizeX, screenSizeY;
        m_Player->GetViewportSize(&screenSizeX, &screenSizeY);
        SDK::FVector2D centerScreen{ (float)screenSizeX / 2, (float)screenSizeY / 2 };
        SDK::FVector2D screenPos;

        if (Engine::WorldToScreen(m_Player, position, &screenPos))
        {
            auto smoothing = deltaTime * cfg.m_AimbotSmoothing;
            SDK::FVector2D enemy2D = Vector2D::Subtract(screenPos, centerScreen);
            mouse_event(MOUSEEVENTF_MOVE, (float)(enemy2D.X / 20.0f) * smoothing, (float)(enemy2D.Y / 20.0f) * smoothing, 0, NULL);
        }
	}
}

#endif
