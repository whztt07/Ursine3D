#pragma once

#include "RenderEffect.h"
#include "Shader.h"

namespace ursine
{
    namespace ecs
    {
        class BlurEffect : public RenderEffect
        {
            Shader _x_shader;
            Shader _y_shader;

            GLuint _buffer_vao;
            GLuint _buffer_vbo;


            SMat3 _world_to_ndc;

        public:
            BlurEffect(void);
            ~BlurEffect(void);

            void Bind(World *world) override;
            void Render(Renderable *renderable) override;
            void UnBind(void) override;

			ALLOW_ALIGNED_ALLOC(16)
        };
    }
}
