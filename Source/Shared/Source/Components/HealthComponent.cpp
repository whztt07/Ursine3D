#include "Precompiled.h"

#include "HealthComponent.h"

NATIVE_COMPONENT_DEFINITION( Health );

Health::Health(void)
    : BaseComponent( )
    , m_health( 100 )
{
    
}

Health::~Health(void)
{
    
}

float Health::GetHealth(void) const
{
    return m_health;
}

void Health::SetHealth(const float health)
{
    m_health = health;
}

void Health::DealDamage(const float damage)
{
    m_health -= damage;

    if(m_health <= 0)
    {
        GetOwner( )->Delete( );
    }
}