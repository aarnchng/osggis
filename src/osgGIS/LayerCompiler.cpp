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

#include <osgGIS/LayerCompiler>
#include <osgSim/LineOfSight>
#include <osgDB/ReadFile>
#include <map>
#include <string>
#include <queue>

using namespace osgGIS;




LayerCompiler::LayerCompiler()
{
    read_cb = new SmartReadCallback();
    render_bin_number = INT_MAX;
    fade_lods = false;
}


void
LayerCompiler::addScript( float min_range, float max_range, Script* script )
{
    ScriptRange slice;
    slice.min_range = min_range;
    slice.max_range = max_range;
    slice.script = script;

    script_ranges.push_back( slice );
}



void
LayerCompiler::setTerrain(osg::Node*              _terrain,
                          const SpatialReference* _terrain_srs,
                          const GeoExtent&        _terrain_extent )
{
    terrain        = _terrain;
    terrain_srs    = (SpatialReference*)_terrain_srs;
    terrain_extent = _terrain_extent;
}


void
LayerCompiler::setTerrain(osg::Node*              _terrain,
                          const SpatialReference* _terrain_srs )
{
    setTerrain( _terrain, _terrain_srs, GeoExtent::infinite() );
}


void
LayerCompiler::setRenderBinNumber( int value )
{
    render_bin_number = value;
}


int
LayerCompiler::getRenderBinNumber() const
{
    return render_bin_number;
}

void
LayerCompiler::setFadeLODs( bool value )
{
    fade_lods = value;
}

bool
LayerCompiler::getFadeLODs() const
{
    return fade_lods;
}
