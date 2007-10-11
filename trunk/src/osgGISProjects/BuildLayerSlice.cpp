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

#include <osgGISProjects/BuildLayerSlice>

using namespace osgGISProjects;
using namespace osgGIS;

BuildLayerSlice::BuildLayerSlice()
{
    min_range = 0.0;
    max_range = -1.0;
    min_res_level = -1;
    max_res_level = -1;
}

Script*
BuildLayerSlice::getScript()
{
    return script.get();
}

void
BuildLayerSlice::setScript( Script* _script )
{
    script = _script;
}


int
BuildLayerSlice::getMinResolutionLevel() const
{
    return min_res_level;
}

void
BuildLayerSlice::setMinResolutionLevel( int value )
{
    min_res_level = value;
}

int
BuildLayerSlice::getMaxResoltuionLevel() const
{
    return max_res_level;
}

void
BuildLayerSlice::setMaxResolutionLevel( int value )
{
    max_res_level = value;
}

double
BuildLayerSlice::getMinRange() const
{
    return min_range;
}

void
BuildLayerSlice::setMinRange( double value )
{
    min_range = value;
}

double
BuildLayerSlice::getMaxRange() const
{
    return max_range;
}

void
BuildLayerSlice::setMaxRange( double value )
{
    max_range = value;
    if ( max_range < 0 ) max_range = DBL_MAX;
}

