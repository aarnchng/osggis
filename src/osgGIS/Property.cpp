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

#include <osgGIS/Property>
#include <sstream>
#include <algorithm>

using namespace osgGIS;

static std::string
normalize( std::string input )
{
    std::replace( input.begin(), input.end(), '_', '-' );
    return input;
}

Property::Property()
{
}

Property::Property( const std::string& _name, const std::string& _value )
{
    name = normalize( _name );
    value = _value;
}

Property::Property( const std::string& _name, int _value )
{
    name = normalize( _name );
    std::stringstream ss;
    ss << _value;
    value = ss.str();
}

Property::Property( const std::string& _name, float _value )
{
    name = normalize( _name );
    std::stringstream ss;
    ss << _value;
    value = ss.str();
}

Property::Property( const std::string& _name, double _value )
{
    name = normalize( _name );
    std::stringstream ss;
    ss << _value;
    value = ss.str();
}

Property::Property( const std::string& _name, bool _value )
{
    name = normalize( _name );
    value = _value? "true" : "false";
}

Property::Property( const std::string& _name, const osg::Vec2f& _v )
{
    name = normalize( _name );
    std::stringstream ss;
    ss << _v[0] << " " << _v[1];
    value = ss.str();
}

Property::Property( const std::string& _name, const osg::Vec3f& _v )
{
    name = normalize( _name );
    std::stringstream ss;
    ss << _v[0] << " " << _v[1] << " " << _v[2];
    value = ss.str();
}

Property::Property( const std::string& _name, const osg::Vec4f& _v )
{
    name = normalize( _name );
    std::stringstream ss;
    ss << _v[0] << " " << _v[1] << " " << _v[2] << " " << _v[3];
    value = ss.str();
}

Property::Property( const std::string& _name, const osg::Matrix& _v )
{
    name = normalize( _name );
    std::stringstream ss;
    const osg::Matrix::value_type* p = _v.ptr();
    for( int i=0; i<15; i++ ) ss << *p++ << " ";
    ss << *p;
    value = ss.str();
}

const std::string& 
Property::getName() const
{
    return name;
}

const std::string& 
Property::getValue() const
{
    return value;
}

int 
Property::getIntValue( int def ) const
{
    return value.length() > 0? ::atoi( value.c_str() ) : def;
}

float 
Property::getFloatValue( float def ) const
{
    return value.length() > 0? (float)::atof( value.c_str() ) : def;
}

double 
Property::getDoubleValue( double def ) const
{
    return value.length() > 0? ::atof( value.c_str() ) : def;
}

bool 
Property::getBoolValue( bool def ) const
{
    std::string temp = value;
    std::transform( temp.begin(), temp.end(), temp.begin(), tolower );
    return temp == "true" || temp == "yes" || temp == "on" ? true : false;
}

osg::Vec2f
Property::getVec2fValue() const
{
    osg::Vec2f v;
    std::stringstream ss( value );
    ss >> v[0] >> v[1];
    return v;
}

osg::Vec3f
Property::getVec3fValue() const
{
    osg::Vec3f v;
    std::stringstream ss( value );
    ss >> v[0] >> v[1] >> v[2];
    return v;
}

osg::Vec4f
Property::getVec4fValue() const
{
    osg::Vec4f v;
    std::stringstream ss( value );
    ss >> v[0] >> v[1] >> v[2] >> v[3];
    return v;
}

osg::Matrix
Property::getMatrixValue() const
{
    osg::Matrix m;
    std::stringstream ss( value );
    osg::Matrix::value_type* p = m.ptr();
    for( int i=0; i<16; i++ ) ss >> *p++;
    return m;
}


int 
Properties::getIntValue( const std::string& key, int def )
{
    std::string nkey = normalize( key );
    for( Properties::const_iterator i = begin(); i != end(); i++ )
    {
        if ( i->getName() == nkey )
        {
            return i->getIntValue( def );
        }
    }
    return def;
}

float 
Properties::getFloatValue( const std::string& key, float def )
{
    std::string nkey = normalize( key );
    for( Properties::const_iterator i = begin(); i != end(); i++ )
    {
        if ( i->getName() == nkey )
        {
            return i->getFloatValue( def );
        }
    }
    return def;
}

double 
Properties::getDoubleValue( const std::string& key, double def )
{
    std::string nkey = normalize( key );
    for( Properties::const_iterator i = begin(); i != end(); i++ )
    {
        if ( i->getName() == nkey )
        {
            return i->getDoubleValue( def );
        }
    }
    return def;
}

bool 
Properties::getBoolValue( const std::string& key, bool def )
{
    std::string nkey = normalize( key );
    for( Properties::const_iterator i = begin(); i != end(); i++ )
    {
        if ( i->getName() == nkey )
        {
            return i->getBoolValue( def );
        }
    }
    return def;
}

std::string 
Properties::getValue( const std::string& key, std::string def )
{
    std::string nkey = normalize( key );
    for( Properties::const_iterator i = begin(); i != end(); i++ )
    {
        if ( i->getName() == nkey )
        {
            return i->getValue();
        }
    }
    return def;
}
