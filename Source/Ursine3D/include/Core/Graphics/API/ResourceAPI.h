/* Start Header ---------------------------------------------------------------
Copyright (C) 2015 DigiPen Institute of Technology. Reproduction or
disclosure of this file or its contents without the prior written
consent of DigiPen Institute of Technology is prohibited.
=============================================================================*/
/*!
File Name:      ResourceAPI.h
Module:         Graphics
Purpose:        interface for handling resources
Language:       C++

Project:        Graphics Prototype
Author:         Matt Yan, m.yan@digipen.edu
*/
/*- End Header --------------------------------------------------------------*/

#pragma once

#include "GFXAPIDefines.h"

namespace ursine
{
    namespace graphics
    {
        class ResourceAPI
        {
            friend class GfxAPI;
        private:
            struct privData;

        public:
            // get model handle
            GfxHND GetModelHandle(const char *name);

            // get texture handle
            GfxHND GetTexHandle(const char *name);

            // create a texture of a given width/height, returns handle
            // resize a texture
            // destroy a texture resource

            //validate that a model/texture exists

            //get a list of available models

            //get a list of available textures

        private:
            privData *m_privates;

        private:
            void SetPrivates(void *priv, void *priv2);
            void Initialize();
            void Uninitialize();
        };
    }
}