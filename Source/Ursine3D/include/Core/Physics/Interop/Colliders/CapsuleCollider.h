/* ---------------------------------------------------------------------------
** Team Bear King
** ?2015 DigiPen Institute of Technology, All Rights Reserved.
**
** CapsuleCollider.h
**
** Author:
** - Jordan Ellis - contact@jordanellis.me
**
** Contributors:
** - <list in same format as author if applicable>
** -------------------------------------------------------------------------*/

#pragma once

#include "PhysicsInteropConfig.h"

namespace ursine
{
    namespace physics
    {
        class CapsuleCollider : public CapsuleColliderBase
        {
        public:
            CapsuleCollider(void);

            float GetHeight(void) const;
            void SetHeight(float height);

            float GetRadius(void) const;
            void SetRadius(float radius);
        };
    }
}
