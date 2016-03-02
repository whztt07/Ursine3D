#include "UrsinePrecompiled.h"

#include "WorldReader.h"
#include "WorldData.h"

namespace ursine
{
    namespace resources
    {
        WorldReader::WorldReader(void) { }

        ResourceData::Handle WorldReader::Read(ResourceReader &input)
        {
            std::string jsonString;

            input.ReadString( jsonString );

            std::string jsonError;

            auto json = Json::parse( jsonString, jsonError );

            return std::make_shared<WorldData>( json );
        }
    }
}