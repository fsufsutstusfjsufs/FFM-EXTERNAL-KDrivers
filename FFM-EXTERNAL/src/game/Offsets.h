#ifndef OFFSET_H
#define OFFSET_H

#include <cstdint>

// =========================================
// UPDATED OFFSETS LIST - SEPTEMBER 16, 2026
// Source: Desktop/offset.h (ARM64 Bit)
// =========================================

namespace Offsets {
    // ---- MainBase / GameFacade ----
    constexpr uintptr_t GameFacade_TypeInfo = 0xAD68E18;
    constexpr uintptr_t GameFacade          = 0xAD68E18;   // TypeInfo / BSS slot
    constexpr uintptr_t Staticfield         = 0xB8;
    constexpr uintptr_t StaticGameFacade    = 0xB8;
    constexpr uintptr_t BaseGame_HasInited  = 0x80;
    constexpr uintptr_t BaseGame_MatchEnd   = 0x8C;
    constexpr uintptr_t MatchIsRunning      = 0xCC;

    // ---- Match / Player container (MatchGame) ----
    constexpr uintptr_t CurrentMatch        = 0x90;
    constexpr uintptr_t localPlayer         = 0xD8;
    constexpr uintptr_t LocalPlayer         = 0xD8;
    constexpr uintptr_t CurrentObserve      = 0x100;
    constexpr uintptr_t ObserverPlayer      = 0x30;
    constexpr uintptr_t Dictionary          = 0xC0;
    constexpr uintptr_t DictionaryEntities  = 0xC0;
    constexpr uintptr_t LDictionaryEntities = 0xC8;

    // ---- Camera chain ----
    constexpr uintptr_t CameraBase          = 0xD8;        // From Desktop/offset.h
    constexpr uintptr_t Camera              = 0x20;
    constexpr uintptr_t IntPtrCam           = 0x10;
    constexpr uintptr_t Matrix              = 0x100;
    constexpr uintptr_t FollowCamera        = 0x698;
    constexpr uintptr_t MainCameraTransform = 0x3F0;

    // ---- Player flags & info ----
    constexpr uintptr_t Player_IsDead       = 0x7C;
    constexpr uintptr_t Player_isdead       = 0x7C;
    constexpr uintptr_t Player_Data         = 0x70;        // GetCurHP
    constexpr uintptr_t GetCurHP            = 0x70;
    constexpr uintptr_t GetCur              = 0x10;
    constexpr uintptr_t entryAddr           = 0x18;
    constexpr uintptr_t enemyList           = 0x10;
    constexpr uintptr_t HP                  = 0x20;
    constexpr uintptr_t Health              = 0x18;
    constexpr uintptr_t IsClientBot         = 0x4A8;
    constexpr uintptr_t IsBot               = 0x4A8;
    constexpr uintptr_t NamePtr             = 0x498;
    constexpr uintptr_t Player_Name         = 0x498;

    // ---- Avatar / team / visible ----
    constexpr uintptr_t AvatarManager       = 0x778;
    constexpr uintptr_t UmaAvatarSimple     = 0x138;
    constexpr uintptr_t Avatar              = 0x138;
    constexpr uintptr_t IsVisible           = 0x101;
    constexpr uintptr_t Avatar_IsVisible    = 0x101;
    constexpr uintptr_t UmaData             = 0x28;
    constexpr uintptr_t Avatar_Data         = 0x28;
    constexpr uintptr_t UmaData_IsTeam      = 0x81;
    constexpr uintptr_t Avatar_Data_IsTeam  = 0x81;

    // ---- Aim & SilentAim ----
    constexpr uintptr_t AimRotation         = 0x61C;
    constexpr uintptr_t Colider_Field       = 0x80;
    constexpr uintptr_t HedColider          = 0x740;
    constexpr uintptr_t aimInfo             = 0xE58;
    constexpr uintptr_t startPos            = 0x4C;
    constexpr uintptr_t raycast             = 0x40;
    constexpr uintptr_t isFiring            = 0x850;
    constexpr uintptr_t sAim1               = 0x850;       // isFiring
    constexpr uintptr_t sAim2               = 0xE58;       // aimInfo
    constexpr uintptr_t sAim3               = 0x4C;        // startPos
    constexpr uintptr_t sAim4               = 0x40;        // raycast
    constexpr uintptr_t CallSetAimRotationCount = 0x79C;

    // ---- Bones ----
    constexpr uintptr_t Head                = 0x6A8;
    constexpr uintptr_t Spine               = 0x6B8;
    constexpr uintptr_t Hip                 = 0x6B0;
    constexpr uintptr_t Root                = 0x6D0;
    constexpr uintptr_t lHand               = 0x728;
    constexpr uintptr_t rHand               = 0x720;
    constexpr uintptr_t lElbow              = 0x738;
    constexpr uintptr_t rElbow              = 0x730;
    constexpr uintptr_t lShoulder           = 0x710;
    constexpr uintptr_t rShoulder           = 0x718;
    constexpr uintptr_t lAnkle              = 0x6E0;
    constexpr uintptr_t rAnkle              = 0x6E8;
    constexpr uintptr_t lToe                = 0x6F0;
    constexpr uintptr_t rToe                = 0x6F8;

    // ---- Weapon ----
    constexpr uintptr_t InventoryManager    = 0x748;
    constexpr uintptr_t Weapon              = 0x608;
    constexpr uintptr_t Player_ActiveUISightingWeapon = 0x608;
    constexpr uintptr_t Player_WeaponOnHand = 0x17E8;
    constexpr uintptr_t Item                = 0xA0;
    constexpr uintptr_t WeaponComponent     = 0x80;
    constexpr uintptr_t ItemData            = 0x28;
    constexpr uintptr_t ItemData_WeaponId   = 0x40;
    constexpr uintptr_t NoRecoil            = 0x18;
    constexpr uintptr_t Weapon_Data         = 0x98;
    constexpr uintptr_t WeaponData_Type     = 0x64;
    constexpr uintptr_t WeaponData_AimType  = 0xC8;
    constexpr int       WeaponType_Sniper   = 1;

    // ---- Knock & Health ----
    constexpr uintptr_t PhyXdata            = 0x1D60;
    constexpr uintptr_t maybeDead           = 0x20;
    constexpr uintptr_t isKnocked           = 0x10;

    // ---- Misc ----
    constexpr uintptr_t GhostHack           = 0x814;
    constexpr uintptr_t PlayerAttributes    = 0x770;
    constexpr uintptr_t ShootNoReload       = 0x111;

    // ---- Transform ----
    constexpr uintptr_t transformValue      = 0x10;
    constexpr uintptr_t transformObjValue   = 0x10;
    constexpr uintptr_t indexValue          = 0x40;
    constexpr uintptr_t matrixValue         = 0x38;
    constexpr uintptr_t matrixListValue     = 0x18;
    constexpr uintptr_t matrixIndicesValue  = 0x20;
    constexpr uintptr_t resultValue         = 0x30;
    constexpr uintptr_t tMatrixValue        = 0x30;
}

namespace bone {
    constexpr uintptr_t HedColider          = 0x740;
    constexpr uintptr_t Head                = 0x6A8;
    constexpr uintptr_t Spine               = 0x6B8;
    constexpr uintptr_t Hip                 = 0x6B0;
    constexpr uintptr_t Root                = 0x6D0;
    constexpr uintptr_t lHand               = 0x728;
    constexpr uintptr_t rHand               = 0x720;
    constexpr uintptr_t lElbow              = 0x738;
    constexpr uintptr_t rElbow              = 0x730;
    constexpr uintptr_t lShoulder           = 0x710;
    constexpr uintptr_t rShoulder           = 0x718;
    constexpr uintptr_t lAnkle              = 0x6E0;
    constexpr uintptr_t rAnkle              = 0x6E8;
    constexpr uintptr_t lToe                = 0x6F0;
    constexpr uintptr_t rToe                = 0x6F8;
}

namespace offset {
    using namespace Offsets;
}

#endif // OFFSET_H
