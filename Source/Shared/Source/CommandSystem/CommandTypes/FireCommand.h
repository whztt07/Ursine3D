/* ----------------------------------------------------------------------------
** Team Bear King
** ?2015 DigiPen Institute of Technology, All Rights Reserved.
**
** FireCommand.h
**
** Author:
** - Matt Yan - m.yan@digipen.edu
** --------------------------------------------------------------------------*/

#pragma once

#include "RecordableCommand.h"

class FireCommand
    : public RecordableCommand
{
    RECORDABLE_COMMAND;

public:
    FireCommand(void);

    void Execute(ursine::ecs::Entity *receiver) override;
    void StopExecute(ursine::ecs::Entity *receiver) override;

    void StartRecording(ursine::ecs::Entity *receiver) override;
    void Record(ursine::ecs::Entity *receiver, const ursine::uint64 time) override;
    void RecordedExecutionPrep(ursine::ecs::Entity *receiver, const ursine::uint64 time) override;

};