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
#include <osgSim/OverlayNode>
#include <osgDB/ReadFile>
#include <osg/TexEnv>
#include <map>
#include <string>
#include <queue>

using namespace osgGIS;




LayerCompiler::LayerCompiler()
{
    read_cb = new SmartReadCallback();
    render_bin_number = INT_MAX;
    fade_lods = false;
    overlay = false;
    aoi_xmin = DBL_MAX;
    aoi_ymin = DBL_MAX;
    aoi_xmax = DBL_MIN;
    aoi_ymax = DBL_MIN;
}

void
LayerCompiler::setTaskManager( TaskManager* _manager )
{
    task_manager = _manager;
}

TaskManager*
LayerCompiler::getTaskManager()
{
    return task_manager.get();
}

void
LayerCompiler::addFilterGraph( float min_range, float max_range, FilterGraph* graph )
{
    FilterGraphRange slice;
    slice.min_range = min_range;
    slice.max_range = max_range;
    slice.graph = graph;

    graph_ranges.push_back( slice );
}

LayerCompiler::FilterGraphRangeList&
LayerCompiler::getFilterGraphs()
{
    return graph_ranges;
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

osg::Node*
LayerCompiler::getTerrainNode()
{
    return terrain.get();
}

SpatialReference* 
LayerCompiler::getTerrainSRS() 
{
    return terrain_srs.get();
}

const GeoExtent& 
LayerCompiler::getTerrainExtent() const
{
    return terrain_extent;
}

void
LayerCompiler::setArchive( osgDB::Archive* _archive )
{
    archive = _archive;
}


osgDB::Archive*
LayerCompiler::getArchive() 
{
    return archive.get();
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


void
LayerCompiler::setPaged( bool value )
{
    paged = value;
}

bool
LayerCompiler::getPaged() const
{
    return paged;
}

void
LayerCompiler::setOverlay( bool value )
{
    overlay = value;
}

bool
LayerCompiler::getOverlay() const
{
    return overlay;
}

osg::Node*
LayerCompiler::convertToOverlay( osg::Node* input )
{
    osgSim::OverlayNode* o_node = new osgSim::OverlayNode();
    o_node->getOrCreateStateSet()->setTextureAttribute( 1, new osg::TexEnv( osg::TexEnv::DECAL ) );
    o_node->setOverlaySubgraph( input );
    o_node->setOverlayTextureSizeHint( 1024 );
    return o_node;
}

Session*
LayerCompiler::getSession()
{
    if ( !session.valid() )
        session = new Session();

    return session.get();
}

void
LayerCompiler::setSession( Session* _session )
{
    session = _session;
}

void
LayerCompiler::setPreCompileExpr( const std::string& expr )
{
    pre_compile_expr = expr;
}

const std::string&
LayerCompiler::getPreCompileExpr() const
{
    return pre_compile_expr;
}

void
LayerCompiler::setAreaOfInterest( double x0, double y0, double x1, double y1 )
{
    aoi_xmin = x0;
    aoi_ymin = y0;
    aoi_xmax = x1;
    aoi_ymax = y1;
}

GeoExtent
LayerCompiler::getAreaOfInterest( FeatureLayer* layer )
{
    if ( aoi_xmin < aoi_xmax && aoi_ymin < aoi_ymax )
    {
        return GeoExtent( aoi_xmin, aoi_ymin, aoi_xmax, aoi_ymax, layer->getSRS() );
    }
    else
    {
        return layer->getExtent();
    }
}

Properties
LayerCompiler::getProperties()
{
    Properties props;
    //TODO - populate!
    return props;
}

void
LayerCompiler::setProperties( Properties& input )
{
    setPaged( input.getBoolValue( "paged", getPaged() ) );
    setFadeLODs( input.getBoolValue( "fade_lods", getFadeLODs() ) );
    setRenderBinNumber( input.getIntValue( "render_bin_number", getRenderBinNumber() ) );
    setPreCompileExpr( input.getValue( "pre_script", getPreCompileExpr() ) );

    aoi_xmin = input.getDoubleValue( "aoi_xmin", DBL_MAX );
    aoi_ymin = input.getDoubleValue( "aoi_ymin", DBL_MAX );
    aoi_xmax = input.getDoubleValue( "aoi_xmax", DBL_MIN );
    aoi_ymax = input.getDoubleValue( "aoi_ymax", DBL_MIN );
}
