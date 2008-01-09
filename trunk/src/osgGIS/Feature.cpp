#include <osgGIS/Feature>

using namespace osgGIS;

Attribute
FeatureBase::getAttribute( const std::string& key ) const
{
    AttributeTable::const_iterator i = user_attrs.find( key );
    return i != user_attrs.end()? i->second : Attribute::invalid();
}

double
FeatureBase::getAttributeAsDouble( const std::string& key ) const
{
    Attribute attr = getAttribute( key );
    return attr.isValid()? attr.asDouble() : 0.0;
}

int 
FeatureBase::getAttributeAsInt( const std::string& key ) const
{
    Attribute a = getAttribute( key );
    return a.isValid()? a.asInt() : 0;
}

bool 
FeatureBase::getAttributeAsBool( const std::string& key ) const
{
    Attribute a = getAttribute( key );
    return a.isValid()? a.asBool() : false;
}

std::string 
FeatureBase::getAttributeAsString( const std::string& key ) const
{
    Attribute a = getAttribute( key );
    return a.isValid()? a.asString() : "";
}

void 
FeatureBase::setAttribute( const std::string& key, const std::string& value )
{
    user_attrs[key] = Attribute( key, value );
}

void 
FeatureBase::setAttribute( const std::string& key, int value )
{
    user_attrs[key] = Attribute( key, value );
}

void 
FeatureBase::setAttribute( const std::string& key, double value )
{
    user_attrs[key] = Attribute( key, value );
}

void
FeatureBase::setAttribute( const std::string& key, bool value )
{
    user_attrs[key] = Attribute( key, value );
}

const AttributeTable&
FeatureBase::getUserAttrs() const
{
    return user_attrs;
}
