#include "Precompiled.h"

#include "Editor.h"
#include "Project.h"

#include <Application.h>

#include <WindowManager.h>
#include <UIManager.h>

#include <Color.h> 

#include <RenderableComponent.h>
#include <LightComponent.h>
#include <Model3DComponent.h>
#include <CapsuleColliderComponent.h>
#include <RigidbodyComponent.h>
#include <BoxColliderComponent.h>
#include <Game Engine/Scene/Component/Native Components/CameraComponent.h>
#include "Tools/Scene/Entity Systems/EditorCameraSystem.h"
#include <Game Engine/Scene/Component/Native Components/Billboard2DComponent.h>

#include "CharacterControllerComponent.h"

using namespace ursine;

namespace
{
    const auto kEditorEntryPoint = "file:///Assets/UI/Resources/Main.html";

    const auto kEditorClearColor = Color( 0xFF252526 );

    const auto kDefaultWindowWidth = 1280;
    const auto kDefaultWindowHeight = 720;
}

CORE_SYSTEM_DEFINITION( Editor );

Editor::Editor(void)
    : m_graphics( nullptr )
    , m_mainWindow( { nullptr } )
    , m_project( nullptr ) { }

Editor::~Editor(void) { }

void Editor::OnInitialize(void)
{
    auto *app = Application::Instance;

    app->Connect(
        APP_UPDATE,
        this,
        &Editor::onAppUpdate
    );

    auto *windowManager = GetCoreSystem( WindowManager );
    auto *uiManager = GetCoreSystem( UIManager );

    m_mainWindow.window = windowManager->AddWindow(
        "Ursine3D Editor",
        { 0, 0 },
        { static_cast<float>( kDefaultWindowWidth ), static_cast<float>( kDefaultWindowHeight ) },
        SDL_WINDOW_RESIZABLE
    );

    m_mainWindow.window->Listener( this )
        .On( WINDOW_RESIZE, &Editor::onMainWindowResize );

    m_mainWindow.window->SetLocationCentered( );
    m_mainWindow.window->Show( true );
    m_mainWindow.window->SetIcon( "Assets/Resources/Icon.png" );

    m_graphics = GetCoreSystem( graphics::GfxAPI );

    initializeGraphics( );

    m_mainWindow.ui = uiManager->CreateView( m_mainWindow.window, kEditorEntryPoint );

    m_mainWindow.ui->SetViewport( {
        0, 0,
        kDefaultWindowWidth, kDefaultWindowHeight
    } );

    m_project = std::make_shared<Project>( m_mainWindow.ui );

    initializeScene( );
}

void Editor::OnRemove(void)
{
    Application::Instance->Disconnect(
        APP_UPDATE,
        this,
        &Editor::onAppUpdate
    );

    m_mainWindow.window->Listener( this )
        .Off( WINDOW_RESIZE, &Editor::onMainWindowResize );

    m_mainWindow.ui->Close( );
    m_mainWindow.ui = nullptr;

    m_mainWindow.window = nullptr;

    m_project = nullptr;
}

std::shared_ptr<Project> Editor::GetProject(void) const
{
    return m_project;
}

void Editor::initializeGraphics(void)
{
    graphics::GfxConfig config;

    config.Fullscreen_ = false;

    config.HandleToWindow_ =
        static_cast<HWND>( m_mainWindow.window->GetPlatformHandle( ) );

    config.ModelListPath_ = "Assets/Models/";
    config.ShaderListPath_ = URSINE_SHADER_BUILD_DIRECTORY;
    config.TextureListPath_ = "Assets/Textures/";
    config.WindowWidth_ = kDefaultWindowWidth;
    config.WindowHeight_ = kDefaultWindowHeight;

    URSINE_TODO( "..." );

    config.m_renderUI = true;
    config.debug = false;

    config.Profile_ = false;

    m_graphics->StartGraphics( config );
    m_graphics->Resize( kDefaultWindowWidth, kDefaultWindowHeight );
}

void Editor::initializeScene(void)
{
    auto &scene = m_project->GetScene( );

    auto &world = scene.GetWorld( );
    {
        auto viewport = m_graphics->ViewportMgr.CreateViewport( 0, 0 );

        auto &handle = m_graphics->ViewportMgr.GetViewport( viewport );

        handle.SetPosition( 0, 0 );

        handle.SetBackgroundColor( 255.0f, 0.0f, 0.0f, 1.0f );

        scene.SetViewport( viewport );

        m_graphics->SetGameViewport( viewport );
    }

    {
        auto *entity_char = world.CreateEntity();

        entity_char->AddComponent<ecs::Renderable>();
        auto model = entity_char->AddComponent<ecs::Model3D>();

        auto name = "Character";

        model->SetModel(name);
        model->GetModel()->SetMaterial("Blank");

        entity_char->SetName(name);

        entity_char->AddComponent<CharacterController>( );

        auto *collider = entity_char->AddComponent<ecs::CapsuleCollider>();
        auto *body = entity_char->AddComponent<ecs::Rigidbody>();
        collider->SetHeight(19.0f);
        collider->SetRadius(4.0f);
        collider->SetOffset(SVec3(0.0f, 15.2f, 0.0f));

        body->LockXRotation(true);
        body->LockZRotation(true);

        auto transform = entity_char->GetTransform();

        transform->SetWorldPosition(SVec3{ 0.0f, 10.0f, 0.0f });
        transform->SetWorldRotation(SQuat{ 0.0f, 0.0f, 0.0f });
        transform->SetWorldScale(SVec3{ 1.0f, 1.0f, 1.0f });
    }

    {
        auto *floor = world.CreateEntity();

        floor->AddComponent<ecs::Renderable>();
        auto model = floor->AddComponent<ecs::Model3D>();
        model->SetModel("Cube");

        floor->AddComponent<ecs::BoxCollider>();
        
        floor->GetTransform()->SetWorldScale(SVec3(100, 0.1f, 100));
    }

    auto *univLight = world.CreateEntity( "Global Light" );
    {
        auto *component = univLight->AddComponent<ecs::Light>( );

        component->SetType( ecs::LightType::Directional );
        component->SetPosition( { 0.0f, 0.0f, 0.0f } );
        component->SetRadius( 40.0f );
        component->SetDirection( { 0.0f, 1.0f, 0.0f } );
        component->SetColor( Color::White );
    }
}

void Editor::onAppUpdate(EVENT_HANDLER(Application))
{
    EVENT_ATTRS(Application, EventArgs);

    auto dt = sender->GetDeltaTime( );

    auto &scene = m_project->GetScene( );

    scene.Update( dt );

    m_graphics->StartFrame( );

    scene.Render( );

    m_mainWindow.ui->DrawMain( );

    m_graphics->EndFrame( );
}

void Editor::onMainWindowResize(EVENT_HANDLER(Window))
{
    EVENT_ATTRS(Window, WindowResizeArgs);

    m_graphics->Resize( args->width, args->height );

    m_mainWindow.ui->SetViewport( {
        0, 0,
        args->width, args->height
    } );
}
