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

const BuildList&
Project::getBuilds() const
{
    return builds;
}

BuildList&
Project::getBuilds()
{
    return builds;
}

