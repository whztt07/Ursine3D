#include "UrsinePrecompiled.h"

#include "Hierarchy.h"
#include "EntityEvent.h"
#include "Entity.h"
#include "HierarchyEvents.h"

namespace ursine
{
    namespace ecs
    {
        const std::vector<EntityID> *Hierarchy::GetChildren(const Entity* entity) const
        {
            return m_nodes[ entity->GetID( ) ].Children( );
        }
    
        EntityID Hierarchy::GetParent(const Entity* entity) const
        {
            return m_nodes[ entity->GetID( ) ].Parent();
        }
    
        EntityID Hierarchy::GetRoot(const Entity* entity) const
        {
            return m_nodes[ entity->GetID( ) ].Root();
        }
    
        void Hierarchy::AddEntity(Entity* entity)
        {
            auto id = entity->GetID( );
      
            // grow the array's size if needed
            if (id > m_nodes.size( ))
                m_nodes.emplace_back( entity->GetID( ) );
      
            // Add it to the root
            m_root.AddChild( id );
      
            // subscribe to this entities events
            entity->Listener( this )
                .On( ENTITY_PARENT_CHANGED, &Hierarchy::ParentChanged );
        }
    
        void Hierarchy::RemoveEntity(Entity* entity)
        {
            auto id = entity->GetID( );
            auto &entityNode = m_nodes[ id ];
            
            // Remove this entity from the parent
            auto parentID = entityNode.Parent( );
      
            if (parentID == -1)
                m_root.RemoveChild( id );
            else
                m_nodes[ parentID ].RemoveChild( id );
      
            // Deactivate this entity
            entityNode.Deactivate( );
      
            // unsubscribe to this entities events
            entity->Listener( this )
                .Off( ENTITY_PARENT_CHANGED, &Hierarchy::ParentChanged );
        }
    
        void Hierarchy::ParentChanged(EVENT_HANDLER( Entity ))
        {
            EVENT_ATTRS( Entity, ParentChangedArgs );
    
            auto entityID = sender->GetID( );
            auto &entityNode = m_nodes[ entityID ];
            
            // Save the old parent
            auto oldParent = entityNode.Parent( );
            auto newParent = args->newParent;
      
            if (newParent == oldParent)
                return;
      
            // assign the new parent
            entityNode.SetParent( newParent );
      
            // Add to the root node if it has no parent
            if (newParent == -1)
            {
                m_root.AddChild( entityID );
                entityNode.SetRoot( entityID );
            }
            // Add to the new parent node
            else
            {
                auto &parentNode = m_nodes[ newParent ];
                parentNode.AddChild( entityID );
                entityNode.SetRoot( parentNode.Root( ));
            }
      
            // Remove it from the old parent (root or other entity)
            if (oldParent == -1)
                m_root.RemoveChild( entityID );
            else
                m_nodes[ oldParent ].RemoveChild( entityID );
      }
    }
}