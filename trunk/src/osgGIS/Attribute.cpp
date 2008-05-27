/**
 * osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007 Glenn Waldron and Pelican Ventures, Inc.
 * http://osggis.org
 *
 * osgGIS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <osgGIS/Attribute>
#include <osgGIS/Utils>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace osgGIS;

Attribute
Attribute::invalid()
{
    return Attribute();
}

Attribute::Attribute()
{
    valid = false;
}

Attribute::Attribute( const std::string& _key, const std::string& _value )
{
    key = StringUtils::toLower( _key );
    string_value = _value;
    type = TYPE_STRING;
    valid = true;
}

Attribute::Attribute( const std::string& _key, const char* _value )
{
    key = StringUtils::toLower( _key );
    string_value = _value;
    type = TYPE_STRING;
    valid = true;
}

Attribute::Attribute( const std::string& _key, int _value )
{
    key = StringUtils::toLower( _key );
    int_value = _value;
    type = TYPE_INT;
    valid = true;
}

Attribute::Attribute( const std::string& _key, double _value )
{
    key = StringUtils::toLower( _key );
    double_value = _value;
    type = TYPE_DOUBLE;
    valid = true;
}

Attribute::Attribute( const std::string& _key, bool _value )
{
    key = StringUtils::toLower( _key );
    int_value = _value? 1 : 0;
    type = TYPE_BOOL;
    valid = true;
}

Attribute::~Attribute()
{
    //NOP
}

bool
Attribute::isValid() const
{
    return valid;
}

const std::string&
Attribute::getKey() const
{
    return key;
}

const Attribute::Type&
Attribute::getType() const
{
    return type;
}

const char*
Attribute::asString() const
{
    if ( type != TYPE_STRING && string_value.length() == 0 )
    {
        std::stringstream gen;
        if ( type == TYPE_INT )
            gen << int_value;
        else if ( type == TYPE_DOUBLE )
            gen << double_value;
        const_cast<Attribute*>(this)->string_value = gen.str();
    }
    return string_value.c_str();
}

int
Attribute::asInt() const
{
    if ( type == TYPE_INT )
        return int_value;
    else if ( type == TYPE_DOUBLE )
        return (int)double_value;
    else if ( type == TYPE_BOOL )
        return int_value;
    else {
        int temp;
        sscanf( const_cast<Attribute*>(this)->string_value.c_str(), "%d", &temp );
        return temp;
    }
}

double
Attribute::asDouble() const
{
    if ( type == TYPE_DOUBLE )
        return double_value;
    else if ( type == TYPE_INT )
        return (double)int_value;
    else if ( type == TYPE_BOOL )
        return (double)int_value;
    else {
        double temp;
        sscanf( const_cast<Attribute*>(this)->string_value.c_str(), "%lf", &temp );
        return temp;
    }
}

bool
Attribute::asBool() const
{
    if ( type == TYPE_BOOL )
        return int_value != 0;
    else if ( type == TYPE_INT )
        return int_value != 0;
    else if ( type == TYPE_DOUBLE )
        return double_value != 0.0;
    else {
        std::string temp = string_value;
        std::transform( temp.begin(), temp.end(), temp.begin(), tolower );
        return string_value == "true" || string_value == "yes" || string_value == "on" ||
               string_value == "t" || string_value == "y";
    }
}


/*****************************************************************************/


AttributeSchema::AttributeSchema(const std::string&     _attr_name,
                                 const Attribute::Type& _attr_type )
{
    name = StringUtils::toLower( _attr_name );
    type = _attr_type;
}
        
AttributeSchema::AttributeSchema(const std::string&     _attr_name,
                                 const Attribute::Type& _attr_type,
                                 const Properties&      _props )
{
    name = StringUtils::toLower( _attr_name );
    type = _attr_type;
    props = _props;
}
        
const std::string&
AttributeSchema::getName() const
{
    return name;
}

const Attribute::Type& 
AttributeSchema::getType() const
{
    return type;
}

const Properties& 
AttributeSchema::getProperties() const
{
    return props;
}


/***************************************************************************/


Attribute
AttributedBase::getAttribute( const std::string& key ) const
{
    std::string lkey = StringUtils::toLower( key );
    AttributeTable::const_iterator i = user_attrs.find( lkey );
    return i != user_attrs.end()? i->second : Attribute::invalid();
}

AttributeList
AttributedBase::getAttributes() const
{
    AttributeList result;
    for( AttributeTable::const_iterator i = user_attrs.begin(); i != user_attrs.end(); i++ )
        result.push_back( i->second );
    return result;
}

AttributeSchemaList
AttributedBase::getAttributeSchemas() const
{
    AttributeSchemaList result;

    for( AttributeTable::const_iterator i = user_attrs.begin(); i != user_attrs.end(); i++ )
    {
        result.push_back( AttributeSchema( i->first, i->second.getType(), Properties() ) );
    }

    return result;
}

void 
AttributedBase::setAttribute( const std::string& key, const std::string& value )
{
    std::string lkey = StringUtils::toLower( key );
    user_attrs[lkey] = Attribute( lkey, value );
}

void 
AttributedBase::setAttribute( const std::string& key, int value )
{
    std::string lkey = StringUtils::toLower( key );
    user_attrs[lkey] = Attribute( lkey, value );
}

void 
AttributedBase::setAttribute( const std::string& key, double value )
{
    std::string lkey = StringUtils::toLower( key );
    user_attrs[lkey] = Attribute( lkey, value );
}

void
AttributedBase::setAttribute( const std::string& key, bool value )
{
    std::string lkey = StringUtils::toLower( key );
    user_attrs[lkey] = Attribute( lkey, value );
}

const AttributeTable&
AttributedBase::getUserAttrs() const
{
    return user_attrs;
}