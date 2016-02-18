/* ---------------------------------------------------------------------------
** Team Bear King
** ?2015 DigiPen Institute of Technology, All Rights Reserved.
**
** GfxManager.cpp
**
** Author:
** - Matt Yan - m.yan@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** -------------------------------------------------------------------------*/

#include "UrsinePrecompiled.h"
#include "GfxManager.h"

//@Matt
#include "VertexDefinitions.h"
#include <complex>
#include "DepthStencilStateList.h"
#include <d3d11.h>
#include <Core/Graphics/Animation/Builder/AnimationState.h>
#include <Core/Graphics/Animation/Builder/AnimationBuilder.h>

static int tempID = -1;
static HWND wHND = 0;

namespace ursine
{
    namespace graphics
    {
        bool sort(_DRAWHND &h1, _DRAWHND &h2)
        {
            if ( *reinterpret_cast<unsigned long long*>(&h1) < *reinterpret_cast<unsigned long long*>(&h2) )
                return true;
            return false;
        }

        void GfxManager::Initialize(GfxConfig &config)
        {
            /////////////////////////////////////////////////////////////////
            // ALLOCATE MANAGERS ////////////////////////////////////////////
            dxCore = new DXCore::DirectXCore;
            gfxInfo = new GfxInfo;
            shaderManager = new DXCore::ShaderManager;
            bufferManager = new DXCore::ShaderBufferManager;
            layoutManager = new DXCore::InputLayoutManager;
            modelManager = new ModelManager;
            renderableManager = new RenderableManager;
            cameraManager = new CameraManager;
            textureManager = new TextureManager;
            viewportManager = new ViewportManager;
            uiManager = new GfxUIManager;
            drawingManager = new DrawingManager;
            gfxProfiler = new GfxProfiler;

            /////////////////////////////////////////////////////////////////
            // INITIALIZING /////////////////////////////////////////////////
            m_drawList.resize(MAX_DRAW_CALLS);
            m_drawCount = 0;
            m_profile = config.enableProfiling;
            m_threadHandle = nullptr;
            m_debug = config.enableDebugInfo;
            m_currentlyRendering = false;
            m_sceneActive = false;
            m_currentID = -1;

            //writing log stuff
            LogMessage("GRAPHICS");
            LogMessage("Graphics Config:", 1);
            LogMessage("Width: %i", 2, config.windowWidth);
            LogMessage("Height: %i", 2, config.windowHeight);
            LogMessage("Model Path: %s", 2, config.modelListPath.c_str());
            LogMessage("Texture Path: %s", 2, config.textureListPath.c_str());
            LogMessage("Shader Path: %s", 2, config.shaderListPath.c_str());
            LogMessage("Fullscreen: %s", 2, config.fullscreen == true ? "True" : "False");
            LogMessage("GPU Info", 1);
            gfxInfo->Initialize();
            gfxInfo->SetDimensions(config.windowWidth, config.windowHeight);

            wHND = config.handleToWindow;

            /////////////////////////////////////////////////////////////////
            // INITIALIZE MANAGERS //////////////////////////////////////////

            LogMessage("Initialize DirectX", 1);
            dxCore->Initialize(config.windowWidth, config.windowHeight, config.handleToWindow, gfxInfo, config.fullscreen, m_debug);

            LogMessage("Initialize Shaders", 1);
            shaderManager->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext(), config.shaderListPath);

            {
                //load shaders
                shaderManager->LoadShader(SHADER_BASIC, "BasicModelShader");
                shaderManager->LoadShader(SHADER_DIFFUSE, "DiffuseShader");
                shaderManager->LoadShader(SHADER_NORMAL, "NormalShader");
                shaderManager->LoadShader(SHADER_DEFFERED_TEXTURE, "DeferredTextureShader");
                shaderManager->LoadShader(SHADER_DEFERRED_DEPTH, "DeferredDepth");
                shaderManager->LoadShader(SHADER_DEFERRED_DEPTH_NORM, "DeferredDepthNormalMap");
                shaderManager->LoadShader(SHADER_DIRECTIONAL_LIGHT, "DirectionalLightSource");
                shaderManager->LoadShader(SHADER_SPOT_LIGHT, "SpotlightSource");
                shaderManager->LoadShader(SHADER_POINT_LIGHT, "PointLightSource");
                shaderManager->LoadShader(SHADER_QUAD, "QuadShader");
                shaderManager->LoadShader(SHADER_UI, "UIShader");
                shaderManager->LoadShader(SHADER_PRIMITIVE, "PrimitiveShader");
                shaderManager->LoadShader(SHADER_POINT, "PointShader");
                shaderManager->LoadShader(SHADER_SHADOW_PASS, "ShadowMapShader");
                shaderManager->LoadShader(SHADER_BILLBOARD2D, "BillboardedSprite");
                shaderManager->LoadShader(SHADER_PARTICLE, "ParticleShader");
                shaderManager->LoadShader(SHADER_EMISSIVE, "EmissiveShader");
                shaderManager->LoadShader(SHADER_FORWARD, "ForwardRenderer");
                shaderManager->LoadShader(SHADER_SPRITE_TEXT, "SpriteTextShader");
                
                //load compute
                shaderManager->LoadShader(SHADER_MOUSEPOSITION, "MouseTypeID");
            }

            LogMessage("Initialize Buffers", 1);
            bufferManager->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext());

            LogMessage("Initialize Models", 1);
            modelManager->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext(), config.modelListPath);

            renderableManager->Initialize();
            cameraManager->Initialize();

            LogMessage("Initialize Textures", 1);
            textureManager->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext(), config.textureListPath);

            viewportManager->Initialize(dxCore->GetRenderTargetMgr());

            uiManager->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext(), dxCore->GetRenderTargetMgr(), this);

            drawingManager->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext());

            gfxProfiler->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext(), m_profile);

            //create layouts
            layoutManager->Initialize(dxCore->GetDevice(), dxCore->GetDeviceContext(), shaderManager);

            //init drawing manager
            drawingManager->EndScene();

            Invalidate();

            m_ready = true;

            // load the font
            std::string textPath = "Assets/Bitmap Fonts/MainFont.fnt";
            m_font.Load(textPath);

            textureManager->CreateTexture("Font", "Assets/Bitmap Fonts/"  + m_font.GetTextureFiles()[ 0 ], 512, 512);
        }

        void GfxManager::Uninitialize()
        {
            //WaitForSingleObject( m_threadHandle, INFINITE );

            gfxInfo->Uninitialize();
            shaderManager->Uninitialize();
            bufferManager->Uninitialize();
            layoutManager->Uninitialize();
            modelManager->Uninitialize();
            renderableManager->Uninitialize();
            cameraManager->Uninitialize();
            textureManager->Uninitialize();
            viewportManager->Uninitialize();
            uiManager->Uninitialize();
            drawingManager->Uninitialize();
            gfxProfiler->Uninitialize();

            //last
            dxCore->Uninitialize();

            delete gfxInfo;
            delete dxCore;
            delete shaderManager;
            delete bufferManager;
            delete layoutManager;
            delete modelManager;
            delete renderableManager;
            delete cameraManager;
            delete textureManager;
            delete viewportManager;
            delete uiManager;
            delete drawingManager;
            delete gfxProfiler;
        }

        void GfxManager::Render(GfxHND handle)
        {
            UAssert(m_currentlyRendering == true, "Attempted to render an object without starting the frame!");
            UAssert(m_sceneActive == true, "Attempted to render an object without beginning a scene!");

            //convert to renderable handle
            _RENDERABLEHND *render = HND_RENDER(handle);

            //make sure right handle was passed
            UAssert(render->ID_ == ID_RENDERABLE, "Attempted to draw non-valid handle!");

            //make sure we have enough room to render
            UAssert(m_drawCount < MAX_DRAW_CALLS, "Out of available draw calls! Let Matt know, easy fix.");

            //get a new draw call
            _DRAWHND &drawCall = m_drawList[ m_drawCount++ ];
            drawCall.buffer_ = 0;

            switch ( render->Type_ )
            {
                //rendering a model
            case RENDERABLE_MODEL3D:
            {
                Model3D *current = &renderableManager->m_renderableModel3D[ render->Index_ ];

                drawCall.Index_ = render->Index_;
                drawCall.Type_ = render->Type_;
                drawCall.Material_ = textureManager->GetTextureIDByName(current->GetMaterialslName());

                drawCall.Model_ = modelManager->GetModelIDByName(current->GetModelName());
                drawCall.Overdraw_ = current->GetOverdraw();
                drawCall.Shader_ = drawCall.Overdraw_ ? SHADER_OVERDRAW_MODEL : SHADER_DEFERRED_DEPTH;
            }
            break;
            case RENDERABLE_BILLBOARD2D:
            {
                Billboard2D *current = &renderableManager->m_renderableBillboards[ render->Index_ ];

                drawCall.Index_ = render->Index_;
                drawCall.Type_ = render->Type_;
                drawCall.Material_ = textureManager->GetTextureIDByName(current->GetTextureName());

                drawCall.Shader_ = SHADER_BILLBOARD2D;
                drawCall.Overdraw_ = current->GetOverdraw();
            }
            break;
            case RENDERABLE_LIGHT:
            {
                Light *current = &renderableManager->m_renderableLights[ render->Index_ ];

                drawCall.Index_ = render->Index_;
                drawCall.Type_ = render->Type_;
                drawCall.Overdraw_ = current->GetOverdraw();

                switch ( current->GetType() )
                {
                case Light::LIGHT_DIRECTIONAL:
                    drawCall.Shader_ = SHADER_DIRECTIONAL_LIGHT;
                    break;
                case Light::LIGHT_POINT:
                    drawCall.Shader_ = SHADER_POINT_LIGHT;
                    break;
                case Light::LIGHT_SPOTLIGHT:
                    drawCall.Shader_ = SHADER_SPOT_LIGHT;
                    break;
                default:
                    break;
                }
            }
            break;
            case RENDERABLE_PS:
            {
                ParticleSystem *current = &renderableManager->m_renderableParticleSystems[ render->Index_ ];

                drawCall.Index_ = render->Index_;
                drawCall.Type_ = render->Type_;
                drawCall.Overdraw_ = current->GetOverdraw();
                drawCall.Shader_ = SHADER_PARTICLE;
            }
            break;
            case RENDERABLE_SPRITE_TEXT:
            {
                SpriteText *current = &renderableManager->m_renderableSpriteText[ render->Index_ ];

                drawCall.Index_ = render->Index_;
                drawCall.Type_ = render->Type_;
                drawCall.Overdraw_ = current->GetOverdraw();
                drawCall.Shader_ = SHADER_SPRITE_TEXT;
            }
                break;
            default:
                break;
            }
        }

        void GfxManager::StartFrame()
        {
            UAssert(m_currentlyRendering == false, "Attempted to start the frame without ending the last one!");
            m_currentlyRendering = true;

            while ( m_rendering );

            m_rendering = true;

            dxCore->StartDebugEvent("FrameStart");

            dxCore->ClearSwapchain();
            dxCore->ClearTargetBuffers();
            dxCore->ClearDebugBuffer();
            gfxProfiler->BeginFrame();

            //cache state of all graphics objects
            renderableManager->CacheFrame();

            float colorArray[ 4 ] = { 0,0,0,1 };
            dxCore->GetDeviceContext()->ClearRenderTargetView(dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_DEFERRED_SPECPOW)->RenderTargetView, colorArray);
        }

        void GfxManager::BeginScene()
        {
            UAssert(m_currentlyRendering == true, "Attempted to begin a scene without starting the frame!");
            UAssert(m_sceneActive == false, "Attempted to begin a scene without ending the last one!");
            m_sceneActive = true;

            //clear draw call list
            memset(reinterpret_cast<unsigned long long*>(&m_drawList[ 0 ]), 0, sizeof(unsigned long long) * m_drawCount * 2);
            m_drawCount = 0;

            //clear debug buffer
            dxCore->ClearDebugBuffer();
        }

        void GfxManager::RenderScene(float dt, GfxHND camera)
        {
            UAssert(m_currentlyRendering == true, "Attempted to render a scene without starting the frame!");
            UAssert(m_sceneActive == true, "Attempted to render a scene without beginning one!");

            // get viewport
            _RESOURCEHND *camHND = reinterpret_cast<_RESOURCEHND*>(&camera);
            UAssert(camHND->ID_ == ID_CAMERA, "Attempted to render UI with invalid camera!");

            Camera &cam = cameraManager->GetCamera(camera);

            dxCore->StartDebugEvent("CameraRenderScene");

            //get game vp dimensions
            Viewport &gameVP = viewportManager->GetViewport(m_GameViewport);
            D3D11_VIEWPORT gvp = gameVP.GetViewportData();

            //set directx viewport
            float w, h, x, y;
            cam.GetViewportPosition(x, y);
            cam.GetDimensions(w, h);

            w *= gvp.Width;
            h *= gvp.Height;

            x = x * gvp.Width + gvp.TopLeftX;
            y = y * gvp.Height + gvp.TopLeftY;

            D3D11_VIEWPORT vpData;
            vpData.Width = w;
            vpData.Height = h;
            vpData.TopLeftX = x;
            vpData.TopLeftY = y;
            vpData.MinDepth = 0;
            vpData.MaxDepth = 1;

            dxCore->GetDeviceContext()->RSSetViewports(1, &vpData);

            /////////////////////////////////////////////////////////////////
            if ( cam.GetRenderMode() == VIEWPORT_RENDER_DEFERRED )
                RenderScene_Deferred(dt, camera);
            else
                RenderScene_Forward(dt, camera);

            dxCore->EndDebugEvent();
            return;
            //close thread handle if needed
            if ( m_threadHandle != nullptr )
                CloseHandle(m_threadHandle);

            return;
            /*
            auto *data = new threadData;
            data->gfx = this;
            data->dt = dt;
            data->forward = viewportManager->GetRenderMode( viewport ) == VIEWPORT_RENDER_FORWARD;
            data->viewport = viewport;

            m_threadHandle = CreateThread( nullptr, 0, renderBootstrap, data, 0, &m_threadID );*/
        }

        DWORD GfxManager::renderBootstrap(LPVOID lpParam)
        {
            auto *data = reinterpret_cast<threadData*>(lpParam);

            if ( data->forward == true )
            {
                data->gfx->RenderScene_Forward(data->dt, data->viewport);
            }
            else
            {
                data->gfx->RenderScene_Deferred(data->dt, data->viewport);
            }

            data->gfx->EndScene();

            delete lpParam;

            return 0;
        }

        void GfxManager::RenderScene_Deferred(float dt, GfxHND camera)
        {
            /////////////////////////////////////////////////////////////////
            // PRE FRAME STUFF 
            // init buffers for frame
            dxCore->ClearDeferredBuffers();
            dxCore->ClearDepthBuffers();
            dxCore->ClearDebugBuffer();

            // get camera
            Camera &currentCamera = cameraManager->GetCamera(camera);

            //get d3d11 viewport info
            //get game vp dimensions
            Viewport &gameVP = viewportManager->GetViewport(m_GameViewport);
            D3D11_VIEWPORT gvp = gameVP.GetViewportData();

            //set directx viewport
            float w, h;
            currentCamera.GetDimensions(w, h);

            w *= gvp.Width;
            h *= gvp.Height;

            currentCamera.SetScreenDimensions(w, h);
            currentCamera.SetScreenPosition(gvp.TopLeftX, gvp.TopLeftY);

            /////////////////////////////////////////////////////////////////
            // gets the projection matrix and view matrix
            SMat4 proj, view;
            proj = currentCamera.GetProjMatrix();
            view = currentCamera.GetViewMatrix();

            /////////////////////////////////////////////////////////////////
            // SORT ALL DRAW CALLS
            std::sort(m_drawList.begin(), m_drawList.begin() + m_drawCount, sort);

            /////////////////////////////////////////////////////////////////
            // BEGIN RENDERING
            //keep track of where we are 
            int currentIndex = 0;

            dxCore->StartDebugEvent("GBuffer Pass");
            //render 3d models deferred

            PrepFor3DModels(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_DEFERRED_DEPTH )
                Render3DModel(m_drawList[ currentIndex++ ], currentCamera);
            STAMP("Model Rendering");
            dxCore->EndDebugEvent();

            /////////////////////////////////////////////////////////
            // LIGHT PASS
            dxCore->StartDebugEvent("Light Pass");
            PrepForLightPass(view, proj, currentCamera);

            //spot light pass
            PrepForSpotlightPass(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_SPOT_LIGHT )
                RenderSpotLight(m_drawList[ currentIndex++ ], currentCamera, proj);
            STAMP("Spotlight Rendering");

            //point light pass
            PrepForPointLightPass(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_POINT_LIGHT )
                RenderPointLight(m_drawList[ currentIndex++ ], currentCamera, proj);
            STAMP("Point Light Rendering");

            //directional light pass
            PrepForDirectionalLightPass(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_DIRECTIONAL_LIGHT )
                RenderDirectionalLight(m_drawList[ currentIndex++ ], currentCamera);
            STAMP("Directional Light Rendering");

            /////////////////////////////////////////////////////////
            // EMISSIVE
            shaderManager->BindShader(SHADER_EMISSIVE);

            // one fullscreen pass
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));

            dxCore->EndDebugEvent();
            STAMP("Emissive Pass");

            /////////////////////////////////////////////////////////
            //debug 
            PrepFor3DModels(view, proj); // I don't think gets set properly

            dxCore->StartDebugEvent("Debug Pass");
            PrepForDebugRender();
            dxCore->SetRasterState(RASTER_STATE_SOLID_NOCULL);
            RenderDebugPoints(view, proj, currentCamera);
            dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);
            RenderDebugLines(view, proj, currentCamera);

            dxCore->EndDebugEvent();
            STAMP("Debug Pass");

            /////////////////////////////////////////////////////////
            // overdraw pass for weird stuff
            PrepFor3DModels(view, proj);
            {
                dxCore->GetRenderTargetMgr()->SetForwardTargets(dxCore->GetDepthMgr()->GetDepthStencilView(DEPTH_STENCIL_OVERDRAW));

                while ( m_drawList[ currentIndex ].Shader_ == SHADER_OVERDRAW_MODEL )
                    Render3DModel(m_drawList[ currentIndex++ ], currentCamera);

                dxCore->GetRenderTargetMgr()->SetForwardTargets(dxCore->GetDepthMgr()->GetDepthStencilView(DEPTH_STENCIL_MAIN));
            }           

            /////////////////////////////////////////////////////////////////
            // RENDER MAIN //////////////////////////////////////////////////
            dxCore->StartDebugEvent("Final Pass");
            PrepForFinalOutput();

            dxCore->GetDeviceContext()->PSSetShaderResources(0, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_DEFERRED_COLOR)->ShaderMap);
            dxCore->GetDeviceContext()->PSSetShaderResources(1, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_LIGHTMAP)->ShaderMap);
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));

            dxCore->EndDebugEvent();
            STAMP("Main Screen Pass");

            /////////////////////////////////////////////////////////////////
            //render primitive layer
            dxCore->StartDebugEvent("Primitive Pass");
            dxCore->SetDepthState(DEPTH_STATE_NODEPTH_STENCIL);
            dxCore->GetDeviceContext()->PSSetShaderResources(0, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_DEBUG)->ShaderMap);

            shaderManager->BindShader(SHADER_QUAD);
            layoutManager->SetInputLayout(SHADER_QUAD);

            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));
            STAMP("Primitive Pass");

            //overdraw render
            PrepForOverdrawDebugRender(view, proj);
            dxCore->SetRasterState(RASTER_STATE_SOLID_NOCULL);
            RenderDebugPoints(view, proj, currentCamera, true);
            dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);
            RenderDebugLines(view, proj, currentCamera, true);
            STAMP("Overdraw Pass");

            PrepForParticleSystems(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_PARTICLE )
                RenderParticleSystem(m_drawList[ currentIndex++ ], currentCamera);
            STAMP("Forward Particle Pass");

            PrepForBillboard2D(view, proj, currentCamera);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_BILLBOARD2D )
                Render2DBillboard(m_drawList[ currentIndex++ ], currentCamera);
            STAMP("Billboard Rendering");

            PrepForSpriteText(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_SPRITE_TEXT )
                RenderSpriteText(m_drawList[ currentIndex++ ], currentCamera);

            dxCore->EndDebugEvent();

            //clearing all buffers
            textureManager->MapTextureByName("Wire");
            textureManager->MapTextureByName("Wire", 1);
            textureManager->MapTextureByName("Wire", 2);
            textureManager->MapTextureByName("Wire", 3);
        }

        void GfxManager::RenderScene_Forward(float dt, GfxHND camera)
        {
            /////////////////////////////////////////////////////////////////
            // PRE FRAME STUFF 
            // init buffers for frame
            dxCore->ClearDeferredBuffers();
            dxCore->ClearDepthBuffers();
            dxCore->ClearDebugBuffer();

            // get camera
            Camera &currentCamera = cameraManager->GetCamera(camera);

            //get d3d11 viewport info
            //get game vp dimensions
            Viewport &gameVP = viewportManager->GetViewport(m_GameViewport);
            D3D11_VIEWPORT gvp = gameVP.GetViewportData();

            //set directx viewport
            float w, h;
            currentCamera.GetDimensions(w, h);

            w *= gvp.Width;
            h *= gvp.Height;

            currentCamera.SetScreenDimensions(w, h);
            currentCamera.SetScreenPosition(gvp.TopLeftX, gvp.TopLeftY);

            /////////////////////////////////////////////////////////////////
            // gets the projection matrix and view matrix
            SMat4 proj, view;
            proj = currentCamera.GetProjMatrix();
            view = currentCamera.GetViewMatrix();

            /////////////////////////////////////////////////////////////////
            // SORT ALL DRAW CALLS
            std::sort(m_drawList.begin(), m_drawList.begin() + m_drawCount, sort);

            /////////////////////////////////////////////////////////////////
            // BEGIN RENDERING
            //keep track of where we are 
            int currentIndex = 0;

            dxCore->StartDebugEvent("Forward Pass");

            // rendering models
            PrepFor3DModels(view, proj);
            {
                InvProjBuffer ipb;
                SMat4 temp = view;
                temp.Transpose();
                temp.Inverse();

                ipb.invProj = temp.ToD3D();
                currentCamera.GetPlanes(ipb.nearPlane, ipb.farPlane);
                bufferManager->MapBuffer<BUFFER_INV_PROJ>(&ipb, SHADERTYPE_PIXEL);

                shaderManager->BindShader(SHADER_FORWARD);
                dxCore->GetRenderTargetMgr()->SetForwardTargets(dxCore->GetDepthMgr()->GetDepthStencilView(DEPTH_STENCIL_MAIN));
            }

            while ( m_drawList[ currentIndex ].Shader_ == SHADER_DEFERRED_DEPTH )
                Render3DModel(m_drawList[ currentIndex++ ], currentCamera);
            STAMP("Model Rendering");
            dxCore->EndDebugEvent();

            // LIGHT PASS
            dxCore->StartDebugEvent("Light Pass");
            PrepForLightPass(view, proj, currentCamera);

            //spot light pass
            PrepForSpotlightPass(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_SPOT_LIGHT )
                currentIndex++;
            //point light pass
            PrepForPointLightPass(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_POINT_LIGHT )
                currentIndex++;
            //directional light pass
            PrepForDirectionalLightPass(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_DIRECTIONAL_LIGHT )
                currentIndex++;

            /////////////////////////////////////////////////////////
            // overdraw pass for weird stuff
            PrepFor3DModels(view, proj);
            {
                dxCore->GetRenderTargetMgr()->SetDeferredTargets(dxCore->GetDepthMgr()->GetDepthStencilView(DEPTH_STENCIL_OVERDRAW));

                while ( m_drawList[ currentIndex ].Shader_ == SHADER_OVERDRAW_MODEL )
                    Render3DModel(m_drawList[ currentIndex++ ], currentCamera);

                dxCore->GetRenderTargetMgr()->SetDeferredTargets(dxCore->GetDepthMgr()->GetDepthStencilView(DEPTH_STENCIL_MAIN));
            }

            /////////////////////////////////////////////////////////////////
            // RENDER MAIN //////////////////////////////////////////////////
            dxCore->StartDebugEvent("Final Pass");
            PrepForFinalOutput();

            dxCore->EndDebugEvent();
            STAMP("Main Screen Pass");

            /////////////////////////////////////////////////////////////////
            //render primitive layer
            PrepFor3DModels(view, proj); // I don't think gets set properly

            dxCore->StartDebugEvent("Debug Pass");
            PrepForDebugRender();
            dxCore->SetRenderTarget(RENDER_TARGET_SWAPCHAIN);
            dxCore->SetRasterState(RASTER_STATE_SOLID_NOCULL);
            RenderDebugPoints(view, proj, currentCamera);
            dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);
            RenderDebugLines(view, proj, currentCamera);

            dxCore->EndDebugEvent();
            STAMP("Debug Pass");

            //overdraw render
            PrepForOverdrawDebugRender(view, proj);
            dxCore->SetRasterState(RASTER_STATE_SOLID_NOCULL);
            RenderDebugPoints(view, proj, currentCamera, true);
            dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);
            RenderDebugLines(view, proj, currentCamera, true);
            STAMP("Overdraw Pass");

            PrepForParticleSystems(view, proj);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_PARTICLE )
                RenderParticleSystem(m_drawList[ currentIndex++ ], currentCamera);
            STAMP("Forward Particle Pass");

            //render 3d models deferred
            PrepForBillboard2D(view, proj, currentCamera);
            while ( m_drawList[ currentIndex ].Shader_ == SHADER_BILLBOARD2D )
                Render2DBillboard(m_drawList[ currentIndex++ ], currentCamera);
            STAMP("Billboard Rendering");

            dxCore->EndDebugEvent();

            //clearing all buffers
            textureManager->MapTextureByName("Wire");
            textureManager->MapTextureByName("Wire", 1);
            textureManager->MapTextureByName("Wire", 2);
            textureManager->MapTextureByName("Wire", 3);
        }

        void GfxManager::EndScene()
        {
            UAssert(m_currentlyRendering == true, "Attemped to end a scene without starting the frame!");
            UAssert(m_sceneActive == true, "Attempted to end a scene before beginning one!");
            m_sceneActive = false;

            // reset drawing for next scene
            drawingManager->EndScene();
        }

        void GfxManager::EndFrame()
        {
            UAssert(m_sceneActive == false, "Attempted to end the frame without ending the scene!");
            UAssert(m_currentlyRendering == true, "Attemped to end the frame when it was never started!");

            //compute pass for mouse position
            PrepForCompute();
            RenderComputeMousePos();

            m_currentlyRendering = false;

            dxCore->EndDebugEvent();

            // present
            dxCore->SwapChainBuffer();

            // end the frame
            gfxProfiler->EndFrame();

            // end profiler
            gfxProfiler->WaitForCalls(m_profile);

            //end rendering
            m_rendering = false;

            //check to resize
            dxCore->CheckSize();

            //invalidate CPU-side gfx engine for next frame
            dxCore->Invalidate();
            Invalidate();

        }

        // preparing for different stages /////////////////////////////////
        void GfxManager::PrepFor3DModels(const SMat4 &view, const SMat4 &proj)
        {
            float blendFactor[ 4 ] = { 1.f, 1.f, 1.f, 1.f };
            dxCore->GetDeviceContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
            dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);

            shaderManager->BindShader(SHADER_DEFERRED_DEPTH);
            layoutManager->SetInputLayout(SHADER_DEFERRED_DEPTH);

            //set render type
            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            dxCore->GetRenderTargetMgr()->SetDeferredTargets(dxCore->GetDepthMgr()->GetDepthStencilView(DEPTH_STENCIL_MAIN));

            //map the sampler
            textureManager->MapSamplerState(SAMPLER_WRAP_TEX);

            //set culling
            dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);

            bufferManager->MapCameraBuffer(view, proj);

            dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);

            //TEMP
            URSINE_TODO("Remove this");
            bufferManager->MapCameraBuffer(view, proj, SHADERTYPE_GEOMETRY);
        }

        void GfxManager::PrepForBillboard2D(const SMat4& view, const SMat4& proj, Camera &currentCamera)
        {
            float blendFactor[ 4 ] = { 1.f, 1.f, 1.f, 1.f };
            dxCore->GetDeviceContext()->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
            dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);

            //deferred shading
            dxCore->GetRenderTargetMgr()->SetForwardTargets(dxCore->GetDepthMgr()->GetDepthStencilView(DEPTH_STENCIL_MAIN));

            //set input
            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

            //set up buffers
            bufferManager->MapCameraBuffer(view, proj, SHADERTYPE_GEOMETRY);
            PointGeometryBuffer pgb;
            pgb.cameraUp = SVec4(currentCamera.GetUp(), 0).ToD3D();
            pgb.cameraPosition = SVec4(currentCamera.GetPosition(), 1).ToD3D();
            bufferManager->MapBuffer<BUFFER_POINT_GEOM>(&pgb, SHADERTYPE_GEOMETRY);

            //bind shader 
            shaderManager->BindShader(SHADER_BILLBOARD2D);
            layoutManager->SetInputLayout(SHADER_BILLBOARD2D);

            textureManager->MapSamplerState(SAMPLER_WRAP_TEX);

            //set model
            modelManager->BindModel("Sprite");
            dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);
            dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);
        }

        void GfxManager::PrepForParticleSystems(const SMat4 & view, const SMat4 & proj)
        {
            // set index buffer
            modelManager->BindModel("ParticleIndices", 0, true);

            // set topology
            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            layoutManager->SetInputLayout(SHADER_COUNT);

            // set inv view, viewproj matrices
            bufferManager->MapCameraBuffer(view, proj);

            //map inv view
            InvProjBuffer ipb;
            SMat4 temp = view;
            temp.Transpose();
            temp.Inverse();

            ipb.invProj = temp.ToD3D();
            bufferManager->MapBuffer<BUFFER_INV_PROJ>(&ipb, SHADERTYPE_VERTEX);

            // bind shader
            shaderManager->BindShader(SHADER_PARTICLE);

            dxCore->SetRasterState(RASTER_STATE_SOLID_NOCULL);

            dxCore->SetRenderTarget(RENDER_TARGET_SWAPCHAIN);

            dxCore->SetBlendState(BLEND_STATE_ADDITIVE);
            dxCore->SetDepthState(DEPTH_STATE_CHECKDEPTH_NOWRITE_NOSTENCIL);
        }

        void GfxManager::PrepForCompute(void)
        {
#if defined(URSINE_WITH_EDITOR)

            //set states
            dxCore->SetRenderTarget(RENDER_TARGET_LIGHTMAP, false);
            dxCore->SetBlendState(BLEND_STATE_ADDITIVE);
            dxCore->SetDepthState(DEPTH_STATE_NODEPTH_NOSTENCIL);
            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            //bind shader
            shaderManager->BindShader(SHADER_MOUSEPOSITION);
#endif
        }

        void GfxManager::PrepForLightPass(const SMat4 &view, const SMat4 &proj, Camera &currentCamera)
        {
            //set states
            dxCore->SetRenderTarget(RENDER_TARGET_LIGHTMAP, false);
            dxCore->SetBlendState(BLEND_STATE_ADDITIVE);
            dxCore->SetDepthState(DEPTH_STATE_NODEPTH_NOSTENCIL);
            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            //map inv proj
            InvProjBuffer ipb;
            SMat4 temp = proj;
            temp.Transpose();
            temp.Inverse();

            ipb.invProj = temp.ToD3D();
            currentCamera.GetPlanes(ipb.nearPlane, ipb.farPlane);
            bufferManager->MapBuffer<BUFFER_INV_PROJ>(&ipb, SHADERTYPE_PIXEL);

            //map camera buffer
            bufferManager->MapCameraBuffer(view, proj);

            //set gbuffer resources
            ID3D11ShaderResourceView *srv = dxCore->GetDepthMgr()->GetDepthStencilSRV(DEPTH_STENCIL_MAIN);
            dxCore->GetDeviceContext()->PSSetShaderResources(0, 1, &srv);
            dxCore->GetDeviceContext()->PSSetShaderResources(1, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_DEFERRED_COLOR)->ShaderMap);
            dxCore->GetDeviceContext()->PSSetShaderResources(2, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_DEFERRED_NORMAL)->ShaderMap);
            dxCore->GetDeviceContext()->PSSetShaderResources(3, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_DEFERRED_SPECPOW)->ShaderMap);
        }

        void GfxManager::PrepForPointLightPass(const SMat4 &view, const SMat4 &proj)
        {
            modelManager->BindModel(modelManager->GetModelIDByName("Sphere"));

            //bind shader
            shaderManager->BindShader(SHADER_POINT_LIGHT);
            layoutManager->SetInputLayout(SHADER_POINT_LIGHT);
        }

        void GfxManager::PrepForSpotlightPass(const SMat4& view, const SMat4& proj)
        {
            //bind model
            modelManager->BindModel(modelManager->GetModelIDByName("lightCone"));

            //bind shader
            shaderManager->BindShader(SHADER_SPOT_LIGHT);
            layoutManager->SetInputLayout(SHADER_SPOT_LIGHT);
        }

        void GfxManager::PrepForDirectionalLightPass(const SMat4 &view, const SMat4 &proj)
        {
            modelManager->BindModel(modelManager->GetModelIDByName("internalQuad"));

            shaderManager->BindShader(SHADER_DIRECTIONAL_LIGHT);
            layoutManager->SetInputLayout(SHADER_DIRECTIONAL_LIGHT);
            dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);

            bufferManager->MapCameraBuffer(SMat4::Identity(), SMat4::Identity());
            bufferManager->MapTransformBuffer(SMat4(-2, 2, 1));
        }

        void GfxManager::PrepForDebugRender()
        {
            dxCore->SetBlendState(BLEND_STATE_NONE);
            dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);
            dxCore->SetRenderTarget(RENDER_TARGET_DEBUG);
            bufferManager->MapTransformBuffer(SMat4::Identity());
            dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);
        }

        void GfxManager::PrepForFinalOutput()
        {
            dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);
            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            dxCore->SetDepthState(DEPTH_STATE_NODEPTH_NOSTENCIL);
            dxCore->SetRenderTarget(RENDER_TARGET_SWAPCHAIN);
            dxCore->SetBlendState(BLEND_STATE_DEFAULT);

            modelManager->BindModel(modelManager->GetModelIDByName("internalQuad"));
            shaderManager->BindShader(SHADER_DEFFERED_TEXTURE);
            layoutManager->SetInputLayout(SHADER_DEFFERED_TEXTURE);

            bufferManager->MapCameraBuffer(SMat4::Identity(), SMat4::Identity());
            bufferManager->MapTransformBuffer(SMat4(-2, 2, 1));
        }

        void GfxManager::PrepForUI()
        {
            SMat4 trans;
            trans.Translate(SVec3(0, 0, 0.1f));

            dxCore->SetBlendState(BLEND_STATE_DEFAULT);
            dxCore->SetRasterState(RASTER_STATE_UI);
            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            dxCore->SetDepthState(DEPTH_STATE_NODEPTH_NOSTENCIL);
            dxCore->SetRenderTarget(RENDER_TARGET_SWAPCHAIN);
            bufferManager->MapCameraBuffer(SMat4::Identity(), SMat4::Identity());

            textureManager->MapSamplerState(SAMPLER_NO_FILTERING);
            shaderManager->BindShader(SHADER_UI);
            layoutManager->SetInputLayout(SHADER_UI);
            bufferManager->MapTransformBuffer(SMat4(-2, 2, 1) * trans);
            modelManager->BindModel(modelManager->GetModelIDByName("internalQuad"));
        }

        void GfxManager::PrepForOverdrawDebugRender(const SMat4 &view, const SMat4 &proj)
        {
            dxCore->SetBlendState(BLEND_STATE_NONE);
            dxCore->SetDepthState(DEPTH_STATE_NODEPTH_NOSTENCIL);
            dxCore->SetRenderTarget(RENDER_TARGET_SWAPCHAIN);
            bufferManager->MapTransformBuffer(SMat4::Identity());
            dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);
            bufferManager->MapCameraBuffer(view, proj);
        }

        void GfxManager::PrepForSpriteText(const SMat4& view, const SMat4& proj)
        {
            textureManager->MapTextureByName("Font");
            shaderManager->BindShader(SHADER_SPRITE_TEXT);
            layoutManager->SetInputLayout(SHADER_SPRITE_TEXT);
            bufferManager->MapTransformBuffer(SMat4::Identity());

            dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            layoutManager->SetInputLayout(SHADER_COUNT);

            // raster
            dxCore->SetRasterState(RASTER_STATE_SOLID_NOCULL);

            // rt
            dxCore->SetRenderTarget(RENDER_TARGET_SWAPCHAIN);

            // blend and depth
            dxCore->SetBlendState(BLEND_STATE_DEFAULT);
            dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);

            // set index buffer
            modelManager->BindModel("ParticleIndices", 0, true);

            //map inv view
            InvProjBuffer ipb;
            SMat4 temp = view;
            temp.Transpose();
            temp.Inverse();

            ipb.invProj = temp.ToD3D();
            bufferManager->MapBuffer<BUFFER_INV_PROJ>(&ipb, SHADERTYPE_VERTEX);
        }

        // rendering //////////////////////////////////////////////////////
        void GfxManager::Render3DModel(_DRAWHND handle, Camera &currentcamera)
        {
            Model3D &current = renderableManager->m_renderableModel3D[ handle.Index_ ];

            if ( !current.m_active )
                return;

            if ( currentcamera.CheckMask(current.GetRenderMask()) )
                return;

            /////////////////////////////////////////////////////////
            // map color
            Color c = renderableManager->m_renderableModel3D[ handle.Index_ ].GetColor();
            PrimitiveColorBuffer pcb;
            pcb.color.x = c.r;
            pcb.color.y = c.g;
            pcb.color.z = c.b;
            pcb.color.w = c.a;
            bufferManager->MapBuffer<BUFFER_PRIM_COLOR>(&pcb, SHADERTYPE_PIXEL);

            /////////////////////////////////////////////////////////
            // material buffer 
            MaterialDataBuffer mdb;

            // get material data
            current.GetMaterialData(mdb.emissive, mdb.specularPower, mdb.specularIntensity);

            // set unique ID for this model
            int overdrw = current.GetOverdraw() == true ? 1 : 0;

            //             16                8
            mdb.id = (handle.Index_) | (handle.Type_ << 12) | (overdrw << 15) | (1 << 11);

            // map buffer
            bufferManager->MapBuffer<BUFFER_MATERIAL_DATA>(&mdb, SHADERTYPE_PIXEL);

            /////////////////////////////////////////////////////////
            // map matrix palette
            MatrixPalBuffer data;

            int index = 0;
            for ( auto &x : current.GetMatrixPalette() )
            {
                data.matPal.matPal[ index++ ] = x.ToD3D();
            }
            bufferManager->MapBuffer<BUFFER_MATRIX_PAL, MatrixPalBuffer>(&data, SHADERTYPE_VERTEX);

            /////////////////////////////////////////////////////////
            // map texture
            textureManager->MapTextureByID(handle.Material_);

            //render
            unsigned count = modelManager->GetModelMeshCount(handle.Model_);

            auto *modelResource = modelManager->GetModel(handle.Model_);

            /////////////////////////////////////////////////////////
            // If the whole model is to be rendered
            if ( current.GetMeshIndex() == -1 )
            {
                // rendering each model
                for ( unsigned x = 0; x < count; ++x )
                {
                    auto *mesh = modelResource->GetMesh(x);

                    // map transform
                    bufferManager->MapTransformBuffer(renderableManager->m_renderableModel3D[ handle.Index_ ].GetWorldMatrix());

                    // set model
                    modelManager->BindModel(handle.Model_, x);
                    shaderManager->Render(modelManager->GetModelIndexcountByID(handle.Model_, x));
                }

                //render debug lines
                if ( current.GetDebug() )
                {
                    dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);
                    dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);

                    pcb.color.x = 0.75f;
                    pcb.color.y = 0.75f;
                    pcb.color.z = 0.45f;
                    bufferManager->MapBuffer<BUFFER_PRIM_COLOR>(&pcb, SHADERTYPE_PIXEL);

                    mdb.emissive = 4;
                    mdb.specularPower = 0;
                    mdb.specularIntensity = 0;
                    bufferManager->MapBuffer<BUFFER_MATERIAL_DATA>(&mdb, SHADERTYPE_PIXEL);
                    textureManager->MapTextureByName("Blank");

                    for ( uint x = 0; x < count; ++x )
                    {
                        // set model
                        modelManager->BindModel(handle.Model_, x);
                        shaderManager->Render(modelManager->GetModelIndexcountByID(handle.Model_, x));
                    }

                    dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);
                    dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);
                }
            }
            /////////////////////////////////////////////////////////
            // SPECIFIC RENDERING
            else
            {
                int x = current.GetMeshIndex();

                auto *mesh = modelResource->GetMesh(x);

                // map transform
                bufferManager->MapTransformBuffer(renderableManager->m_renderableModel3D[ handle.Index_ ].GetWorldMatrix() /** mesh->GetLocalToParentTransform( )*/);

                // set model
                modelManager->BindModel(handle.Model_, x);
                shaderManager->Render(modelManager->GetModelIndexcountByID(handle.Model_, x));


                //render debug lines
                Model3D &model = renderableManager->m_renderableModel3D[ handle.Index_ ];
                if ( model.GetDebug() )
                {
                    dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);
                    dxCore->SetRasterState(RASTER_STATE_LINE_RENDERING);

                    pcb.color.x = 0.75f;
                    pcb.color.y = 0.75f;
                    pcb.color.z = 0.45f;
                    bufferManager->MapBuffer<BUFFER_PRIM_COLOR>(&pcb, SHADERTYPE_PIXEL);

                    mdb.emissive = 4;
                    mdb.specularPower = 0;
                    mdb.specularIntensity = 0;
                    bufferManager->MapBuffer<BUFFER_MATERIAL_DATA>(&mdb, SHADERTYPE_PIXEL);
                    textureManager->MapTextureByName("Blank");

                    // set model
                    modelManager->BindModel(handle.Model_, x);
                    shaderManager->Render(modelManager->GetModelIndexcountByID(handle.Model_, x));

                    dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);
                    dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);
                }
            }
        }

        void GfxManager::Render2DBillboard(_DRAWHND handle, Camera &currentCamera)
        {
            auto billboard = renderableManager->m_renderableBillboards[ handle.Index_ ];

            if ( !billboard.m_active )
                return;

            if ( currentCamera.CheckMask(billboard.GetRenderMask()) )
                return;

            BillboardSpriteBuffer bsb;

            billboard.GetDimensions(bsb.width, bsb.height);

            //map camera data
            PointGeometryBuffer pgb;
            pgb.cameraUp = SVec4(currentCamera.GetUp(), 0).ToD3D();
            pgb.cameraPosition = SVec4(currentCamera.GetPosition(), 1).ToD3D();
            bufferManager->MapBuffer<BUFFER_POINT_GEOM>(&pgb, SHADERTYPE_GEOMETRY);

            //map color data
            Color c = billboard.GetColor();
            PrimitiveColorBuffer pcb;
            pcb.color.x = c.r;
            pcb.color.y = c.g;
            pcb.color.z = c.b;
            pcb.color.w = c.a;
            bufferManager->MapBuffer<BUFFER_PRIM_COLOR>(&pcb, SHADERTYPE_PIXEL);

            //map sprite data
            bufferManager->MapBuffer<BUFFER_BILLBOARDSPRITE>(&bsb, SHADERTYPE_GEOMETRY);

            //map transform
            bufferManager->MapTransformBuffer(SMat4(billboard.GetPosition()));

            //map id data
            MaterialDataBuffer mdb;

            //set unique ID for this model
            int overdrw = billboard.GetOverdraw() == true ? 1 : 0;

            mdb.emissive = 1.f;
            //             16                8
            mdb.id = (handle.Index_) | (handle.Type_ << 12) | (overdrw << 15) | (1 << 11);
            bufferManager->MapBuffer<BUFFER_MATERIAL_DATA>(&mdb, SHADERTYPE_PIXEL);

            //map texture
            textureManager->MapTextureByID(handle.Material_);

            //set overdraw value
            if ( handle.Overdraw_ )
                dxCore->SetDepthState(DEPTH_STATE_PASSDEPTH_WRITESTENCIL);
            else
                dxCore->SetDepthState(DEPTH_STATE_DEPTH_NOSTENCIL);

            //render
            shaderManager->Render(1);
        }

        void GfxManager::RenderParticleSystem(_DRAWHND handle, Camera &currentCamera)
        {
            auto &particleSystem = renderableManager->m_renderableParticleSystems[ handle.Index_ ];

            if ( !particleSystem.m_active )
                return;

            if ( currentCamera.CheckMask(particleSystem.GetRenderMask()) )
                return;

            if ( particleSystem.GetAdditive() )
                dxCore->SetBlendState(BLEND_STATE_ADDITIVE);
            else
                dxCore->SetBlendState(BLEND_STATE_DEFAULT);

            // bind color and space
            PointGeometryBuffer pgb;

            if ( particleSystem.GetSystemSpace() )
            {
                // use world space
                pgb.cameraPosition = DirectX::XMFLOAT4(0, 0, 0, 0);
            }
            else
            {
                // use local space
                pgb.cameraPosition = DirectX::XMFLOAT4(particleSystem.GetPosition().GetFloatPtr());
            }

            // set color
            auto &color = particleSystem.GetColor();
            pgb.cameraUp.x = color.r;
            pgb.cameraUp.y = color.g;
            pgb.cameraUp.z = color.b;
            pgb.cameraUp.w = color.a;

            bufferManager->MapBuffer<BUFFER_POINT_GEOM>(&pgb, SHADERTYPE_VERTEX);

            textureManager->MapTextureByName(particleSystem.GetParticleTexture());

            // material buffer 
            MaterialDataBuffer mdb;

            // set unique ID for this model
            int overdrw = particleSystem.GetOverdraw() == true ? 1 : 0;

            //             16                8
            mdb.id = (handle.Index_) | (handle.Type_ << 12) | (overdrw << 15) | (1 << 11);

            // map buffer
            bufferManager->MapBuffer<BUFFER_MATERIAL_DATA>(&mdb, SHADERTYPE_PIXEL);

            if ( currentCamera.CheckMask(particleSystem.GetRenderMask()) )
                return;

            if ( particleSystem.GetActiveParticleCount() > 0 )
            {
                unsigned passCount;
                if ( particleSystem.GetActiveParticleCount() > 1024 )
                    passCount = (particleSystem.GetActiveParticleCount() / 1024) + 1;
                else
                    passCount = 1;

                if ( currentCamera.GetRenderMode() == ViewportRenderMode::VIEWPORT_RENDER_FORWARD )
                    passCount = 1;

                unsigned totalParticlesToRender = particleSystem.GetActiveParticleCount();

                for ( unsigned x = 0; x < passCount; ++x )
                {
                    unsigned particlesInPass = 1024;

                    if ( x == passCount - 1 )
                        particlesInPass = particleSystem.GetActiveParticleCount() % 1024;

                    // bind particle data
                    ParticleBuffer pb;
                    memcpy(&pb.data, &(particleSystem.GetGPUParticleData()[ x * 1024 ]), sizeof(Particle_GPU) * particlesInPass);

                    bufferManager->MapBuffer<BUFFER_PARTICLEDATA>(&pb, SHADERTYPE_VERTEX, 13);

                    if ( particlesInPass != 0 )
                        shaderManager->Render(6 * particlesInPass);

                    // update particles left to render
                    totalParticlesToRender -= particlesInPass;

                }
            }
        }

        void GfxManager::RenderComputeMousePos()
        {
            MouseBuffer dataToCS;

            POINT point;
            GetCursorPos(&point);

            ScreenToClient(wHND, &point);

            if ( point.x < 0 ) point.x = 0;
            if ( point.y < 0 ) point.y = 0;

            //@matt set proper mouse position
            dataToCS.mousePos = DirectX::XMINT4(point.x, point.y, 0, 0);

            //bind shader
            shaderManager->BindShader(SHADER_MOUSEPOSITION);

            //set input
            bufferManager->MapBuffer<BUFFER_MOUSEPOS>(&dataToCS, SHADERTYPE_COMPUTE, 0);
            dxCore->GetDeviceContext()->CSSetShaderResources(0, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(RENDER_TARGET_DEFERRED_SPECPOW)->ShaderMap);
            ID3D11ShaderResourceView *srv = dxCore->GetDepthMgr()->GetDepthStencilSRV(DEPTH_STENCIL_MAIN);
            dxCore->GetDeviceContext()->CSSetShaderResources(1, 1, &srv);

            //set UAV as output 
            dxCore->GetDeviceContext()->CSSetUnorderedAccessViews(COMPUTE_BUFFER_ID, 1, &bufferManager->m_computeUAV[ COMPUTE_BUFFER_ID ], nullptr);
            dxCore->GetDeviceContext()->CSSetUnorderedAccessViews(COMPUTE_BUFFER_ID_CPU, 1, &bufferManager->m_computeUAV[ COMPUTE_BUFFER_ID_CPU ], nullptr);

            //call the compute shader. Results *should* be written to the UAV above
            dxCore->GetDeviceContext()->Dispatch(1, 1, 1);

            //copy data to intermediary buffer
            //                                                           CPU read-only staging buffer                            GPU compute output
            dxCore->GetDeviceContext()->CopyResource(bufferManager->m_computeBufferArray[ COMPUTE_BUFFER_ID_CPU ], bufferManager->m_computeBufferArray[ COMPUTE_BUFFER_ID ]);

            //read from intermediary buffer
            ComputeIDOutput dataFromCS;
            bufferManager->ReadComputeBuffer<COMPUTE_BUFFER_ID_CPU>(&dataFromCS, SHADERTYPE_COMPUTE);

            dxCore->GetDeviceContext()->CSSetShaderResources(0, 0, nullptr);

            m_currentPosition = SVec3(static_cast<float>(point.x), static_cast<float>(point.y), dataFromCS.depth);

            tempID = dataFromCS.id;

            int index = tempID & 0x7FF;
            int type = (tempID >> 12) & 0x3;
            int overdraw = (tempID >> 15) & 0x1;

            unsigned w, h;
            gfxInfo->GetDimensions(w, h);

            if ( tempID != -1 && tempID < 73727 && (unsigned)point.x < w && (unsigned)point.y < h )
            {
                switch ( type )
                {
                case RENDERABLE_MODEL3D:
                    m_currentID = renderableManager->m_renderableModel3D[ index ].GetEntityUniqueID();
                    break;
                case RENDERABLE_BILLBOARD2D:
                    m_currentID = renderableManager->m_renderableBillboards[ index ].GetEntityUniqueID();
                    break;
                }
            }
            else
                m_currentID = -1;
        }

        void GfxManager::RenderPointLight(_DRAWHND handle, Camera &currentCamera, SMat4 &proj)
        {
            Light &pl = renderableManager->m_renderableLights[ handle.Index_ ];

            if ( !pl.m_active )
                return;

            if ( currentCamera.CheckMask(pl.GetRenderMask()) )
                return;

            //get data
            float radius = pl.GetRadius();

            //domain shader needs light proj
            SMat4 lightProj;
            lightProj = SMat4(radius, radius, radius); //scaling
            lightProj *= SMat4(pl.GetPosition()); //translate to world space
            lightProj *= currentCamera.GetViewMatrix(); //transform into view space
            lightProj *= proj; //transform into screeen space

                               //map
            bufferManager->MapBuffer<BUFFER_LIGHT_PROJ>(&lightProj, SHADERTYPE_DOMAIN);

            //ps needs point light data buffer
            SMat4 view = currentCamera.GetViewMatrix(); //need to transpose view (dx11 gg)
            view.Transpose();
            SVec3 lightPosition = view.TransformPoint(pl.GetPosition());

            PointLightBuffer pointB;
            pointB.lightPos = lightPosition.ToD3D();
            pointB.lightRadius = pl.GetRadius();
            pointB.intensity = pl.GetIntensity();
            pointB.color.x = pl.GetColor().r;
            pointB.color.y = pl.GetColor().g;
            pointB.color.z = pl.GetColor().b;
            
            bufferManager->MapBuffer<BUFFER_POINT_LIGHT>(&pointB, SHADERTYPE_PIXEL);

            //light transform
            SMat4 transform;
            transform *= SMat4(pl.GetPosition());
            transform *= SMat4(radius, radius, radius);
            bufferManager->MapTransformBuffer(transform);

            //get camera position
            SVec3 tempPos = currentCamera.GetPosition();

            ////get light position
            SVec3 lightP = pl.GetPosition();
            SVec3 camLight = tempPos - lightP;
            float distance = camLight.LengthSquared();
            float radiusSqr = radius * radius;

            if ( radiusSqr > fabs(distance) )
                dxCore->SetRasterState(RASTER_STATE_SOLID_FRONTCULL);
            else
                dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);

            //render!
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("Sphere")));
        }

        void GfxManager::RenderSpotLight(_DRAWHND handle, Camera& currentCamera, SMat4& proj)
        {
            Light &pl = renderableManager->m_renderableLights[ handle.Index_ ];

            if ( !pl.m_active )
                return;

            if ( currentCamera.CheckMask(pl.GetRenderMask()) )
                return;

            SMat4 view = currentCamera.GetViewMatrix(); //need to transpose view (dx11 gg)
            view.Transpose();
            SVec3 lightDirection = view.TransformVector(pl.GetDirection());

            SVec3 lightPosition = view.TransformPoint(pl.GetPosition());

            //spotlight data
            SpotlightBuffer slb;
            slb.diffuseColor = pl.GetColor().ToVector3().ToD3D();
            slb.lightDirection = lightDirection.ToD3D();
            slb.lightPosition = lightPosition.ToD3D();
            slb.intensity = pl.GetIntensity();
            slb.innerAngle = cosf((pl.GetSpotlightAngles().X() / 2.f) * (3.141596f / 180.0f));   //needs to be in radians
            slb.outerAngle = cosf((pl.GetSpotlightAngles().Y() / 2.f) * (3.141596f / 180.0f));
            slb.lightSize = pl.GetRadius( );

            bufferManager->MapBuffer<BUFFER_SPOTLIGHT>(&slb, SHADERTYPE_PIXEL);

            //transform data  
            bufferManager->MapTransformBuffer(pl.GetSpotlightTransform() * SMat4(SVec3(0, 0, 0.5f), SQuat(-90.0f, 0.0f, 0.0f), SVec3::One()));

            //what culling to use?

            // get vector from light to camera
            SVec3 light2Cam = currentCamera.GetPosition( ) - lightPosition;
            light2Cam.Normalize( );
            float dotp = light2Cam.Dot(lightDirection);

            // if camera is outside the cone, use this
            if(dotp < cosf(slb.outerAngle) )
                dxCore->SetRasterState(RASTER_STATE_SOLID_BACKCULL);
            // else, use frontface culling
            else
                dxCore->SetRasterState(RASTER_STATE_SOLID_FRONTCULL);
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("lightCone")));
        }

        void GfxManager::RenderDirectionalLight(_DRAWHND handle, Camera &currentCamera)
        {
            Light &l = renderableManager->m_renderableLights[ handle.Index_ ];

            if ( !l.m_active )
                return;

            if ( currentCamera.CheckMask(l.GetRenderMask()) )
                return;

            SMat4 view = currentCamera.GetViewMatrix(); //need to transpose view (dx11 gg)
            view.Transpose();
            SVec3 lightDirection = view.TransformVector(l.GetDirection());

            DirectionalLightBuffer lightB;
            lightB.lightDirection.x = lightDirection.X();
            lightB.lightDirection.y = lightDirection.Y();
            lightB.lightDirection.z = lightDirection.Z();

            //used as a buffer
            lightB.intensity = l.GetIntensity();

            lightB.lightColor = DirectX::XMFLOAT3(l.GetColor().r * lightB.intensity, l.GetColor().g * lightB.intensity, l.GetColor().b * lightB.intensity);

            bufferManager->MapBuffer<BUFFER_DIRECTIONAL_LIGHT>(&lightB, SHADERTYPE_PIXEL);
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));
        }

        void GfxManager::RenderDebugPoints(const SMat4 &view, const SMat4 &proj, Camera &currentCamera, bool overdraw)
        {
            //render points
            if ( (drawingManager->CheckRenderPoints() && !overdraw) || (drawingManager->CheckOverdrawRenderPoints() && overdraw) )
            {
                ID3D11Buffer *mesh, *indices;
                unsigned vertCount, indexCount;

                if ( !overdraw )
                    drawingManager->ConstructPointMesh(vertCount, indexCount, &mesh, &indices);
                else
                    drawingManager->ConstructOverdrawPointMesh(vertCount, indexCount, &mesh, &indices);

                //set input
                dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

                //set up buffers
                bufferManager->MapCameraBuffer(view, proj, SHADERTYPE_GEOMETRY);
                PointGeometryBuffer pgb;
                pgb.cameraUp = SVec4(currentCamera.GetUp(), 0).ToD3D();
                pgb.cameraPosition = SVec4(currentCamera.GetPosition(), 1).ToD3D();
                bufferManager->MapBuffer<BUFFER_POINT_GEOM>(&pgb, SHADERTYPE_GEOMETRY);

                //bind shader
                shaderManager->BindShader(SHADER_POINT);
                layoutManager->SetInputLayout(SHADER_POINT);

                //map meshes
                modelManager->BindMesh<PrimitiveVertex>(mesh, indices);

                //render
                shaderManager->Render(indexCount);
            }
        }

        void GfxManager::RenderDebugLines(const SMat4 &view, const SMat4 &proj, Camera &currentCamera, bool overdraw)
        {
            //render lines
            if ( (drawingManager->CheckRenderLines() && !overdraw) || (drawingManager->CheckOverdrawRenderLines() && overdraw) )
            {
                ID3D11Buffer *mesh, *indices;
                unsigned vertCount, indexCount;

                if ( !overdraw )
                    drawingManager->ConstructLineMesh(vertCount, indexCount, &mesh, &indices);
                else
                    drawingManager->ConstructOverdrawLineMesh(vertCount, indexCount, &mesh, &indices);

                //set input
                dxCore->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

                //bind shader
                shaderManager->BindShader(SHADER_BASIC);
                layoutManager->SetInputLayout(SHADER_BASIC);

                //map meshes
                modelManager->BindMesh<PrimitiveVertex>(mesh, indices);

                //render
                shaderManager->Render(indexCount);
            }
        }

        void GfxManager::RenderSpriteText(_DRAWHND handle, Camera& currentCamera)
        {
            SpriteText &spriteText = renderableManager->m_renderableSpriteText[ handle.Index_ ];

            // quick culling stuff
            if ( !spriteText.m_active )
                return;
            if ( currentCamera.CheckMask(spriteText.GetRenderMask()) )
                return;

            if ( spriteText.GetText().length() <= 0 )
                return;

            // map color data ///////////////////////////////////////
            PointGeometryBuffer pgb;
            
            // set color
            auto &color = spriteText.GetColor();
            pgb.cameraUp.x = color.r;
            pgb.cameraUp.y = color.g;
            pgb.cameraUp.z = color.b;
            pgb.cameraUp.w = color.a;

            bufferManager->MapBuffer<BUFFER_POINT_GEOM>(&pgb, SHADERTYPE_VERTEX);

            // map text data ////////////////////////////////////////
            SpriteTextBuffer stb;
            stb.worldPosition = spriteText.GetPosition( ).ToD3D( );
            stb.offset = spriteText.GetSize();
            stb.sizeScalar = DirectX::XMFLOAT2(spriteText.GetWidth() * spriteText.GetSize(), spriteText.GetHeight() * spriteText.GetSize());
            stb.textureDimensions = DirectX::XMFLOAT2(
                m_font.GetCommonData().textureDimensions.scaleW, 
                m_font.GetCommonData().textureDimensions.scaleH
            );

            // change sampler
            if ( spriteText.GetFilter() )
                textureManager->MapSamplerState(SAMPLER_WRAP_TEX);
            else
                textureManager->MapSamplerState(SAMPLER_NO_FILTERING);

            /////////////////////////////////////////////////////////
            // mapping glyph data
            Vec2 cursor = Vec2(0, 0);   // "cursor" for text positions

            GlyphBuffer gb;
            auto &text = spriteText.GetText( );
            auto &characterData = m_font.GetCharacterData( );
            auto &commonData = m_font.GetCommonData( );

            // offset data for calculating stuff
            float distance = 0;
            unsigned lineStartIndex = 0;

            // for each glyph, determine position and stuff
            for ( unsigned x = 0; x < text.length( ); ++x)
            {
                auto &currentCharacter = text[ x ];
                auto &currentGlyph = gb.glyphData[ x ];

                auto &currentCharData = characterData[ currentCharacter ];

                // for newline characters
                if(currentCharacter == '\n')
                {
                    // calculate the current offsets so we can center ourselves
                    float lineOffset;

                    switch ( spriteText.GetAlignment() )
                    {
                    case 1:
                        lineOffset = (distance / commonData.textureDimensions.scaleW) * 0.5f;
                        break;
                    case 2:
                        lineOffset = (distance / commonData.textureDimensions.scaleW);
                        break;
                    default:
                        lineOffset = 0;
                        break;
                    }

                    // add it to the current line's characters
                    for ( unsigned y = lineStartIndex; y < x; ++y)
                    {
                        gb.glyphData[ y ].screenPosition.x -= lineOffset;
                    }

                    // reset the stats, don't include the current newline
                    lineStartIndex = x + 1;
                    distance = 0;

                    // increment y
                    cursor += Vec2(0, (commonData.lineHeight / commonData.textureDimensions.scaleH) * stb.sizeScalar.y);

                    continue;
                }

                // First, advance cursor by the xoffset
                cursor += Vec2(currentCharData.offset.x, 0);

                // then, check if we need to kern
                if ( x > 0 )
                {   // grab kerning data for behind us. Check if we need to apply
                    auto kerningData = characterData[ text[ x - 1 ] ].kerningMap.find(static_cast<int16_t>(currentCharacter));

                    // if the last character has us
                    if ( kerningData != characterData[ text[ x - 1 ] ].kerningMap.end() )
                    {
                        // add the kerning data to us
                        cursor += Vec2(kerningData->second, 0);
                    }
                }

                // "render", divide by size of the tex to scale to right size
                currentGlyph.screenPosition = DirectX::XMFLOAT2((cursor.X()) / commonData.textureDimensions.scaleW, cursor.Y());

                currentGlyph.buffer.x = spriteText.GetPPU();

                // set glyph position in tex
                currentGlyph.glyphPosition = DirectX::XMFLOAT2(
                    currentCharData.textureCoordinates.x / commonData.textureDimensions.scaleW,
                    currentCharData.textureCoordinates.y / commonData.textureDimensions.scaleH
                );

                // set glyph size
                currentGlyph.glyphSize = DirectX::XMFLOAT2(
                    currentCharData.textureDimensions.x / commonData.textureDimensions.scaleW,
                    currentCharData.textureDimensions.y / commonData.textureDimensions.scaleH
                );

                cursor += Vec2(currentCharData.xadvance, 0);

                distance = cursor.X( );
            }

            // perform final line adjustment
            // calculate the current offsets so we can center ourselves
            float lineOffset;

            switch ( spriteText.GetAlignment() )
            {
            case 1:
                lineOffset = (distance / commonData.textureDimensions.scaleW) * 0.5f;
                break;
            case 2:
                lineOffset = (distance / commonData.textureDimensions.scaleW);
                break;
            default:
                lineOffset = 0;
                break;
            }

            // add it to the current line's characters
            for ( unsigned y = lineStartIndex; y < text.length( ); ++y )
            {
                gb.glyphData[ y ].screenPosition.x -= lineOffset;
            }

            // DONE, map the data
            bufferManager->MapBuffer<BUFFER_TEXTDATA>(&stb, SHADERTYPE_VERTEX, 7);
            bufferManager->MapBuffer<BUFFER_GLYPHDATA>(&gb, SHADERTYPE_VERTEX, 10);

            shaderManager->Render(6 * static_cast<unsigned>(text.length()));

        }

        void GfxManager::SetGameViewport(GfxHND vp)
        {
            _VIEWPORTHND *viewPort = reinterpret_cast<_VIEWPORTHND*>(&vp);

            UAssert(viewPort->ID_ == ID_VIEWPORT, "Attempted to set game viewport with invalid handle!");

            m_GameViewport = vp;
        }

        void GfxManager::RenderDynamicTexture(GfxHND& texHandle, const float posX, const float posY)
        {
            //get the texture
            Texture *tex = textureManager->GetDynamicTexture(texHandle);

            _RESOURCEHND *handle = HND_RSRCE(texHandle);

            //prep for ui
            PrepForUI();

            //get dimensions
            unsigned width, height;
            gfxInfo->GetDimensions(width, height);

            //will need these to be floats
            float fWidth = static_cast<float>(width);
            float fHeight = static_cast<float>(height);

            //set directx viewport
            D3D11_VIEWPORT vpData = viewportManager->GetViewport(m_GameViewport).GetViewportData();
            unsigned w, h;
            gfxInfo->GetDimensions(w, h);
            vpData.TopLeftX = 0;
            vpData.TopLeftY = 0;
            vpData.Width = static_cast<FLOAT>(w);
            vpData.Height = static_cast<FLOAT>(h);

            dxCore->GetDeviceContext()->RSSetViewports(1, &vpData);

            //calculate position w/ respect to top left
            float finalX = (posX - fWidth / 2.f + tex->m_width / 2.f) / (fWidth / 2.f);
            float finalY = (-posY + height / 2.f - tex->m_height / 2.f) / (fHeight / 2.f);
            SMat4 trans = SMat4(SVec3(finalX, finalY, 0));
            bufferManager->MapTransformBuffer(trans * SMat4(-2 * (tex->m_width / fWidth), 2 * (tex->m_height / fHeight), 1));

            //map tex
            textureManager->MapTextureByID(handle->Index_);

            //render to screen
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));
        }

        void GfxManager::RenderDynamicTextureInViewport(GfxHND& texHandle, const float posX, const float posY, GfxHND& camera)
        {
            _RESOURCEHND *newRender = reinterpret_cast<_RESOURCEHND*>(&camera);

            UAssert(newRender->ID_ == ID_CAMERA, "Attempted to render UI with invalid camera!");

            // get viewport
            Camera &cam = cameraManager->GetCamera(camera);

            //get the texture
            Texture *tex = textureManager->GetDynamicTexture(texHandle);

            _RESOURCEHND *handle = HND_RSRCE(texHandle);

            //prep for ui
            PrepForUI();

            //get dimensions
            unsigned width, height;
            gfxInfo->GetDimensions(width, height);

            //will need these to be floats
            float fWidth = static_cast<float>(width);
            float fHeight = static_cast<float>(height);

            //set directx viewport
            float w, h, x, y;
            Viewport &gameVP = viewportManager->GetViewport(m_GameViewport);
            D3D11_VIEWPORT gvp = gameVP.GetViewportData();
            cam.GetViewportPosition(x, y);
            cam.GetDimensions(w, h);

            w *= gvp.Width;
            h *= gvp.Height;

            x *= gvp.Width;
            y *= gvp.Height;

            D3D11_VIEWPORT vpData;
            vpData.Width = w;
            vpData.Height = h;
            vpData.TopLeftX = x;
            vpData.TopLeftY = y;
            vpData.MinDepth = 0;
            vpData.MaxDepth = 1;

            dxCore->GetDeviceContext()->RSSetViewports(1, &vpData);

            //calculate position w/ respect to top left
            float finalX = (posX - fWidth / 2.f + tex->m_width / 2.f) / (fWidth / 2.f);
            float finalY = (-posY + height / 2.f - tex->m_height / 2.f) / (fHeight / 2.f);
            SMat4 trans = SMat4(SVec3(finalX, finalY, 0));
            bufferManager->MapTransformBuffer(trans * SMat4(-2 * (tex->m_width / fWidth), 2 * (tex->m_height / fHeight), 1));

            //map tex
            textureManager->MapTextureByID(handle->Index_);

            //render to screen
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));
        }

        void GfxManager::RenderToDynamicTexture(const int srcWidth, const int srcHeight, const void* input, const int inputWidth, const int inputHeight, GfxHND destTexture, const int destinationX, const int destinationY)
        {
            //set up description
            D3D11_TEXTURE2D_DESC desc;
            desc.Width = srcWidth;
            desc.Height = srcHeight;
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // dammmnnnn
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            //set up resource
            D3D11_SUBRESOURCE_DATA subrsc;
            subrsc.pSysMem = input;
            subrsc.SysMemPitch = srcWidth * 4; //length of one line in bytes, 32 bit color
            subrsc.SysMemSlicePitch = 0;

            //create the texture
            ID3D11Texture2D *tex;
            HRESULT hr = dxCore->GetDevice()->CreateTexture2D(&desc, &subrsc, &tex);

            UAssert(hr == S_OK, "Failed to create UI texture!");

            //this doesn't work for now
            //define the box of the texture
            D3D11_BOX box; //this box is the taken from the SOURCE texture
            box.back = 1; //this might need SERIOUS changes
            box.front = 0;

            box.left = 0;
            box.top = 0;

            box.right = inputWidth;
            box.bottom = inputHeight;

            //now that we have the texture, we need to write it to the render target
            Texture *target = textureManager->GetDynamicTexture(destTexture);
            dxCore->GetDeviceContext()->CopySubresourceRegion(target->m_texture2d, 0, destinationX, destinationY, 0, tex, 0, &box);


            RELEASE_RESOURCE(tex);
        }

        int GfxManager::GetCurrentUniqueID()
        {
            return m_currentID;
        }

        SVec3 GfxManager::GetCurrentWorldPosition(const GfxHND& cameraHandle)
        {
            auto &camera = cameraManager->GetCamera(cameraHandle);

            // get the saved depth
            float depth = m_currentPosition.Z();

            // transform from screen to world, given a specific camera
            auto worldPosition = camera.ScreenToWorld(Vec2(m_currentPosition.X(), m_currentPosition.Y()), depth);

            return worldPosition;
        }

        // misc stuff /////////////////////////////////////////////////////
        DXCore::DirectXCore *GfxManager::GetDXCore()
        {
            return dxCore;
        }

        void GfxManager::Resize(int width, int height)
        {
            if ( !m_ready )
                return;

            gfxInfo->SetDimensions(width, height);

            //MAIN render targets, not viewports
            dxCore->ResizeDX(width, height);

            Invalidate();
        }

        void GfxManager::SetFullscreenState(const bool state)
        {
            dxCore->SetFullscreenState(state);
        }

        void GfxManager::Invalidate()
        {
            dxCore->Invalidate();
            shaderManager->Invalidate();
            layoutManager->Invalidate();
        }

        void GfxManager::RenderUI(GfxHND camera, RENDER_TARGETS input)
        {
            _RESOURCEHND *newRender = reinterpret_cast<_RESOURCEHND*>(&camera);

            UAssert(newRender->ID_ == ID_CAMERA, "Attempted to render UI with invalid camera!");

            PrepForUI();

            // get viewport
            Camera &cam = cameraManager->GetCamera(camera);

            //get game vp dimensions
            Viewport &gameVP = viewportManager->GetViewport(m_GameViewport);
            D3D11_VIEWPORT gvp = gameVP.GetViewportData();

            //set directx viewport
            float w, h, x, y;
            cam.GetViewportPosition(x, y);
            cam.GetDimensions(w, h);

            w *= gvp.Width;
            h *= gvp.Height;

            x *= gvp.Width;
            y *= gvp.Height;

            D3D11_VIEWPORT vpData;
            vpData.Width = w;
            vpData.Height = h;
            vpData.TopLeftX = x;
            vpData.TopLeftY = y;
            vpData.MinDepth = 0;
            vpData.MaxDepth = 1;

            dxCore->GetDeviceContext()->RSSetViewports(1, &vpData);

            dxCore->GetDeviceContext()->PSSetShaderResources(0, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(input)->ShaderMap);
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));
        }

        void GfxManager::RenderUI_Main(RENDER_TARGETS input)
        {
            PrepForUI();

            //set directx viewport
            D3D11_VIEWPORT vpData = viewportManager->GetViewport(m_GameViewport).GetViewportData();
            unsigned w, h;
            gfxInfo->GetDimensions(w, h);
            vpData.TopLeftX = 0;
            vpData.TopLeftY = 0;
            vpData.Width = static_cast<FLOAT>(w);
            vpData.Height = static_cast<FLOAT>(h);

            dxCore->GetDeviceContext()->RSSetViewports(1, &vpData);

            dxCore->GetDeviceContext()->PSSetShaderResources(0, 1, &dxCore->GetRenderTargetMgr()->GetRenderTarget(input)->ShaderMap);
            shaderManager->Render(modelManager->GetModelVertcountByID(modelManager->GetModelIDByName("internalQuad")));
        }
    }
}
