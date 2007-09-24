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

using namespace osgGISProjects;

BuildLayer::BuildLayer()
{
    //NOP
}

BuildLayer::BuildLayer( const std::string& _name )
{
    setName( _name );
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
BuildLayer::getTarget() const
{
    return target;
}

void
BuildLayer::setTarget( const std::string& _target )
{
    target = _target;
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