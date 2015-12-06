/* ----------------------------------------------------------------------------
** Team Bear King
** © 2015 DigiPen Institute of Technology, All Rights Reserved.
**
** CharacterControllerSystem.h
**
** Author:
** - Jordan Ellis - j.ellis@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/

#pragma once

#include <EntitySystem.h>
#include <Components/CharacterControllerComponent.h>

class CharacterControllerSystem : public ursine::ecs::EntitySystem
{
    ENTITY_SYSTEM;

public:
    CharacterControllerSystem(ursine::ecs::World *world);

protected:

    void OnInitialize(void) override;
    void OnRemove(void) override;

private:

    // used to maintain player count and spawnpoint list
    void onComponentAdded(EVENT_HANDLER(ursine::ecs:::World));

    // spawn points and player count
    void onComponentRemoved(EVENT_HANDLER(ursine::ecs::World));

    void onUpdate(EVENT_HANDLER(ursine::ecs::World));

    void Process(ursine::ecs::Entity *entity);

    std::list<ursine::ecs::Entity *> m_entityList;

} Meta(Enable);