#include "UrsinePrecompiled.h"

#include "LightComponent.h"

#include "GfxAPI.h"
#include <Game Engine/Scene/Entity/EntityEvent.h>

namespace ursine
{
    namespace ecs
    {
        NATIVE_COMPONENT_DEFINITION( Light );

        Light::Light(void)
            : BaseComponent( )
            , m_light( nullptr )
        {
            auto *graphics = GetCoreSystem( graphics::GfxAPI );

            m_handle = graphics->
                RenderableMgr.AddRenderable( graphics::RENDERABLE_LIGHT );

            m_light = &graphics->
                RenderableMgr.GetLight( m_handle );

            m_light->Initialize( );
        }

        Light::~Light(void)
        {
            RenderableComponentBase::OnRemove( GetOwner( ) );
            
            m_light = nullptr;

            GetCoreSystem( graphics::GfxAPI )
                ->RenderableMgr.DestroyRenderable( m_handle );
        }

        void Light::OnInitialize(void)
        {
            RenderableComponentBase::OnInitialize( GetOwner( ) );

            updateRenderer( );
        }

        graphics::GfxHND Light::GetHandle(void) const
        {
            return m_handle;
        }

        const graphics::Light *Light::GetLight(void)
        {
            return m_light;
        }

        LightType Light::GetType(void)
        {
            return static_cast<LightType>( m_light->GetType( ) );
        }

        void Light::SetType(LightType type)
        {
            m_light->SetType( static_cast<graphics::Light::LightType>( type ) );

            NOTIFY_COMPONENT_CHANGED( "Type", type );
        }

        const SVec3 &Light::GetDirection(void)
        {
            return m_light->GetDirection( );
        }

        void Light::SetDirection(const SVec3 &direction)
        {
            m_light->SetDirection( direction );

            NOTIFY_COMPONENT_CHANGED( "Direction", direction );
        }

        const SVec3 &Light::GetPosition(void)
        {
            return m_light->GetPosition( );
        }

        void Light::SetPosition(const SVec3 &position)
        {
            m_light->SetPosition( position );

            NOTIFY_COMPONENT_CHANGED( "Position", position );
        }

        const Color &Light::GetColor(void)
        {
            return m_light->GetColor( );
        }

        void Light::SetColor(const Color &color)
        {
            m_light->SetColor( color );

            NOTIFY_COMPONENT_CHANGED( "Color", color );
        }

        float Light::GetRadius(void)
        {
            return m_light->GetRadius( );
        }

        void Light::SetRadius(const float radius)
        {
            m_light->SetRadius( radius );

            NOTIFY_COMPONENT_CHANGED( "Radius", radius );
        }

        float Light::GetIntensity(void)
        {
            return m_light->GetIntensity( );
        }

        void Light::SetIntensity(const float intensity)
        {
            m_light->SetIntensity( intensity );

            NOTIFY_COMPONENT_CHANGED( "Intensity", intensity );
        }

        const Vec2 &Light::GetSpotlightAngles(void)
        {
            return m_light->GetSpotlightAngles( );
        }

        void Light::SetSpotlightAngles(const Vec2 &angles)
        {
            m_light->SetSpotlightAngles( angles );

            NOTIFY_COMPONENT_CHANGED( "SpotLightAngles", angles );
        }

        void Light::updateRenderer(void)
        {
            auto trans = GetOwner()->GetTransform();
            auto &light = GetCoreSystem(graphics::GfxAPI)->RenderableMgr.GetLight(m_handle);

            //set position using the transform
            light.SetPosition(trans->GetWorldPosition());

            //get inner/outer angles
            float outer = light.GetSpotlightAngles().Y() * 3.1415f / 180.f;

            SVec3 scale = trans->GetWorldScale();
            scale.SetX(tanf(outer / 2.f) * scale.Y() * 2.2f); //arbitrary scalar to prevent artifacts from exact sizing
            scale.SetZ(tanf(outer / 2.f) * scale.Y() * 2.2f);
            trans->SetWorldScale(scale);

            //update transform for the spotlight
            light.SetSpotlightTransform(trans->GetLocalToWorldMatrix());

            //set our direction using our rotation 
            SVec3 lightDir = SVec3(0, -1, 0);
            lightDir = trans->GetWorldRotation() * lightDir;

            light.SetDirection(lightDir);

            light.SetSpotlightTransform(trans->GetLocalToWorldMatrix());
        }
    }
}
