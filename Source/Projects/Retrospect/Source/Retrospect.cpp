#include "Precompiled.h"

#include "Retrospect.h"

#include <Application.h>

#include <WindowManager.h>
#include <UIManager.h>
#include <ScreenManager.h>

#include <Color.h>
#include <Vec3.h>
#include <CameraComponent.h>
#include <LightComponent.h>
#include <WorldSerializer.h>

#include <AudioManager.h>

using namespace ursine;

namespace
{
    const auto kEntryPoint = "file:///Assets/UI/Resources/Main.html";
    const auto kStartWorld = "Assets/Worlds/SubmissionSemester1-Cpt.uworld";

    const auto kDefaultWindowWidth = 1280;
    const auto kDefaultWindowHeight = 720;
}

JSFunction(InitGame)
{
    gScreenManager->AddOverlay( "MainMenu" );

    return CefV8Value::CreateUndefined( );
}

CORE_SYSTEM_DEFINITION( Retrospect );

Retrospect::Retrospect(void)
    : m_graphics( nullptr )
    , m_mainWindow( { nullptr } )
{

}

Retrospect::~Retrospect(void)
{

}

void Retrospect::OnInitialize(void)
{
    auto *app = Application::Instance;

    app->Connect( APP_UPDATE, this, &Retrospect::onAppUpdate );

    auto *windowManager = GetCoreSystem( WindowManager );
    auto *uiManager = GetCoreSystem( UIManager );

    m_mainWindow.window = windowManager->AddWindow(
        "Retrospect",
        { 0, 0 },
        { static_cast<float>( kDefaultWindowWidth ), static_cast<float>( kDefaultWindowHeight ) },
        SDL_WINDOW_RESIZABLE
    );

    m_mainWindow.window->Listener( this )
        .On( WINDOW_RESIZE, &Retrospect::onMainWindowResize );

    m_mainWindow.window->SetLocationCentered( );
    m_mainWindow.window->Show( true );
    //m_mainWindow.window->SetFullScreen( true );

    m_graphics = GetCoreSystem( graphics::GfxAPI );

    initializeGraphics( );

    m_mainWindow.ui = uiManager->CreateView( m_mainWindow.window, kEntryPoint );

    m_mainWindow.ui->SetViewport( {
        0, 0,
        kDefaultWindowWidth, kDefaultWindowHeight
    } );

    std::string debugURL( "http://localhost:" );

    debugURL += std::to_string( UIManager::REMOTE_DEBUGGING_PORT );

    ShellExecute(
        nullptr,
        "open",
        debugURL.c_str( ),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    m_screenManager.SetUI( m_mainWindow.ui );

    initializeScene( );
}

void Retrospect::OnRemove(void)
{
    Application::Instance->Disconnect(
        APP_UPDATE,
        this,
        &Retrospect::onAppUpdate
    );

    m_mainWindow.window->Listener( this )
        .Off( WINDOW_RESIZE, &Retrospect::onMainWindowResize );

    m_mainWindow.ui->Close( );
    m_mainWindow.ui = nullptr;
    
    m_mainWindow.window = nullptr;
}

void Retrospect::initializeGraphics(void)
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

void Retrospect::initializeScene(void)
{
    //auto world = m_scene->GetWorld( );
    {
        auto handle = m_graphics->ViewportMgr.CreateViewport( kDefaultWindowWidth, kDefaultWindowHeight );

        auto &viewport = m_graphics->ViewportMgr.GetViewport( handle );

        viewport.SetPosition( 0, 0 );

        viewport.SetBackgroundColor( 255.0f, 0.0f, 0.0f, 1.0f );

        //m_scene->SetViewport( handle );

        m_graphics->SetGameViewport( handle );

        ecs::WorldSerializer serializer;

        //world = serializer.Deserialize( kStartWorld );

        //m_scene->SetWorld( world );
    }

    //world->GetEntityFromName( "Editor Camera" )->Delete( );

    const std::string init = "INIT.bnk";
    const std::string bgm = "BGM.bnk";
    const std::string car = "Car.bnk";
    const std::string RPM = "RPM";

    const std::string play_CarEngine = "Play_RecordableMusic";

    AkBankID initID = AK_INVALID_BANK_ID;
    AkBankID carID = AK_INVALID_BANK_ID;
    AkBankID bgmID = AK_INVALID_BANK_ID;

    const AkGameObjectID GAME_OBJECT_ID_CAR = 100;
    const AkGameObjectID GAME_OBJECT_NON_RECORDABLE = 200;

    auto *audio = GetCoreSystem( AudioManager );

    audio->LoadBank( init, initID );
    audio->LoadBank( car, carID );
    audio->LoadBank( bgm, bgmID );

    audio->RegisterObject( GAME_OBJECT_ID_CAR, 0x08 );
    audio->PlayEvent( play_CarEngine, GAME_OBJECT_ID_CAR );
}

void Retrospect::onAppUpdate(EVENT_HANDLER(ursine::Application))
{
    EVENT_ATTRS(Application, EventArgs);

    auto dt = sender->GetDeltaTime( );

    m_screenManager.Update( );
}

void Retrospect::onMainWindowResize(EVENT_HANDLER(ursine::Window))
{
    EVENT_ATTRS(Window, WindowResizeArgs);

    m_graphics->Resize( args->width, args->height );

    m_mainWindow.ui->SetViewport( {
        0, 0,
        args->width, args->height
    } );
}
