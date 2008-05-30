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

#include <osgGISProjects/Map>

using namespace osgGISProjects;
using namespace osgGIS;

Map::Map()
{
    //NOP
}

void
Map::setName( const std::string& value )
{
    name = value;
}

const std::string&
Map::getName() const
{
    return name;
}

void
Map::setTerrain( Terrain* value )
{
    terrain = value;
}

Terrain*
Map::getTerrain() const
{
    return terrain.get();
}

MapLayerList&
Map::getMapLayers()
{
    return map_layers;
}

const MapLayerList&
Map::getMapLayers() const
{
    return map_layers;
}
