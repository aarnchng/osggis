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

#include <osgGIS/Script>

using namespace osgGIS;

Script::Script()
{
    //NOP
}

Script::Script(const std::string& _code )
{
    setCode( _code );
}

Script::Script(const std::string& _name,
               const std::string& _language, 
               const std::string& _code )
{
    setName( _name );
    setLanguage( _language );
    setCode( _code );
}

void
Script::setName( const std::string& _name )
{
    name = _name;
}

const std::string&
Script::getName() const
{
    return name;
}

void
Script::setLanguage( const std::string& _language )
{
    language = _language;
}

const std::string&
Script::getLanguage() const
{
    return language;
}

void
Script::setCode( const std::string& _code )
{
    code = _code;
}

const std::string&
Script::getCode() const
{
    return code;
}


ScriptResult
ScriptResult::Error( const std::string& _msg )
{
    return ScriptResult( false, _msg );
}

ScriptResult::ScriptResult( bool _valid, const std::string& _msg )
{
    valid = false;
    prop = Property( "", _msg );
}

ScriptResult::ScriptResult()
{
    valid = false;
}

ScriptResult::ScriptResult( const std::string& val )
{
    prop = Property( "", val );
    valid = true;
}

ScriptResult::ScriptResult( double val )
{
    prop = Property( "", val );
    valid = true;
}

ScriptResult::ScriptResult( int val )
{
    prop = Property( "", val );
    valid = true;
}

ScriptResult::ScriptResult( bool val )
{
    prop = Property( "", val );
    valid = true;
}

ScriptResult::ScriptResult( osg::Referenced* val )
{
    prop = Property( "", val );
    valid = true;
}

bool 
ScriptResult::isValid()
{
    return valid;
}

std::string 
ScriptResult::asString() const
{
    return prop.getValue();
}

float      
ScriptResult::asFloat( float def ) const
{
    return prop.getFloatValue( def );
}

double      
ScriptResult::asDouble( double def ) const
{
    return prop.getDoubleValue( def );
}

int         
ScriptResult::asInt( int def ) const
{
    return prop.getIntValue( def );
}

bool        
ScriptResult::asBool( bool def ) const
{
    return prop.getBoolValue( def );
}

osg::Vec4
ScriptResult::asVec4() const
{
    return prop.getVec4Value();
}

osg::Vec3
ScriptResult::asVec3() const
{
    return prop.getVec3Value();
}

osg::Referenced*
ScriptResult::asRef()
{
    return prop.getRefValue();
}

