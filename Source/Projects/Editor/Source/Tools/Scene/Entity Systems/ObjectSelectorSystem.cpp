#include "Precompiled.h"

#include "ObjectSelectorSystem.h"
#include <Game Engine/Scene/Component/Native Components/RenderableComponent.h>
#include <Game Engine/Scene/Component/Native Components/Model3DComponent.h>
#include <Tools/Scene/Components/SelectedComponent.h>
#include <Vec2.h>
#include <Vec3.h>
#include "EditorCameraSystem.h"
#include <SystemManager.h>
#include <Core/Graphics/API/GfxAPI.h>

using namespace ursine;

ENTITY_SYSTEM_DEFINITION(ObjectSelectorSystem);

ObjectSelectorSystem::ObjectSelectorSystem(ursine::ecs::World* world)
    : EntitySystem(world)
    , m_xAxis(nullptr)
    , m_yAxis(nullptr)
    , m_zAxis(nullptr)
    , m_currentID(-1)
    , m_graphics(nullptr)
    , m_dragging(false)
    , m_axis(-1)
    , m_currentTool(TOOL_TRANSLATION)
    , m_baseTranslation(Vec3(0, 0, 0))
    , m_offset(0)
    , m_baseScale(Vec3(1, 1, 1))
    , m_baseMousePos(Vec3(0, 0, 0))
{
    
}

ursine::ecs::Entity* ObjectSelectorSystem::GetCurrentFocus()
{
    return m_world->GetEntityUnique(m_currentID);
}

void ObjectSelectorSystem::OnInitialize()
{
    //connect to the mouse events
    GetCoreSystem(ursine::MouseManager)->Listener(this)
        .On(MM_BUTTON_DOWN, &ObjectSelectorSystem::onMouseDown)
        .On(MM_MOVE, &ObjectSelectorSystem::onMouseMove)
        .On(MM_BUTTON_UP, &ObjectSelectorSystem::onMouseUp)
        .On(MM_SCROLL, &ObjectSelectorSystem::onMouseScroll);

    GetCoreSystem(ursine::KeyboardManager)->Listener(this)
        .On(KM_KEY_DOWN, &ObjectSelectorSystem::onKeyDown);

    m_world->Listener(this)
        .On(ursine::ecs::WorldEventType::WORLD_UPDATE, &ObjectSelectorSystem::onUpdate);

    //grab graphics
    m_graphics = GetCoreSystem(ursine::graphics::GfxAPI);

    //construct the 3 axis
    m_xAxis = m_world->CreateEntity();
    m_yAxis = m_world->CreateEntity();
    m_zAxis = m_world->CreateEntity();

    //get their transforms, set data
    auto xTransf = m_xAxis->GetComponent<ursine::ecs::Transform>();
    auto yTransf = m_yAxis->GetComponent<ursine::ecs::Transform>();
    auto zTransf = m_zAxis->GetComponent<ursine::ecs::Transform>();
    {
        //translation
        xTransf->SetWorldPosition(ursine::SVec3(2000, 0, 0));
        yTransf->SetWorldPosition(ursine::SVec3(0, 2000, 0));
        zTransf->SetWorldPosition(ursine::SVec3(0, 0, 2000));

        //rotation
        xTransf->SetWorldRotation(ursine::SQuat(90, ursine::SVec3(0, 0, 1)));
        yTransf->SetWorldRotation(ursine::SQuat(0, ursine::SVec3(0, 0, 1)));
        zTransf->SetWorldRotation(ursine::SQuat(90, ursine::SVec3(1, 0, 0)));

        xTransf->SetWorldScale(SVec3(0.1f, 1.1f, 0.1f)); 
        yTransf->SetWorldScale(SVec3(0.1f, 1.1f, 0.1f));
        zTransf->SetWorldScale(SVec3(0.1f, 1.1f, 0.1f));
    }

    //give them all renderables and models
    m_xAxis->AddComponent<ursine::ecs::Renderable>();
    m_yAxis->AddComponent<ursine::ecs::Renderable>();
    m_zAxis->AddComponent<ursine::ecs::Renderable>();

    m_xAxis->AddComponent<ursine::ecs::Model3D>();
    m_yAxis->AddComponent<ursine::ecs::Model3D>();
    m_zAxis->AddComponent<ursine::ecs::Model3D>();

    //get their models
    auto xHand = m_xAxis->GetComponent<ursine::ecs::Renderable>()->GetHandle();
    auto yHand = m_yAxis->GetComponent<ursine::ecs::Renderable>()->GetHandle();
    auto zHand = m_zAxis->GetComponent<ursine::ecs::Renderable>()->GetHandle();

    auto &xModel = m_graphics->RenderableMgr.GetModel3D(xHand);
    auto &yModel = m_graphics->RenderableMgr.GetModel3D(yHand);
    auto &zModel = m_graphics->RenderableMgr.GetModel3D(zHand);
    {

        xModel.SetModel("Cylinder");
        yModel.SetModel("Cylinder");
        zModel.SetModel("Cylinder");

        xModel.SetMaterial("Blank");
        yModel.SetMaterial("Blank");
        zModel.SetMaterial("Blank");

        xModel.SetColor( ursine::Color(1, 0, 0, 1));
        yModel.SetColor( ursine::Color(0, 1, 0, 1));
        zModel.SetColor( ursine::Color(0, 0, 1, 1));

        xModel.SetMaterialData(8, 0, 0);
        yModel.SetMaterialData(8, 0, 0);
        zModel.SetMaterialData(8, 0, 0);

        xModel.SetOverdraw(true);
        yModel.SetOverdraw(true);
        zModel.SetOverdraw(true);
    }
}

void ObjectSelectorSystem::OnRemove()
{
    GetCoreSystem(ursine::MouseManager)->Listener(this)
        .Off(MM_BUTTON_DOWN, &ObjectSelectorSystem::onMouseDown)
        .Off(MM_MOVE, &ObjectSelectorSystem::onMouseMove)
        .Off(MM_BUTTON_UP, &ObjectSelectorSystem::onMouseUp)
        .Off(MM_SCROLL, &ObjectSelectorSystem::onMouseScroll);

    GetCoreSystem(ursine::KeyboardManager)->Listener(this)
        .Off(KM_KEY_DOWN, &ObjectSelectorSystem::onKeyDown);

    m_zAxis->Delete( );
    m_xAxis->Delete( );
    m_yAxis->Delete( );
}

// EVENTS ///////////////////////////////////////////////////////////
void ObjectSelectorSystem::onMouseDown(EVENT_HANDLER(ursine::MouseManager))
{
    EVENT_ATTRS(MouseManager, MouseButtonArgs);

    if (args->button == MBTN_LEFT)
    {
        //update out bases
        updateBases( );

        //get the current ID from graphics
        auto newID = m_graphics->GetMousedOverID( );

        //make sure it's a valid id
        if (newID == -1)
        {
            //if we were pressing alt just ignore this event
            if ((GetCoreSystem(ursine::KeyboardManager)->GetModifiers( ) & KMD_ALT))
                return;

            hideTool( );
            unpickObject(m_currentID);
            m_currentID = -1;

            return;
        }

        //check to see if we are selecting any of our axis
        if (newID == m_xAxis->GetUniqueID( ))
        {
            m_dragging = true;
            m_axis = 0;

            m_baseMousePos = GetMousePosition(args->position);

            return;
        }
        else if (newID == m_yAxis->GetUniqueID( ))
        {
            m_dragging = true;
            m_axis = 1;

            m_baseMousePos = GetMousePosition(args->position);

            return;
        }
        else if (newID == m_zAxis->GetUniqueID( ))
        {
            m_dragging = true;
            m_axis = 2;

            m_baseMousePos = GetMousePosition(args->position);

            return;
        }

        //if we are currently dragging, don't try to pick other objects
        if (m_dragging)
            return;

        //check to see if it's different than the current one
        if (newID != m_currentID)
        {
            //unpick old object
            unpickObject(m_currentID);

            //pick new object
            pickObject(newID);

            //update id
            m_currentID = newID;

            //save position
            moveToolToEntity(newID);
        }
    }
    else if (args->button == MBTN_MIDDLE)
    {
        //update out bases
        updateBases( );
        m_baseMousePos = GetMousePosition(args->position);
        m_dragging = true;
    }
}

void ObjectSelectorSystem::onMouseMove(EVENT_HANDLER(ursine::MouseManager))
{
    EVENT_ATTRS(MouseManager, MouseMoveArgs);

    auto mouseMgr = GetCoreSystem(MouseManager);

    //some switch for detecting tool type
    if(!(GetCoreSystem(ursine::KeyboardManager)->GetModifiers( ) & KMD_ALT))
    {
        //get the editor camera
        graphics::Camera *cam = m_world->GetEntitySystem(EditorCameraSystem)->GetEditorCamera();

        //get the mouse position
        Vec2 screenPos = args->position;

        Vec3 camPos = cam->GetPosition();

        //get the mouse world positions
        Vec3 p1 = cam->ScreenToWorld(screenPos, 0.1f);
        Vec3 p2 = cam->ScreenToWorld(screenPos, 1.f);

        //create a vector going out from the eye
        Vec3 mouseVec = p1 - p2;
        mouseVec.Set(mouseVec.X(), mouseVec.Y(), mouseVec.Z());

        //project onto the CURRENT place, which is dependent on the base position
        //x axis, we will treat z as stationary
        //z value will solve to the current z of the position
        //need t value
        //project onto all 3 planes, calculate an x, y, and z

        float timeX = (m_baseTranslation.Z() - camPos.Z()) / mouseVec.Z();
        float timeY = (m_baseTranslation.Z() - camPos.Z()) / mouseVec.Z();
        float timeZ = (m_baseTranslation.X() - camPos.X()) / mouseVec.X();

        float x = camPos.X() + timeX * mouseVec.X();
        float y = camPos.Y() + timeY * mouseVec.Y();
        float z = camPos.Z() + timeZ * mouseVec.Z();

        //we need to take into account that the current point of the transform may not be 0, 0, 0
        if(m_dragging && m_currentID != -1)
        {
            switch(m_currentTool)
            {
            case TOOL_TRANSLATION:
                updateTranslation(SVec3(x, y, z));
                break;
            case TOOL_SCALE:
                updateScale(SVec3(x, y, z));
                break;
            case TOOL_ROTATION:
                break;
            default:
                break;
            }
        }
    }
}

void ObjectSelectorSystem::onMouseUp(EVENT_HANDLER(ursine::MouseManager))
{
    m_dragging = false;

    //save position
    if (m_currentID != -1)
    {
        auto newObj = m_world->GetEntityUnique(m_currentID);
    }
}

void ObjectSelectorSystem::onMouseScroll(EVENT_HANDLER(ursine::MouseManager))
{
    if (m_currentID != -1) moveToolToEntity(m_currentID);
}

void ObjectSelectorSystem::onKeyDown(EVENT_HANDLER(ursine::KeyboardManager))
{
    EVENT_ATTRS(KeyboardManager, KeyboardKeyArgs);

    if(args->key == KEY_1)
    {
        m_dragging = false;
        m_currentTool = TOOL_TRANSLATION;
        setToTranslate( );
    }
    else if (args->key == KEY_2)
    {
        m_dragging = false;
        m_currentTool = TOOL_SCALE;
        setToScale( );
    } 
    else if (args->key == KEY_3)
    { 
        m_dragging = false;
        m_currentTool = TOOL_ROTATION;
        setToRotation( );
    }
}

void ObjectSelectorSystem::onUpdate(EVENT_HANDLER(ursine::ecs::World))
{
    if (m_currentID == -1)
        return;

    float zoom = m_world->GetEntitySystem(EditorCameraSystem)->GetCamZoom( );
    auto obj = m_world->GetEntityUnique(m_currentID);

    Vec3 pos = obj->GetComponent<ecs::Transform>( )->GetWorldPosition( );

    //figure out some color stuff
    Color xAx = Color(1, 0, 0, 1);
    Color yAx = Color(0, 1, 0, 1);
    Color zAx = Color(0, 0, 1, 1);

    //change color to yellow if dragging
    if (m_dragging && !(GetCoreSystem(ursine::KeyboardManager)->GetModifiers( ) & KMD_ALT))
    {
        switch (m_axis)
        {
        case 0:
            xAx = Color(1, 1, 0, 1);
            break;
        case 1:
            yAx = Color(1, 1, 0, 1);
            break;
        case 2:
            zAx = Color(1, 1, 0, 1);
            break;
        default:
            break;
        }
    }

    //set color in models
    auto xHand = m_xAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );
    auto yHand = m_yAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );
    auto zHand = m_zAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );

    auto &xModel = m_graphics->RenderableMgr.GetModel3D(xHand);
    auto &yModel = m_graphics->RenderableMgr.GetModel3D(yHand);
    auto &zModel = m_graphics->RenderableMgr.GetModel3D(zHand);

    xModel.SetColor(xAx);
    yModel.SetColor(yAx);
    zModel.SetColor(zAx);

    //if (m_currentTool == TOOL_TRANSLATION)
    //{
    //    //float length = zoom / 10;

    //    //float arrWidth = (0.3 / 2) * length;
    //    //float arrLength = length - arrWidth;

    //    ////render the lines
    //    //auto gfx = GetCoreSystem(ursine::graphics::GfxAPI);

    //    //gfx->DrawingMgr.SetOverdraw(true);

    //    ///////////////////////////////////////////////////////////////
    //    //gfx->DrawingMgr.SetColor(xAx);
    //    ////pole
    //    //gfx->DrawingMgr.DrawLine(pos, pos + SVec3(length, 0, 0));
    //    //
    //    ////arrows
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(arrLength, arrWidth  , 0         ), pos + SVec3(length, 0, 0));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(arrLength, -arrWidth , 0         ), pos + SVec3(length, 0, 0));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(arrLength, 0         , arrWidth  ), pos + SVec3(length, 0, 0));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(arrLength, 0         , -arrWidth ), pos + SVec3(length, 0, 0));


    //    //gfx->DrawingMgr.SetColor(yAx);
    //    ////pole
    //    //gfx->DrawingMgr.DrawLine(pos, pos + SVec3(0, length, 0));

    //    ////arrows
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(arrWidth, arrLength, 0), pos + SVec3(0, length, 0));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(-arrWidth, arrLength, 0), pos + SVec3(0, length, 0));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(0, arrLength, arrWidth), pos + SVec3(0, length, 0));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(0, arrLength, -arrWidth), pos + SVec3(0, length, 0));

    //    //gfx->DrawingMgr.SetColor(zAx);
    //    ////pole
    //    //gfx->DrawingMgr.DrawLine(pos, pos + SVec3(0, 0, length));

    //    ////arrows
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(arrWidth, 0, arrLength ), pos + SVec3(0, 0, length));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(-arrWidth, 0, arrLength), pos + SVec3(0, 0, length));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(0, arrWidth, arrLength), pos + SVec3(0, 0, length));
    //    //gfx->DrawingMgr.DrawLine(pos + SVec3(0, -arrWidth, arrLength), pos + SVec3(0, 0, length));

    //    //gfx->DrawingMgr.SetOverdraw(false);
    //}
    //else if(m_currentTool == TOOL_SCALE)
    //{
    //    
    //}
}

// UTILITIES ////////////////////////////////////////////////////////

void ObjectSelectorSystem::calculateOffset(ursine::Vec2 mousePos)
{
    SVec3 mouse = GetMousePosition(mousePos);

    
}

void ObjectSelectorSystem::updateToolPosition(ursine::Vec3 pos)
{
    float zoom = m_world->GetEntitySystem(EditorCameraSystem)->GetCamZoom( );
    float scalar = 1;

    //get their transforms, set data
    auto xTransf = m_xAxis->GetComponent<ursine::ecs::Transform>();
    auto yTransf = m_yAxis->GetComponent<ursine::ecs::Transform>();
    auto zTransf = m_zAxis->GetComponent<ursine::ecs::Transform>();
    {
        switch(m_currentTool)
        {
        case TOOL_TRANSLATION:
            scalar = zoom / 80;
            xTransf->SetWorldScale(SVec3(scalar, zoom / 10.f, scalar));
            yTransf->SetWorldScale(SVec3(scalar, zoom / 10.f, scalar));
            zTransf->SetWorldScale(SVec3(scalar, zoom / 10.f, scalar));

            xTransf->SetWorldPosition(ursine::SVec3(zoom / 20, 0, 0) + pos);
            yTransf->SetWorldPosition(ursine::SVec3(0, zoom / 20, 0) + pos);
            zTransf->SetWorldPosition(ursine::SVec3(0, 0, zoom / 20) + pos);
            break;
        case TOOL_SCALE:
            scalar = zoom / 40;
            xTransf->SetWorldScale(SVec3(scalar, scalar, scalar));
            yTransf->SetWorldScale(SVec3(scalar, scalar, scalar));
            zTransf->SetWorldScale(SVec3(scalar, scalar, scalar));

            xTransf->SetWorldPosition(ursine::SVec3(scalar * 3, 0, 0) + pos);
            yTransf->SetWorldPosition(ursine::SVec3(0, scalar * 3, 0) + pos);
            zTransf->SetWorldPosition(ursine::SVec3(0, 0, scalar * 3) + pos);
            break;
        case TOOL_ROTATION:
            break;
        default:
            break;
        }
    }
}

void ObjectSelectorSystem::moveToolToEntity(const ursine::ecs::EntityUniqueID id)
{
    auto newObj = m_world->GetEntityUnique(id);
    updateToolPosition(newObj->GetComponent<ursine::ecs::Transform>( )->GetWorldPosition( ));
}

// PICKING //////////////////////////////////////////////////////////

void ObjectSelectorSystem::pickObject(const ursine::ecs::EntityUniqueID id)
{
    auto obj = m_world->GetEntityUnique(id);

    //if it existed and it was selected, unselect
    if (obj != nullptr && obj->GetComponent<Selected>( ) == nullptr)
        obj->AddComponent<Selected>( );

    m_baseTranslation = obj->GetComponent<ecs::Transform>()->GetWorldPosition();
    m_baseScale = obj->GetComponent<ecs::Transform>( )->GetWorldScale( );
}

void ObjectSelectorSystem::unpickObject(const ursine::ecs::EntityUniqueID id)
{
    auto obj = m_world->GetEntityUnique(id);

    //if it existed and it was selected, unselect
    if (obj != nullptr && obj->GetComponent<Selected>( ) != nullptr)
        obj->RemoveComponent<Selected>( );
}

// OBJECT TRANSFORMATION / TOOLS ////////////////////////////////////

void ObjectSelectorSystem::setToTranslate()
{
    //get their models
    auto xHand = m_xAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );
    auto yHand = m_yAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );
    auto zHand = m_zAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );

    auto &xModel = m_graphics->RenderableMgr.GetModel3D(xHand);
    auto &yModel = m_graphics->RenderableMgr.GetModel3D(yHand);
    auto &zModel = m_graphics->RenderableMgr.GetModel3D(zHand);

    {
        xModel.SetModel("Cylinder");
        yModel.SetModel("Cylinder");
        zModel.SetModel("Cylinder");

        xModel.SetMaterial("Blank");
        yModel.SetMaterial("Blank");
        zModel.SetMaterial("Blank");

        xModel.SetMaterialData(8, 0, 0);
        yModel.SetMaterialData(8, 0, 0);
        zModel.SetMaterialData(8, 0, 0);

        xModel.SetColor(Color(1, 0, 0, 1));
        zModel.SetColor(Color(0, 0, 1, 1));
        yModel.SetColor(Color(0, 1, 0, 1));

        xModel.SetOverdraw(true);
        yModel.SetOverdraw(true);
        zModel.SetOverdraw(true);

        if (m_currentID != -1) moveToolToEntity(m_currentID);
    }
}

void ObjectSelectorSystem::updateTranslation(const ursine::SVec3& mousePos)
{
    auto transf = m_world->GetEntityUnique(m_currentID)->GetComponent<ecs::Transform>( );
    Vec3 pos = m_baseTranslation;

    Vec3 total = mousePos - m_baseMousePos;
    
    switch (m_axis)
    {
    case 0:
        pos.SetX(mousePos.X( ) - m_baseMousePos.X( ) + m_baseTranslation.X());
        break;
    case 1:
        pos.SetY(mousePos.Y() - m_baseMousePos.Y( ) + m_baseTranslation.Y( ));
        break;
    case 2:
        pos.SetZ(mousePos.Z() - m_baseMousePos.Z( ) + m_baseTranslation.Z( ));
        break;
    }

    transf->SetWorldPosition(pos);
    updateToolPosition(pos);
}

void ObjectSelectorSystem::setToScale()
{
    //get their models
    auto xHand = m_xAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );
    auto yHand = m_yAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );
    auto zHand = m_zAxis->GetComponent<ursine::ecs::Renderable>( )->GetHandle( );

    auto &xModel = m_graphics->RenderableMgr.GetModel3D(xHand);
    auto &yModel = m_graphics->RenderableMgr.GetModel3D(yHand);
    auto &zModel = m_graphics->RenderableMgr.GetModel3D(zHand);

    {

        xModel.SetModel("Cube");
        yModel.SetModel("Cube");
        zModel.SetModel("Cube");

        xModel.SetMaterial("Blank");
        yModel.SetMaterial("Blank");
        zModel.SetMaterial("Blank");

        xModel.SetMaterialData(8, 0, 0);
        yModel.SetMaterialData(8, 0, 0);
        zModel.SetMaterialData(8, 0, 0);

        xModel.SetColor(Color(1, 0, 0, 1));
        zModel.SetColor(Color(0, 0, 1, 1));
        yModel.SetColor(Color(0, 1, 0, 1));

        xModel.SetOverdraw(true);
        yModel.SetOverdraw(true);
        zModel.SetOverdraw(true);

        if (m_currentID != -1) moveToolToEntity(m_currentID);
    }
}

void ObjectSelectorSystem::updateScale(const ursine::SVec3& mousePos)
{
    auto transf = m_world->GetEntityUnique(m_currentID)->GetComponent<ecs::Transform>( );
    Vec3 scale = transf->GetWorldScale();

    switch (m_axis)
    {
    case 0:
        scale.SetX(mousePos.X( ) - m_baseMousePos.X( ) + m_baseScale.X( ));
        break;
    case 1:
        scale.SetY(mousePos.Y( ) - m_baseMousePos.Y( ) + m_baseScale.Y( ));
        break;
    case 2:
        scale.SetZ(mousePos.Z( ) - m_baseMousePos.Z( ) + m_baseScale.Z( ));
        break;
    }


    transf->SetWorldScale(scale);
}

void ObjectSelectorSystem::setToRotation()
{
    float zoom = m_world->GetEntitySystem(EditorCameraSystem)->GetCamZoom( );

    
}

void ObjectSelectorSystem::updateRotation(const ursine::SVec3& mousePos)
{
    
}

void ObjectSelectorSystem::hideTool()
{
    m_dragging = false;
    m_axis = -1;

    m_baseTranslation = SVec3(2000, 2000, 2000);
    updateToolPosition(m_baseTranslation);
}

ursine::SVec3 ObjectSelectorSystem::GetMousePosition(const ursine::Vec2 mousePos)
{
    //get the editor camera
    graphics::Camera *cam = m_world->GetEntitySystem(EditorCameraSystem)->GetEditorCamera( );

    //get the mouse position
    Vec2 screenPos = mousePos;

    Vec3 camPos = cam->GetPosition( );

    //get the mouse world positions
    Vec3 p1 = cam->ScreenToWorld(screenPos, 0.1f);
    Vec3 p2 = cam->ScreenToWorld(screenPos, 1.f);

    //create a vector going out from the eye
    Vec3 mouseVec = p1 - p2;
    mouseVec.Set(mouseVec.X( ), mouseVec.Y( ), mouseVec.Z( ));

    //project onto the CURRENT place, which is dependent on the base position
    //x axis, we will treat z as stationary
    //z value will solve to the current z of the position
    //need t value
    //project onto all 3 planes, calculate an x, y, and z

    float timeX = (m_baseTranslation.Z( ) - camPos.Z( )) / mouseVec.Z( );
    float timeY = (m_baseTranslation.X( ) - camPos.X( )) / mouseVec.X( );
    float timeZ = (m_baseTranslation.X( ) - camPos.X( )) / mouseVec.X( );

    float x = camPos.X( ) + timeX * mouseVec.X( );
    float y = camPos.Y( ) + timeY * mouseVec.Y( );
    float z = camPos.Z( ) + timeZ * mouseVec.Z( );

    return SVec3(x, y, z);
}

void ObjectSelectorSystem::updateBases()
{
    if (m_currentID == -1)
        return;

    auto obj = m_world->GetEntityUnique(m_currentID);

    m_baseTranslation = obj->GetComponent<ecs::Transform>( )->GetWorldPosition( );
    m_baseScale = obj->GetComponent<ecs::Transform>( )->GetWorldScale( );
}