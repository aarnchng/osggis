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

#include <osgGIS/SkinResource>

using namespace osgGIS;

SkinResource::SkinResource()
{
    init();
}

SkinResource::SkinResource( const std::string& _name )
: Resource( _name )
{
    init();
}

void
SkinResource::init()
{
    setTextureWidthMeters( 3.0f );
    setTextureHeightMeters( 4.0f );
    setColor( osg::Vec4( 1, 1, 1, 1 ) );
}

SkinResource::~SkinResource()
{
    //NOP
}

void
SkinResource::setProperty( const Property& prop )
{
    if ( prop.getName() == "texture_width" )
        setTextureWidthMeters( prop.getFloatValue( getTextureWidthMeters() ) );
    else if ( prop.getName() == "texture_height" )
        setTextureHeightMeters( prop.getFloatValue( getTextureHeightMeters() ) );
    else if ( prop.getName() == "texture_path" )
        setTexturePath( prop.getValue() );
    else if ( prop.getName() == "color" )
        setColor( prop.getVec4Value() );
    else
        Resource::setProperty( prop );
}

Properties
SkinResource::getProperties() const
{
    Properties props = Resource::getProperties();
    props.push_back( Property( "texture_path", getTexturePath() ) );
    props.push_back( Property( "texture_width", getTextureWidthMeters() ) );
    props.push_back( Property( "texture_height", getTextureHeightMeters() ) );
    props.push_back( Property( "color", getColor() ) );
    return props;
}

void 
SkinResource::setTexturePath( const std::string& value )
{
    tex_path = value;
}

const std::string& 
SkinResource::getTexturePath() const
{
    return tex_path;
}

void 
SkinResource::setTextureWidthMeters( float value )
{
    tex_width_m = value;
}

float 
SkinResource::getTextureWidthMeters() const
{
    return tex_width_m;
}

void 
SkinResource::setTextureHeightMeters( float value )
{
    tex_height_m = value;
}

float 
SkinResource::getTextureHeightMeters() const
{
    return tex_height_m;
}

void
SkinResource::setColor( const osg::Vec4& value )
{
    color = value;
}

const osg::Vec4&
SkinResource::getColor() const
{
    return color;
}
