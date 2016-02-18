#pragma once

#include "ResourceMetaData.h"
#include "GUID.h"

namespace ursine
{
    namespace resources
    {
        namespace pipeline
        {
            class ResourceItem
            {
            public:
                typedef std::shared_ptr<ResourceItem> Handle;

                ResourceItem(const GUID &guid);

                const GUID &GetGUID(void) const;
                const fs::path &GetBuildFileName(void) const;

            private:
                friend class ResourcePipelineManager;

                fs::path m_fileName;
                fs::path m_metaFileName;
                fs::path m_buildFileName;

                GUID m_guid;
                ResourceMetaData m_metaData;
            };
        }
    }
}