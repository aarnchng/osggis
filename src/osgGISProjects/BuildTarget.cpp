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

#include <osgGISProjects/BuildTarget>

using namespace osgGISProjects;

BuildTarget::BuildTarget()
{
    //NOP
}

BuildTarget::BuildTarget( const std::string& _name )
{
    setName( _name );
}

const std::string&
BuildTarget::getName() const
{
    return name;
}

void
BuildTarget::setName( const std::string& _name )
{
    name = _name;
}

Terrain*
BuildTarget::getTerrain() const
{
    return terrain.get();
}

void
BuildTarget::setTerrain( Terrain* value )
{
    terrain = value;
}

void
BuildTarget::addLayer( BuildLayer* _layer )
{
    layers.push_back( _layer );
}


const BuildLayerList&
BuildTarget::getLayers() const
{
    return layers;
}

