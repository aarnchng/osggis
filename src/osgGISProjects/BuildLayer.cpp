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

#include <osgGISProjects/BuildLayer>
#include <osgGIS/Utils>
#include <osgDB/FileNameUtils>

using namespace osgGISProjects;
using namespace osgGIS;

BuildLayer::BuildLayer()
{
    type = TYPE_SIMPLE;
}

BuildLayer::BuildLayer( const std::string& _name )
{
    setName( _name );
    type = TYPE_SIMPLE;
}

const std::string&
BuildLayer::getName() const
{
    return name;
}

void
BuildLayer::setName( const std::string& _name )
{
    name = _name;
}

void
BuildLayer::setBaseURI( const std::string& value )
{
    base_uri = value;
}

Source*
BuildLayer::getSource()
{
    return source.get();
}

void
BuildLayer::setSource( Source* _source )
{
    source = _source;
}

Terrain*
BuildLayer::getTerrain()
{
    return terrain.get();
}

void
BuildLayer::setTerrain( Terrain* _terrain )
{
    terrain = _terrain;
}

const std::string&
BuildLayer::getTargetPath() const
{
    return target_path;
}

const std::string
BuildLayer::getAbsoluteTargetPath() const
{
    return PathUtils::getAbsPath( base_uri, target_path );
}

void
BuildLayer::setTargetPath( const std::string& value )
{
    target_path = value;
}

const BuildLayerSliceList&
BuildLayer::getSlices() const
{
    return slices;
}

BuildLayerSliceList&
BuildLayer::getSlices()
{
    return slices;
}

void
BuildLayer::setType( BuildLayer::LayerType value )
{
    type = value;
}

BuildLayer::LayerType
BuildLayer::getType() const
{
    return type;
}

const Properties&
BuildLayer::getProperties() const
{
    return properties;
}

Properties&
BuildLayer::getProperties()
{
    return properties;
}

