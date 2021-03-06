/* ----------------------------------------------------------------------------
** Team Bear King
** ?2015 DigiPen Institute of Technology, All Rights Reserved.
**
** ParticlePathAnimatorComponent.cpp
**
** Author:
** - Matt Yan - m.yan@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/

#include "UrsinePrecompiled.h"
#include "ParticlePathAnimatorComponent.h"
#include "EntityEvent.h"
#include "Curves.h"
#include "ParticleSystemComponent.h"

namespace ursine
{
    namespace ecs
    {
        NATIVE_COMPONENT_DEFINITION(ParticlePathAnimator);

        ParticlePathAnimator::ParticlePathAnimator(void)
            : BaseComponent( )
            , m_particleComponent( nullptr )
            , m_loopPath( false )
            , m_renderCurve( false )
            , m_stepCount( 100 )
            , m_strength( 10.0f )
        {
            m_vectorArray.resize( m_stepCount );
        }

        ParticlePathAnimator::~ParticlePathAnimator(void)
        {
            GetOwner()->Listener(this).Off(
                ENTITY_PARTICLE_UPDATE, 
                &ParticlePathAnimator::onParticleUpdate
            );
        }

        void ParticlePathAnimator::OnInitialize(void)
        {
            Component::OnInitialize();

            m_particleComponent = GetOwner()->GetComponent<ParticleSystem>();

            GetOwner()->Listener(this).On(
                ENTITY_PARTICLE_UPDATE, 
                &ParticlePathAnimator::onParticleUpdate,
                1
            );
        }

        bool ParticlePathAnimator::GetIsLooping() const
        {
            return m_loopPath;
        }

        void ParticlePathAnimator::SetIsLooping(bool isLooping)
        {
            m_loopPath = isLooping;

            auto *children = GetOwner()->GetChildren();
            if (children->size() <= 3)
                return;
            updatePointArray( );
            updateVectorArray( );
        }

        bool ParticlePathAnimator::GetDebugRender() const
        {
            return m_renderCurve;
        }

        void ParticlePathAnimator::SetDebugRender(bool useDebug)
        {
            m_renderCurve = useDebug;
        }

        unsigned ParticlePathAnimator::GetStepCount() const
        {
            return m_stepCount;
        }

        void ParticlePathAnimator::SetStepCount(unsigned stepCount)
        {
            m_stepCount = stepCount;
        }

        float ParticlePathAnimator::GetStrength() const
        {
            return m_strength;
        }

        void ParticlePathAnimator::SetStrength(float strength)
        {
            m_strength = strength;
        }

        void ParticlePathAnimator::onParticleUpdate(EVENT_HANDLER(Entity))
        {
            auto *children = GetOwner()->GetChildren();
            auto *particleSystem = GetOwner()->GetComponent<ParticleSystem>();
            SVec3 start = GetOwner()->GetTransform()->GetWorldPosition();

            if (children->size() <= 3)
                return;

            if(checkForPointUpdate( ))
            {
                updatePointArray( );
                updateVectorArray( );
            }

            // UPDATE THE PARTICLES
            m_particleComponent = GetOwner()->GetComponent<ParticleSystem>();
            float dt = Application::Instance->GetDeltaTime();

            // get all the particles
            auto &gpuData = m_particleComponent->GetGPUParticleData();
            auto &cpuData = m_particleComponent->GetCPUParticleData();

            int particleCount = m_particleComponent->GetActiveParticleCount();
            
            // pre-calculate time scalar
            float timeScalar = (dt / 0.01666f) * 1.7f * m_stepCount / 100.0f;

            for (int x = 0; x < particleCount; ++x)
            {
                float lifeScale = 1.0f - (cpuData[ x ].lifeTime / cpuData[ x ].totalLifetime);
                unsigned index = static_cast<unsigned>(lifeScale * m_stepCount);

                // calculate where we are on the current interval
                float scalar = static_cast<float>(index + 1) - lifeScale;
                Vec3 finalVec = m_vectorArray[ index ];
                float finalScalar = timeScalar * (1.0f / cpuData[x].totalLifetime);

                gpuData[ x ].position[ 0 ] += finalVec.X() * finalScalar;
                gpuData[ x ].position[ 1 ] += finalVec.Y() * finalScalar;
                gpuData[ x ].position[ 2 ] += finalVec.Z() * finalScalar;

                if(particleSystem->GetVelocityOrient( ))
                {
                    gpuData[ x ].rotation[ 0 ] = finalVec.X( );
                    gpuData[ x ].rotation[ 1 ] = finalVec.Y( );
                    gpuData[ x ].rotation[ 2 ] = finalVec.Z( );
                }

                if (m_renderCurve)
                {
                    GetCoreSystem(graphics::GfxAPI)->DrawingMgr.SetColor(Color::Red);

                    if (particleSystem->GetSystemSpace() == SystemSpace::WorldSpace)
                    {
                        GetCoreSystem(graphics::GfxAPI)->DrawingMgr.DrawLine(SVec3(gpuData[ x ].position[0], gpuData[ x ].position[1], gpuData[ x ].position[2]), SVec3(gpuData[ x ].position[ 0 ], gpuData[ x ].position[ 1 ], gpuData[ x ].position[ 2 ]) + finalVec * 0.5);
                    }
                    else
                    {
                        GetCoreSystem(graphics::GfxAPI)->DrawingMgr.DrawLine(start + SVec3(gpuData[ x ].position[ 0 ], gpuData[ x ].position[ 1 ], gpuData[ x ].position[ 2 ]), start + SVec3(gpuData[ x ].position[ 0 ], gpuData[ x ].position[ 1 ], gpuData[ x ].position[ 2 ]) + finalVec * 0.5);
                    }
                }
            }

            if(m_renderCurve)
            {
                
                for(unsigned x = 0; x < m_stepCount; ++x)
                {
                    GetCoreSystem(graphics::GfxAPI)->DrawingMgr.SetColor(Color::Green);
                    GetCoreSystem(graphics::GfxAPI)->DrawingMgr.DrawLine(start, start + m_vectorArray[x]);
                    start += m_vectorArray[x];
                }

                GetCoreSystem(graphics::GfxAPI)->DrawingMgr.SetColor(Color::LightGreen);
                GetCoreSystem(graphics::GfxAPI)->DrawingMgr.SetSize(20);

                if (particleSystem->GetSystemSpace() == SystemSpace::WorldSpace)
                {
                    for(auto &point : m_pointArray)
                    {
                        GetCoreSystem(graphics::GfxAPI)->DrawingMgr.DrawPoint(point);
                    }
                }
                else
                {
                    for (auto &point : m_pointArray)
                    {
                        GetCoreSystem(graphics::GfxAPI)->DrawingMgr.DrawPoint(point + start);
                    }
                }
            }
        }

        bool ParticlePathAnimator::checkForPointUpdate(void)
        {
            auto *children = GetOwner()->GetChildren();
            auto childSize = children->size() + 1;
            auto ourSize = static_cast<size_t>(m_pointArray.size());

            bool isEqual = childSize == ourSize;

            if (childSize != ourSize)
                return true;

            unsigned index = 0;

            for(auto &child : *children)
            {
                auto childEntity = GetOwner( )->GetWorld( )->GetEntity( child );
                auto position = childEntity->GetTransform( )->GetWorldPosition( );
                
                if(position != m_pointArray[ index++ + 1])
                {
                    return true;
                }
            }

            return false;
        }

        void ParticlePathAnimator::updatePointArray(void)
        {
            auto *children = GetOwner( )->GetChildren( );
            auto *particleSystem = GetOwner( )->GetComponent<ParticleSystem>( );
            unsigned size = static_cast<unsigned>( children->size( ) );
            m_pointArray.resize(size + 1);

           if(particleSystem->GetSystemSpace( ) == SystemSpace::WorldSpace)
            {
                m_pointArray[0] = GetOwner( )->GetTransform( )->GetWorldPosition( );
                
                for(unsigned x = 0; x < size; ++x)
                {
                    auto childEntity = GetOwner( )->GetWorld( )->GetEntity( (*children)[ x ] );
                    if(childEntity->GetName( ) == "__bb")
                    {
                        m_pointArray.pop_back( );
                        --size;
                        continue;
                    }

                    m_pointArray[ x + 1] = childEntity->GetTransform( )->GetWorldPosition( );
                }
            }
           else
           {
               m_pointArray[ 0 ] = Vec3(0, 0, 0);

               for (unsigned x = 0; x < size; ++x)
               {
                   auto childEntity = GetOwner()->GetWorld()->GetEntity((*children)[ x ]);

                   if (childEntity->GetName() == "__bb")
                   {
                       m_pointArray.pop_back();
                       --size;
                       continue;
                   }


                   m_pointArray[ x + 1 ] = childEntity->GetTransform()->GetLocalPosition();

                   //m_pointArray[ x + 1 ] = GetOwner( )->GetTransform( )->GetWorldRotation( ).Rotate(m_pointArray[ x + 1 ]);
               }
           }
        }

        void ParticlePathAnimator::updateVectorArray(void)
        {
            int pointCount = static_cast<int>(m_pointArray.size());

            // calculate number of steps per curve
            int curveStepCount = m_stepCount / pointCount;

            // calculate the time step from 0 to step count
            float stepSize = 1.0f / curveStepCount;

            // calculate initial values
            float currentTime = 0;

            // for each segment
            for(int segment = 0; segment < pointCount; ++segment)
            {
                SVec3 lastPoint = m_pointArray[ segment ];
                float lastTime = 0;

                // calculate all curve steps for this segment
                for(int step = 0; step < curveStepCount; ++step)
                {
                    // calculate new time
                    float newTime = lastTime + stepSize;

                    Vec3 newPoint = getPoint(segment, newTime);

                    m_vectorArray[segment * curveStepCount + step] = newPoint - lastPoint;

                    // update last point
                    lastPoint = newPoint;
                    lastTime = newTime;
                }
            }
        }

        SVec3 ParticlePathAnimator::getPoint(unsigned index, float t)
        {
            if(!m_loopPath)
            {
                // very first, dupicate 0
                if(index == 0)
                {
                    return Curves::CatmullRomSpline(
                        m_pointArray[ 0 ],
                        m_pointArray[ 0 ],
                        m_pointArray[ 1 ],
                        m_pointArray[ 2 ],
                        t
                    );
                }
                // last node
                else if(index == m_pointArray.size( ) - 1)
                {
                    return Curves::CatmullRomSpline(
                        m_pointArray[ index - 1 ],
                        m_pointArray[ index ],
                        m_pointArray[ index ],
                        m_pointArray[ index ],
                        t
                    );
                }
                // second to last
                else if(index >= m_pointArray.size( ) - 2)
                {
                    return Curves::CatmullRomSpline(
                        m_pointArray[ index - 1 ],
                        m_pointArray[ index ],
                        m_pointArray[ index + 1 ],
                        m_pointArray[ index + 1 ],
                        t
                    );
                }
            
                // normal generation
                return Curves::CatmullRomSpline(
                    m_pointArray[index - 1], 
                    m_pointArray[index], 
                    m_pointArray[index + 1], 
                    m_pointArray[index + 2], 
                    t
                );
            }
            else
            {
                unsigned size = static_cast<unsigned>( m_pointArray.size( ) );

                // normal generation
                return Curves::CatmullRomSpline(
                    m_pointArray[ (size + index - 1) % size ],
                    m_pointArray[ (size + index) % size ],
                    m_pointArray[ (size + index + 1) % size ],
                    m_pointArray[ (size + index + 2) % size ],
                    t
                );
            }
        }
    }
}