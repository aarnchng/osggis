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

#include <osgGISProjects/Project>
#include <algorithm>

using namespace osgGISProjects;
using namespace osgGIS;

Project::Project()
{
    //NOP
}

Project::~Project()
{
    //NOP
}

void
Project::setName( const std::string& value )
{
    name = value;
}

const std::string&
Project::getName() const
{
    return name;
}

const ScriptList&
Project::getScripts() const
{
    return scripts;
}

ScriptList&
Project::getScripts()
{
    return scripts;
}

const FilterGraphList&
Project::getFilterGraphs() const
{
    return graphs;
}

FilterGraphList&
Project::getFilterGraphs()
{
    return graphs;
}

FilterGraph*
Project::getFilterGraph( const std::string& key )
{
    for( FilterGraphList::const_iterator i = graphs.begin(); i != graphs.end(); i++ )
    {
        if ( i->get()->getName() == key )
            return i->get();
    }
    return NULL;
}

const SourceList&
Project::getSources() const
{
    return sources;
}

SourceList&
Project::getSources()
{
    return sources;
}

Source*
Project::getSource( const std::string& key )
{
    for( SourceList::const_iterator i = sources.begin(); i != sources.end(); i++ )
    {
        if ( i->get()->getName() == key )
            return i->get();
    }
    return NULL;
}

const TerrainList&
Project::getTerrains() const
{
    return terrains;
}

TerrainList&
Project::getTerrains()
{
    return terrains;
}

Terrain*
Project::getTerrain( const std::string& key )
{
    for( TerrainList::const_iterator i = terrains.begin(); i != terrains.end(); i++ )
    {
        if ( i->get()->getName() == key )
            return i->get();
    }
    return NULL;
}

const BuildLayerList&
Project::getLayers() const
{
    return layers;
}

BuildLayerList&
Project::getLayers()
{
    return layers;
}

BuildLayer*
Project::getLayer( const std::string& key )
{
    for( BuildLayerList::const_iterator i = layers.begin(); i != layers.end(); i++ )
    {
        if ( i->get()->getName() == key )
            return i->get();
    }
    return NULL;
}

const BuildTargetList&
Project::getTargets() const
{
    return targets;
}

BuildTargetList&
Project::getTargets()
{
    return targets;
}

BuildTarget*
Project::getTarget( const std::string& key )
{
    for( BuildTargetList::const_iterator i = targets.begin(); i != targets.end(); i++ )
    {
        if ( i->get()->getName() == key )
            return i->get();
    }
    return NULL;
}

const ResourceList&
Project::getResources() const
{
    return resources;
}

ResourceList&
Project::getResources()
{
    return resources;
}