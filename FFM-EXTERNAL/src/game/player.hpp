#pragma once
#include "../other/vector3.h"
#include <cstdint>

// ESP PlayerData — screen coords + gameplay metadata.
struct PlayerData {
    uint64_t addr;
    Vector3  Head;        // topo do box (screen)
    Vector3  Toe;         // base do box (screen)
    Vector3  headWorld;   // world pos of head (bone::Head)
    Vector3  spineWorld;  // world pos of spine (bone::Spine)
    Vector3  neckWorld;   // world pos of neck (interpolated)
    bool     isKnocked;
    bool     isVisible;
    bool     isBot;
    float    distance;
    int      curHP;
    int      maxHP;
    char     name[36];    // player display name (UTF-8, truncated)
    int      weaponType;  // -1 unknown, 0..N weapon-type enum

    PlayerData()
        : addr(0), Head(Vector3::Zero()), Toe(Vector3::Zero()),
          headWorld(Vector3::Zero()), spineWorld(Vector3::Zero()), neckWorld(Vector3::Zero()),
          isKnocked(false), isVisible(false), isBot(false), distance(0.f), curHP(0), maxHP(200), weaponType(-1)
    { name[0] = 0; }

    Vector3 getTargetWorld(int targetPos) const {
        switch (targetPos) {
            case 1:  return neckWorld;
            case 2:  return spineWorld;
            default: return headWorld;
        }
    }
};
