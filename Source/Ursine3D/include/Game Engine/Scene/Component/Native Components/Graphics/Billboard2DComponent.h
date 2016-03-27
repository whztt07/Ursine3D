/* ----------------------------------------------------------------------------
** Team Bear King
** © 2015 DigiPen Institute of Technology, All Rights Reserved.
**
** Billboard2DComponent.h
**
** Author:
** - Jordan Ellis - j.ellis@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/

#pragma once

#include "Component.h"
#include "Renderable.h"
#include "GfxAPI.h"
#include "RenderableComponentBase.h"
#include "RenderMask.h"
#include "TextureData.h"

namespace ursine
{
    namespace ecs
    {
        class Billboard2D
            : public Component
        {
            NATIVE_COMPONENT;

            friend class RenderSystem;

        public:
            EditorResourceField(
                ursine::resources::TextureData,
                texture,
                GetTexture,
                SetTexture
            );

            EditorField(
                ursine::Vec2 dimensions,
                GetDimensions,
                SetDimensions
            );

            EditorMeta(BitMaskEditor)
            EditorField(
                ursine::ecs::RenderMask renderMask,
                GetRenderMask,
                SetRenderMask
            );

            Meta(Enable)
            Billboard2D(void);
            ~Billboard2D(void);

            void OnInitialize(void) override;
            void OnSceneReady(Scene *scene) override;

            //get/set model
            graphics::Billboard2D *GetBillboard(void);

            const resources::ResourceReference &GetTexture(void) const;
            void SetTexture(const resources::ResourceReference &texture);

            ursine::Vec2 GetDimensions(void) const;
            void SetDimensions(const ursine::Vec2 &dimensions);

            RenderMask GetRenderMask(void) const;
            void SetRenderMask(RenderMask mask);

        private:
            graphics::GfxAPI *m_graphics;
            graphics::Billboard2D *m_billboard;

            RenderableComponentBase *m_base;

            resources::ResourceReference m_texture;

            void updateRenderer(void);

            void invalidateTexture(bool unload = true);
            void onTextureReload(void);
        } Meta(
            Enable, 
            WhiteListMethods, 
            DisplayName( "Billboard2D" )
        );
    }
}