﻿/* ----------------------------------------------------------------------------
** Team Bear King
** © 2015 DigiPen Institute of Technology, All Rights Reserved.
**
** SceneUtils.cpp
**
** Author:
** - Austin Brunkhorst - a.brunkhorst@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/

#include "Precompiled.h"

#include "SceneUtils.h"

#include "Editor.h"
#include "Project.h"

#include "FileUtils.h"

#include <SystemManager.h>
#include <WorldSerializer.h>

using namespace ursine;

namespace
{
    const ursine::GUID kNullGUID = GUIDNullGenerator( )( );

    Project *getProject(void);
    Scene &getScene(void);
    ecs::World *getActiveWorld(void);

    void saveWorldAs(ecs::World *world, const fs::path &defaultPath);
    void saveWorld(ecs::World *world, const fs::path &path);
}

JSFunction(SceneSaveWorld)
{
    auto *project = getProject( );
    auto &lastOpened = project->GetLastOpenedWorld( );

    auto item = project->GetResourcePipeline( ).GetItem( lastOpened );

    auto *world = getActiveWorld( );

    // empty world, we MUST save as
    if (!item)
        saveWorldAs( world, "" );
    else
        saveWorld( world, item->GetSourceFileName( ) );

    return CefV8Value::CreateBool( true );
}

JSFunction(SceneSaveWorldAs)
{
    auto *project = getProject( );
    auto &lastOpened = project->GetLastOpenedWorld( );

    auto item = project->GetResourcePipeline( ).GetItem( lastOpened );

    auto *world = getActiveWorld( );

    saveWorldAs( world, item ? item->GetSourceFileName( ).parent_path( ) : "" );

    return CefV8Value::CreateBool( true );
}

JSFunction(SceneSetActiveWorld)
{
    if (arguments.size( ) != 1)
        JSThrow( "Invalid arguments.", nullptr );

    auto guid = GUIDStringGenerator( )( arguments[ 0 ]->GetStringValue( ).ToString( ) );

    auto &scene = getScene( );

    auto reference = scene.GetResourceManager( ).CreateReference( guid );

    scene.SetActiveWorld( reference );

    return CefV8Value::CreateBool( true );
}

JSFunction(SceneGetRootEntities)
{
    auto *world = getActiveWorld( );
    
    if (!world)
        return CefV8Value::CreateArray( 0 );

    auto root = world->GetRootEntities( );

    auto ids = CefV8Value::CreateArray( static_cast<int>( root.size( ) ) );

    for (size_t i = 0; i < root.size( ); ++i)
    {
        ids->SetValue( 
            static_cast<int>( i ), 
            CefV8Value::CreateUInt( root[ i ]->GetUniqueID( ) )
        );
    }

    return ids;
}

JSFunction(SceneGetActiveEntities)
{
    auto *world = getActiveWorld( );
    
    if (!world)
        return CefV8Value::CreateArray( 0 );

    auto &active = world->GetActiveEntities( );

    auto ids = CefV8Value::CreateArray( static_cast<int>( active.size( ) ) );

    for (size_t i = 0; i < active.size( ); ++i)
    {
        ids->SetValue( 
            static_cast<int>( i ), 
            CefV8Value::CreateUInt( active[ i ]->GetUniqueID( ) )
        );
    }

    return ids;
}

JSFunction(ScenePlayStart)
{
    Application::PostMainThread( [] {
        auto &scene = getScene( );

        scene.SetPlayState( PS_PLAYING );
    } );

    return CefV8Value::CreateUndefined( );
}

JSFunction(SceneSetPlayState)
{
    if (arguments.size( ) != 1)
        JSThrow( "Invalid arguments.", nullptr );

    auto playing = arguments[ 0 ]->GetBoolValue( );

    Application::PostMainThread( [=] {
        auto &scene = getScene( );

        scene.SetPlayState( playing ? PS_PLAYING : PS_PAUSED );
    } );

    return CefV8Value::CreateUndefined( );
}

JSFunction(SceneStep)
{
    Application::PostMainThread( [=] {
        auto &scene = getScene( );

        scene.Step( );
    } );

    return CefV8Value::CreateUndefined( );
}

JSFunction(ScenePlayStop)
{
    Application::PostMainThread( [] {
        auto &scene = getScene( );

        scene.SetPlayState( PS_EDITOR );
    } );

    return CefV8Value::CreateUndefined( );
}

JSFunction(SceneGetEntitySystems)
{
    Json::array systems;

    for (auto &type : ecs::SystemManager::GetExposedTypes( ))
    {
        auto &meta = type.GetMeta( );

        // exclude auto added systems
        if (!meta.GetProperty<AutoAddEntitySystem>( ))
            systems.emplace_back( type.GetName( ) );
    }

    CefRefPtr<CefV8Value> items;

    JsonSerializer::Deserialize( systems, items );

    return items;
}

namespace
{
    Project *getProject(void)
    {
        auto *editor = GetCoreSystem( Editor );

        return editor->GetProject( );
    }

    Scene &getScene(void)
    {
        return getProject( )->GetScene( );
    }

    ecs::World *getActiveWorld(void)
    {
        return getScene( ).GetActiveWorld( );
    }

    void saveWorldAs(ecs::World *world, const fs::path &defaultPath)
    {
        auto mode = static_cast<cef_file_dialog_mode_t>( FILE_DIALOG_SAVE | FILE_DIALOG_OVERWRITEPROMPT_FLAG );

        RunFileDialog( mode, "Save World", defaultPath, { "World Files|.uworld" },
            [=](int selectedFilter, const fs::FileList &files)
            {
                if (!files.empty( ))
                    saveWorld( world, files[ 0 ] );
            } 
        );
    }

    void saveWorld(ecs::World *world, const fs::path &path)
    {
        auto data = ecs::WorldSerializer( ).Serialize( world );

        UAssert( fs::WriteAllText( path.string( ), data.dump( true ) ),
            "Unable to save world.\nfile: %s",
            path.string( ).c_str( )
        );
    }
}