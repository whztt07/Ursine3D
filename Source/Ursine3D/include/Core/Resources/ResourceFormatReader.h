#pragma once

#include "ResourceReader.h"
#include "ResourceData.h"

namespace ursine
{
    namespace resources
    {
        class ResourceFormatReader
        {
        public:
            ResourceFormatReader(void);

            ResourceData::Handle Read(const fs::path &resourceFile);

        private:
            ResourceReader m_reader;
        };
    }
}