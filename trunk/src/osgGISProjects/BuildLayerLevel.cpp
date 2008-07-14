/**
/* osgGIS - GIS Library for OpenSceneGraph
 * Copyright 2007-2008 Glenn Waldron and Pelican Ventures, Inc.
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

#include <osgGISProjects/BuildLayerLevel>

using namespace osgGISProjects;
using namespace osgGIS;

BuildLayerLevel::BuildLayerLevel()
{
    min_range = 0.0f;
    max_range = FLT_MAX;
    cell_width = 0.0;
    cell_height = 0.0;
    cell_size_factor = 1.0;
}

FilterGraph*
BuildLayerLevel::getFilterGraph()
{
    return graph.get();
}

void
BuildLayerLevel::setFilterGraph( FilterGraph* _graph )
{
    graph = _graph;
}

void
BuildLayerLevel::setSource( Source* value )
{
    source = value;
}

Source*
BuildLayerLevel::getSource() const
{
    return source.get();
}

BuildLayerLevelList&
BuildLayerLevel::getSubLevels()
{
    return sub_levels;
}

const BuildLayerLevelList&
BuildLayerLevel::getSubLevels() const
{
    return sub_levels;
}

double
BuildLayerLevel::getCellWidth() const
{
    return cell_width;
}

void
BuildLayerLevel::setCellWidth( double value )
{
    cell_width = value;
}

double
BuildLayerLevel::getCellHeight() const
{
    return cell_height;
}

void
BuildLayerLevel::setCellHeight( double value )
{
    cell_height = value;
}

void
BuildLayerLevel::setCellSizeFactor( double value )
{
    cell_size_factor = value;
}

double
BuildLayerLevel::getCellSizeFactor() const
{
    return cell_size_factor;
}

float
BuildLayerLevel::getMinRange() const
{
    return min_range;
}

void
BuildLayerLevel::setMinRange( float value )
{
    min_range = value;
}

float
BuildLayerLevel::getMaxRange() const
{
    return max_range;
}

void
BuildLayerLevel::setMaxRange( float value )
{
    max_range = value;
    if ( max_range < 0 ) max_range = FLT_MAX;
}


