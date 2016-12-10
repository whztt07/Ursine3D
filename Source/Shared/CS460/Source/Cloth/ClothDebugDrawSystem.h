/* ----------------------------------------------------------------------------
** Team Bear King
** ?2015 DigiPen Institute of Technology, All Rights Reserved.
**
** ClothDebugDrawSystem.h
**
** Author:
** - Jordan Ellis - j.ellis@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/

#pragma once

#include <FilterSystem.h>

class ClothDebugDrawSystem : public ursine::ecs::FilterSystem
{
    ENTITY_SYSTEM;

public:
    ClothDebugDrawSystem(ursine::ecs::World *world);

private:
    void Process(const ursine::ecs::EntityHandle &entity) override;

} Meta(Enable, AutoAddEntitySystem);
