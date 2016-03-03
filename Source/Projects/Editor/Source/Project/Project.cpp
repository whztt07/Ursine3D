﻿/* ----------------------------------------------------------------------------
** Team Bear King
** © 2015 DigiPen Institute of Technology, All Rights Reserved.
**
** Project.cpp
**
** Author:
** - Austin Brunkhorst - a.brunkhorst@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/

#include "Precompiled.h"

#include "Project.h"
#include "ResourcePipelineConfig.h"

#include <WorldSerializer.h>
#include <WorldData.h>

#include <LightComponent.h>
#include <WorldConfigComponent.h>
#include <Color.h>

using namespace ursine;

namespace
{
    const auto kBuiltInResourcesDirectory = "Resources/EditorTools";

    const auto kResourcesTempDirectory = "Temp";
    const auto kResourcesBuildDirectory = "Resources";
}

Project::Project(void)
    : m_entityManager( nullptr )
    , m_pipelineManager( nullptr )
    , m_lastOpenedWorld( GUIDNullGenerator( )( ) )
{
    m_scene.Listener( this )
        .On( SCENE_WORLD_CHANGED, &Project::onSceneWorldChanged );

    m_resourcePipeline.Listener( this )
        .On( rp::RP_RESOURCE_MODIFIED, &Project::onResourceModified );

    m_builtInResourceManager.SetResourceDirectory( kBuiltInResourcesDirectory );
}

Project::~Project(void)
{
    m_scene.Listener( this )
        .Off( SCENE_WORLD_CHANGED, &Project::onSceneWorldChanged );

    m_resourcePipeline.Listener( this )
        .Off( rp::RP_RESOURCE_MODIFIED, &Project::onResourceModified );

    delete m_entityManager;
    delete m_pipelineManager;
}

const ProjectConfig &Project::GetConfig(void) const
{
    return m_config;
}

rp::ResourcePipelineManager &Project::GetResourcePipeline(void)
{
    return m_resourcePipeline;
}

resources::ResourceManager &Project::GetBuiltInResourceManager(void)
{
    return m_builtInResourceManager;
}

Scene &Project::GetScene(void)
{
    return m_scene;
}

ScenePlayState Project::GetPlayState(void) const
{
    return m_scene.GetPlayState( );
}

void Project::SetPlayState(ScenePlayState state)
{
    auto lastState = m_scene.GetPlayState( );

    m_scene.SetPlayState( state );

    if (lastState == PS_EDITOR && (state == PS_PLAYING || state == PS_PAUSED))
    {
        ecs::WorldSerializer serializer;

        auto *oldWorld = m_scene.GetActiveWorld( );

        m_worldCache = ecs::WorldSerializer::Serialize( oldWorld );

        auto *playWorld = ecs::WorldSerializer::Deserialize( m_worldCache );

        playWorld->GetSettings( )->GetComponent<ecs::WorldConfig>( )->SetInEditorMode( false );

        m_scene.SetActiveWorld( ecs::World::Handle( playWorld ) );

        m_scene.LoadConfiguredSystems( );
    }
    else if((lastState == PS_PLAYING || lastState == PS_PAUSED) && state == PS_EDITOR)
    {
        auto *cachedWorld = ecs::WorldSerializer::Deserialize( m_worldCache );

        m_scene.SetActiveWorld( ecs::World::Handle( cachedWorld ) );

        cachedWorld->GetSettings( )->GetComponent<ecs::WorldConfig>( )->SetInEditorMode( true );
    } 
    else
    {
        m_scene.GetActiveWorld( )->GetSettings( )->GetComponent<ecs::WorldConfig>( )->SetInEditorMode( 
            state == PS_PAUSED 
        );
    }
}

void Project::SetEmptyScene(void)
{
    auto world = std::make_shared<ecs::World>( );

    auto univLight = world->CreateEntity( "Global Light" );
    {
        auto *component = univLight->AddComponent<ecs::Light>( );

        univLight->GetTransform( )->SetLocalPosition( { 0.0f, 60.0f, 0.0f } );

        component->SetLightType( ecs::LightType::Directional );
        component->SetRadius( 40.0f );
        component->SetColor( Color( 0.5f, 0.5f, 0.5f, 1.0f ) );
    }

    m_scene.SetActiveWorld( world );

    world->GetSettings( )->GetComponent<ecs::WorldConfig>( )->SetInEditorMode( true );
}

const ursine::GUID &Project::GetLastOpenedWorld(void)
{
    return m_lastOpenedWorld;
}

bool Project::CreateEditorResource(const ursine::GUID &resourceGUID) const
{
    try
    {
        auto resource = m_resourcePipeline.GetItem( resourceGUID );

        if (!resource)
            return false;

        auto buildFile = resource->GetBuildFileName( );
        auto targetFile = kBuiltInResourcesDirectory / buildFile.filename( );

        boost::system::error_code e;

        copy_file( buildFile, targetFile, fs::copy_option::overwrite_if_exists, e );

        return true;
    }
    catch (...)
    {
        return false;
    }
}

void Project::initialize(const ProjectConfig &config)
{
    m_config = config;

    rp::ResourcePipelineConfig resourceConfig;

    resourceConfig.resourceDirectory = config.rootDirectory / config.resourceDirectory;

    resourceConfig.tempDirectory = config.rootDirectory / config.buildDirectory;
    {
        // add the temp directory
        resourceConfig.tempDirectory /= kResourcesTempDirectory;
    }

    resourceConfig.buildDirectory = config.rootDirectory / config.buildDirectory;
    {
        // add the resource directory
        resourceConfig.buildDirectory /= kResourcesBuildDirectory;
    }

    m_scene.GetResourceManager( ).SetResourceDirectory(
        resourceConfig.buildDirectory
    );

    m_resourcePipeline.SetConfig( resourceConfig );
}

void Project::initializeScene(const resources::ResourceReference &startingWorld)
{
    m_entityManager = new EditorEntityManager( this );

    auto world = startingWorld.Load<resources::WorldData>( m_scene.GetResourceManager( ) );

    if (world)
    {
        m_scene.SetActiveWorld( startingWorld );
    }
    else
    {
        SetEmptyScene( );
    }

    m_pipelineManager = new EditorResourcePipelineManager( this );

    m_resourcePipeline.WatchResourceDirectory( );
}

void Project::onSceneWorldChanged(EVENT_HANDLER(Scene))
{
    EVENT_ATTRS(Scene, SceneWorldChangedArgs);

    if (args->reference)
    {
        m_lastOpenedWorld = args->reference->GetGUID( );
    }
    else
    {
        m_lastOpenedWorld = GUIDNullGenerator( )( );
    }
}

void Project::onResourceModified(EVENT_HANDLER(rp::ResourcePipelineManager))
{
    EVENT_ATTRS(rp::ResourcePipelineManager, rp::ResourceChangeArgs);

    m_scene.GetResourceManager( ).ReloadIfCached( args->resource->GetGUID( ) );

    auto *world = m_scene.GetActiveWorld( );

    if (world)
{
        ecs::EditorWorldResourceModifiedArgs e( args->resource->GetGUID( ) );

        world->Dispatch( ecs::WORLD_EDITOR_RESOURCE_MODIFIED, &e );
    }
}