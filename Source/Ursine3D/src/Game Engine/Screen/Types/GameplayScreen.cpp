/* ---------------------------------------------------------------------------
** Team Bear King
** ?2015 DigiPen Institute of Technology, All Rights Reserved.
**
** GameplayScreen.cpp
**
** Author:
** - Jordan Ellis - J.Ellis@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** -------------------------------------------------------------------------*/

#include "UrsinePrecompiled.h"

#include "GameplayScreen.h"

#include "SystemManager.h"
#include "TimerManager.h"
#include "TweenManager.h"

namespace ursine
{
    GameplayScreen::GameplayScreen(ScreenManager *manager)
        : Screen( manager, false )
        , world( nullptr )
    {

    }

    void GameplayScreen::OnFocusChanged(bool state)
    {
        URSINE_TODO( "Move this to scene" );
        /*ecs::ScreenFocusArgs args( ecs::WORLD_SCREEN_FOCUS_CHANGED, state );

        auto *timerManager = GetCoreSystem( TimerManager );
        auto *tweenManager = GetCoreSystem( TweenManager );

        if (state)
        {
            timerManager->Resume( kTimerGroupGameplay );
            tweenManager->Resume( kTweenGroupGameplay );
        }
        else
        {
            timerManager->Pause( kTimerGroupGameplay );
            tweenManager->Pause( kTweenGroupGameplay );
        }

        if (world)
            world->Dispatch( ecs::WORLD_SCREEN_FOCUS_CHANGED, &args );*/
    }

    void GameplayScreen::Update(void)
    {
        if (world && HasInputFocus( ))
            world->Update( );
    }

    void GameplayScreen::Render(void)
    {
        if (world)
            world->Render( );
    }
}