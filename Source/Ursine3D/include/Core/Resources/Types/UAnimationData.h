/* ----------------------------------------------------------------------------
** Team Bear King
** � 2016 DigiPen Institute of Technology, All Rights Reserved.
**
** UAnimationData.h
**
** Author:
** - Jordan Ellis - j.ellis@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** --------------------------------------------------------------------------*/

#pragma once

#include "ResourceData.h"

namespace ursine
{
    namespace resources
    {
        class UAnimationData : public ResourceData
        {
            RESOURCE_DATA(UAnimationData);

        public:

            std::string name;

            void Write(pipeline::ResourceWriter &output) override;
            meta::Type GetReaderType(void) override;

        } Meta(Register);
    }
}