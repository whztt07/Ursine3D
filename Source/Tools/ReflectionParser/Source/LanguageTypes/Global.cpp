#include "Precompiled.h"

#include "LanguageTypes/Global.h"
#include "LanguageTypes/Class.h"

const TemplateData::PartialType Global::m_getterPartial = [](void)
{
    return
        "{{#hasExplicitGetter}}"
            "{{& explicitGetter}}( )"
        "{{/hasExplicitGetter}}"
        "{{^hasExplicitGetter}}"
            "{{#hasParent}}"
                "{{parentQualifiedName}}::{{name}}"
            "{{/hasParent}}"
            "{{^hasParent}}"
                "{{qualifiedName}}"
            "{{/hasParent}}"
        "{{/hasExplicitGetter}}";
};

const TemplateData::PartialType Global::m_setterPartial = [](void)
{
    return
        "{{#hasExplicitSetter}}"
            "{{& explicitSetter}}( value.GetValue<{{type}}>( ) );"
        "{{/hasExplicitSetter}}"
        "{{^hasExplicitSetter}}"
            "{{#hasParent}}"
                "{{parentQualifiedName}}::{{name}} = value.GetValue<{{type}}>( );"
            "{{/hasParent}}"
            "{{^hasParent}}"
                "{{qualifiedName}} = value.GetValue<{{type}}>( );"
            "{{/hasParent}}"
        "{{/hasExplicitSetter}}";
};

Global::Global(const Cursor &cursor, const Namespace &currentNamespace, Class *parent)
    : LanguageType( cursor, currentNamespace )
    , m_isConst( cursor.GetType( ).IsConst( ) )
    , m_hasExplicitGetter( m_metaData.GetFlag( kMetaExplicitGetter ) )
    , m_hasExplicitSetter( m_metaData.GetFlag( kMetaExplicitSetter ) )
    , m_parent( parent )
    , m_name( cursor.GetSpelling( ) )
    , m_qualifiedName( utils::GetQualifiedName( cursor, currentNamespace ) )
    , m_type( cursor.GetType( ).GetDisplayName( ) )
{
    auto displayName = m_metaData.GetProperty( kMetaDisplayName );

    if (displayName.empty( ))
    {
        m_displayName = m_qualifiedName;
    }
    else
    {
        m_displayName = utils::GetQualifiedName( displayName, currentNamespace );
    }
}

TemplateData Global::CompileTemplate(void) const
{
    TemplateData data = { TemplateData::Type::Object };

    data[ "displayName" ] = m_displayName;
    data[ "qualifiedName" ] = m_qualifiedName;
    data[ "type" ] = m_type;

    data[ "isAccessible" ] = utils::TemplateBool( isAccessible( ) );
    data[ "hasParent" ] = utils::TemplateBool( !!m_parent );

    if (m_parent)
        data[ "parentQualifiedName" ] = m_parent->m_qualifiedName;

    // getter

    data[ "isGetterAccessible" ] = utils::TemplateBool( isGetterAccessible( ) );
    data[ "hasExplicitGetter" ] = utils::TemplateBool( m_hasExplicitGetter );
    data[ "explicitGetter" ] = m_metaData.GetProperty( kMetaExplicitGetter );
    data[ "getterBody" ] = m_getterPartial;

    // setter

    data[ "isSetterAccessible" ] = utils::TemplateBool( isSetterAccessible( ) );
    data[ "hasExplicitSetter" ] = utils::TemplateBool( m_hasExplicitSetter );
    data[ "explicitSetter" ] = m_metaData.GetProperty( kMetaExplicitSetter );
    data[ "setterBody" ] = m_setterPartial;

    m_metaData.CompileTemplateData( data );

    return data;
}

bool Global::isAccessible(void) const
{
    return isGetterAccessible( ) || isSetterAccessible( );
}

bool Global::isGetterAccessible(void) const
{
    if (m_metaData.GetFlag( kMetaEnable ))
    {
        if (m_parent)
            return m_hasExplicitGetter || m_accessModifier == CX_CXXPublic;

        return true;
    }

    return false;
}

bool Global::isSetterAccessible(void) const
{
    if (m_isConst)
        return false;

    if (m_metaData.GetFlag( kMetaEnable ))
    {
        if (m_parent)
            return m_hasExplicitSetter || m_accessModifier == CX_CXXPublic;

        return true;
    }

    return false;
}