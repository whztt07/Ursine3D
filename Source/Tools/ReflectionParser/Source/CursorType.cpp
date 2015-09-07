#include "Precompiled.h"

#include "CursorType.h"

CursorType::CursorType(const CXType &handle)
    : m_handle( handle )
{

}

std::string CursorType::GetDisplayName(void) const
{
    std::string displayName;

    utils::ToString( clang_getTypeSpelling( m_handle ), displayName );

    return displayName;
}

int CursorType::GetArgumentCount(void) const
{
    return clang_getNumArgTypes( m_handle );
}

CursorType CursorType::GetArgument(unsigned index) const
{
    return clang_getArgType( m_handle, index );
}

CursorType CursorType::GetCanonicalType(void) const
{
    return clang_getCanonicalType( m_handle );
}

bool CursorType::IsConst(void) const
{
    return clang_isConstQualifiedType( m_handle ) ? true : false;
}