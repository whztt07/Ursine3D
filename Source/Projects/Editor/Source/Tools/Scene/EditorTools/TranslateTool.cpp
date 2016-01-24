#include "Precompiled.h"

#include "TranslateTool.h"

#include <EditorConfig.h>
#include <SystemManager.h>
#include <CameraComponent.h>
#include <Model3DComponent.h>

using namespace ursine;
using namespace ecs;

TranslateTool::TranslateTool(Editor* editor)
	: EditorTool( editor )
	, m_gizmo( nullptr )
	, m_selected( -1 )
	, m_dragging( false )
	, m_snapping( false )
	, m_local( false )
{
	m_graphics = GetCoreSystem( graphics::GfxAPI );
	m_world = m_editor->GetProject( )->GetScene( )->GetWorld( );
	m_editorCameraSystem = m_world->GetEntitySystem( EditorCameraSystem );
}

void TranslateTool::OnEnable(EntityUniqueID selected)
{
	m_selected = selected;

	if (m_selected != -1)
		enableAxis( );
}

void TranslateTool::OnDisable(void)
{
	m_selected = -1;
	m_dragging = false;
	m_snapping = false;

	disableAxis( );
}

void TranslateTool::OnSelect(Entity* entity)
{
	m_selected = entity->GetUniqueID( );
	enableAxis( );
}

void TranslateTool::OnDeselect(Entity* entity)
{
	m_selected = -1;

	m_deleteGizmo = true;
}

void TranslateTool::OnMouseDown(const MouseButtonArgs& args)
{
	// get the current entity ID the mouse is over
	auto newID = m_graphics->GetMousedOverID( );
	auto entity = m_world->GetEntityUnique( newID );

	if (!entity || m_altDown)
		return;

	auto entityTrans = entity->GetTransform( );

	auto rootName = entityTrans->GetRoot( )->GetOwner( )->GetName( );

	// if we're clicking on ourselves, set the dragging flag,
	// and the vector we're dragging on
	if (rootName == "TranslationGizmo")
	{
		m_dragging = true;

		// Get the selected entity
		auto selected = m_world->GetEntityUnique( m_selected );

		// Get the gizmo's name (the models are under the parent named the axis' name)
		auto name = entityTrans->GetParent( )->GetOwner( )->GetName( );

		if (name == "xAxis")
			setDirectionVectors( SVec3::UnitX( ), selected );
		else if (name == "yAxis")
			setDirectionVectors( SVec3::UnitY( ), selected );
		else if (name == "zAxis")
			setDirectionVectors( SVec3::UnitZ( ), selected );
		else
		{
			name = entity->GetName( );

			if (name == "zxPlane")
				setDirectionVectors( SVec3( 1.0f, 0.0f, 1.0f ), selected );
			else if (name == "yzPlane")
				setDirectionVectors( SVec3( 0.0f, 1.0f, 1.0f ), selected );
			else if (name == "xyPlane")
				setDirectionVectors( SVec3( 1.0f, 1.0f, 0.0f ), selected );
		}
	}
}

void TranslateTool::OnMouseUp(const MouseButtonArgs& args)
{
	if (m_dragging)
		m_dragging = false;
}

void TranslateTool::OnMouseMove(const MouseMoveArgs &args)
{
	if (m_dragging && m_selected != -1)
	{
		// Project the delta vector onto the screen direction vector
		auto b = args.positionDelta;
		auto &a = m_screenDir;

		// invert the Y
		b.Y( ) = -b.Y( );

		float dot = a.Dot( b );
		auto proj = ( dot / b.LengthSquared( ) ) * b;
		auto dist = proj.Length( );
		auto selected = m_world->GetEntityUnique( m_selected )->GetTransform( );
		auto gizmo = m_gizmo->GetTransform( );

		if (dot < 0.0f)
			dist = -dist;
		
		// Multiply by an arbitrary value so that it feels nice.
		// Ideally we would convert the screen space to a world space distance
		// and then apply it to the world space direction vector.
		dist *= 0.5f;

		auto newP = gizmo->GetWorldPosition( ) + m_worldDir * dist;

		gizmo->SetWorldPosition( newP );

		if (m_snapping)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (newP[ i ] > 0.0f)
					newP[ i ] = float(int(newP[ i ] * 2.0f + 0.5f)) / 2.0f;
				else
					newP[ i ] = float(int(newP[ i ] * 2.0f - 0.5f)) / 2.0f;
			}

			selected->SetWorldPosition( newP );
		}
		else
			selected->SetWorldPosition( newP );
	}
}

void TranslateTool::OnKeyDown(const KeyboardKeyArgs &args)
{
	if (args.key == KEY_SPACE)
		m_local = !m_local;

	if (args.key == KEY_LCONTROL)
		m_snapping = true;
}

void TranslateTool::OnKeyUp(const KeyboardKeyArgs &args)
{
	if (args.key == KEY_CONTROL)
		m_snapping = false;
}

void TranslateTool::OnUpdate(KeyboardManager *kManager, MouseManager *mManager)
{
	if (m_deleteGizmo)
	{
		disableAxis( );
		m_deleteGizmo = false;
	}

	if (kManager->IsDown( KEY_LMENU ))
		m_altDown = true;
	else
		m_altDown = false;

	updateAxis( );
}

void TranslateTool::setDirectionVectors(const SVec3& basisVector, Entity *selected)
{
	if (m_local)
		m_worldDir = selected->GetTransform( )->GetWorldRotation( ) * basisVector;
	else
		m_worldDir = basisVector;

	// Set the screen direction
	auto cam = m_editorCameraSystem->GetEditorCamera( );
	auto worldP = selected->GetTransform( )->GetWorldPosition( );
	auto p1 = cam->WorldToScreen( worldP );
	auto p2 = cam->WorldToScreen( worldP + m_worldDir );
	
	m_screenDir = p2 - p1;
	m_screenDir.Normalize( );
}

void TranslateTool::enableAxis(void)
{
	m_gizmo = m_world->CreateEntityFromArchetype( 
		EDITOR_ARCHETYPE_PATH "EditorTools/TranslationGizmo.uatype",
		"TranslationGizmo" 
	);

	setEntitySerializationToggle( false, m_gizmo );
	
	// Make it invisible in the hiearchy
	m_gizmo->SetVisibleInEditor( false );

	// Make all models draw over everything
	for (auto &model : m_gizmo->GetComponentsInChildren<ecs::Model3D>( ))
	{
		model->SetOverdraw( true );
		model->SetMaterialData( 4, 0, 0 );
	}
}

void TranslateTool::disableAxis(void)
{
	if (m_gizmo)
	{
		m_gizmo->Delete( );
		
		// Clear the deletion queue if the scene is paused
		EditorClearDeletionQueue( );
	}

	m_gizmo = nullptr;
}

void TranslateTool::updateAxis(void)
{
	if (m_selected == -1 || m_gizmo == nullptr)
		return;

	// update the size of the gizmo
	auto gizTrans = m_gizmo->GetTransform( );

	gizTrans->SetWorldScale( SVec3( m_editorCameraSystem->GetCamZoom( ) * 0.03f ) );

	// Update the position of things
	auto selected = m_world->GetEntityUnique( m_selected );
	auto selTrans = selected->GetTransform( );

	gizTrans->SetWorldPosition( selTrans->GetWorldPosition( ) );

	if (m_local)
		gizTrans->SetWorldRotation( selTrans->GetWorldRotation( ) );
	else
		gizTrans->SetLocalRotation( SQuat::Identity( ) );
}

void TranslateTool::setEntitySerializationToggle(bool toggle, Entity *entity)
{
	for (auto *child : entity->GetTransform( )->GetChildren( ))
	{
		setEntitySerializationToggle( toggle, child->GetOwner( ) );
	}

	entity->EnableSerialization( toggle );
}
