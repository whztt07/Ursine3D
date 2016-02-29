/* ----------------------------------------------------------------------------
** Team Bear King
** � 2016 DigiPen Institute of Technology, All Rights Reserved.
**
** SpotLightProcessor.cpp
**
** Author:
** - Matt Yan - m.yan@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/
#include "UrsinePrecompiled.h"

#include "SpotLightProcessor.h"
#include "Renderable.h"
#include "GfxManager.h"

namespace ursine
{
    namespace graphics
    {
        SpotLightProcessor::SpotLightProcessor(void)
        {
            m_renderableType = RENDERABLE_LIGHT;
        }

        bool SpotLightProcessor::cullOperation(_DRAWHND handle, Camera &currentCamera)
        {
            UAssert(handle.Type_ == m_renderableType, "GfxEntityProcessor attempted to proces invalid type!");

            Light light = m_manager->renderableManager->GetRenderableByID<Light>( handle.Index_ );

            // if wrong type
            if (light.GetType( ) != Light::LIGHT_SPOTLIGHT)
                return true;

            // if inactive
            if ( !light.GetActive() )
                return true;

            // if culed by camera mask
            if ( currentCamera.CheckMask(light.GetRenderMask()) )
                return true;

            // return false as in DO NOT CULL ME
            return false;
        }

        void SpotLightProcessor::prepOperation(_DRAWHND handle, SMat4 &view, SMat4 &proj, Camera &currentCamera)
        {
            Light spotLight = m_manager->renderableManager->GetRenderableByID<Light>(handle.Index_);

            // pre-calculate some stuff
            SMat4 transposeView = currentCamera.GetViewMatrix( );
            transposeView.Transpose();
            SVec3 lightDirection = transposeView.TransformVector( spotLight.GetDirection( ) );
            SVec3 lightPosition = transposeView.TransformPoint( spotLight.GetPosition( ) );

            // MAP SPOTLIGHT DATA ///////////////////////////////////
            SpotlightBuffer slb;
            slb.diffuseColor = spotLight.GetColor( ).ToVector3( ).ToD3D( );
            slb.lightDirection = lightDirection.ToD3D( );
            slb.lightPosition = lightPosition.ToD3D( );
            slb.intensity = spotLight.GetIntensity( );
            //needs to be in radians
            slb.innerAngle = cosf( math::DegreesToRadians(spotLight.GetSpotlightAngles( ).X( ) / 2.f)); 
            slb.outerAngle = cosf( math::DegreesToRadians(spotLight.GetSpotlightAngles( ).Y( ) / 2.f));
            slb.lightSize = spotLight.GetRadius( );

            m_manager->bufferManager->MapBuffer<BUFFER_SPOTLIGHT>(
                &slb, 
                SHADERTYPE_PIXEL
            );

            // MAP TRANSFORM DATA ///////////////////////////////////
            m_manager->bufferManager->MapTransformBuffer(
                spotLight.GetSpotlightTransform() * SMat4(
                    SVec3(0, 0, 0.5f), 
                    SQuat(-90.0f, 0.0f, 0.0f), 
                    SVec3::One()
                )
            );

            // DETERMINE CULLING MODE ///////////////////////////////
            SVec3 light2Cam = currentCamera.GetPosition( ) - lightPosition;
            light2Cam.Normalize( );
            float dotp = light2Cam.Dot( lightDirection );

            // if camera is outside the cone, use this
            if (dotp < cosf(slb.outerAngle))
                m_manager->dxCore->SetRasterState( RASTER_STATE_SOLID_BACKCULL );
            // else, use frontface culling
            else
                m_manager->dxCore->SetRasterState( RASTER_STATE_SOLID_FRONTCULL );
        }

        void SpotLightProcessor::renderOperation(_DRAWHND handle, Camera &currentCamera)
        {
            Light light = m_manager->renderableManager->GetRenderableByID<Light>(handle.Index_);
            //m_manager->shaderManager->Render(
            //    m_manager->modelManager->GetModelVertcountByID(
            //        m_manager->modelManager->GetModelIDByName("lightCone")
            //    )
            //);
        }
    }
}