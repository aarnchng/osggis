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

#include <osgGISProjects/MapLayer>
#include <osgGIS/TransformFilter>
#include <osgDB/FileNameUtils>
#include <sstream>

using namespace osgGIS;
using namespace osgGISProjects;

MapLayerLevelOfDetail::MapLayerLevelOfDetail(FeatureLayer*     _layer,
                                             FilterGraph*      _graph,
                                             ResourcePackager* _packager,
                                             float             _min_range,
                                             float             _max_range,
                                             bool              _replace_previous,
                                             unsigned int      _depth )
{
    layer            = _layer;
    graph            = _graph;
    packager         = _packager;
    min_range        = _min_range;
    max_range        = _max_range;
    replace_previous = _replace_previous;
    depth            = _depth;
}

FeatureLayer*
MapLayerLevelOfDetail::getFeatureLayer() const {
    return layer.get();
}

FilterGraph*
MapLayerLevelOfDetail::getFilterGraph() const {
    return graph.get();
}

ResourcePackager*
MapLayerLevelOfDetail::getResourcePackager() const {
    return packager.get();
}

float
MapLayerLevelOfDetail::getMinRange() const {
    return min_range;
}

float
MapLayerLevelOfDetail::getMaxRange() const {
    return max_range;
}

bool
MapLayerLevelOfDetail::getReplacePrevious() const {
    return replace_previous;
}

unsigned int
MapLayerLevelOfDetail::getDepth() const {
    return depth;
}

// ============================================================================

MapLayerCell::MapLayerCell(unsigned int _col, unsigned int _row, unsigned int _level, 
                           const GeoExtent&       _extent,
                           MapLayerLevelOfDetail* _def )
{
    col    = _col;
    row    = _row;
    level  = _level;
    extent = _extent;
    def    = _def;
}

unsigned int
MapLayerCell::getCellRow() const {
    return row;
}

unsigned int
MapLayerCell::getCellColumn() const {
    return col;
}

unsigned int
MapLayerCell::getLevel() const {
    return level;
}

const GeoExtent&
MapLayerCell::getExtent() const {
    return extent;
}

MapLayerLevelOfDetail*
MapLayerCell::getDefinition() const {
    return def.get();
}

std::string
MapLayerCell::toString() const 
{
    std::stringstream s;
    s << "X" << col << "Y" << row << "L" << level;
    return s.str();
}

std::string
MapLayerCell::makeURIFromTemplate( const std::string& uri_template )
{
    std::stringstream buf;

    std::string file_path = osgDB::getFilePath( uri_template );
    if ( file_path.length() > 0 )
        buf << file_path << "/";

    buf << osgDB::getStrippedName( uri_template )
        << "_X" << col << "_Y" << row << "_L" << level 
        << "." << osgDB::getLowerCaseFileExtension( uri_template );

    return buf.str();
}

osg::BoundingSphere
MapLayerCell::getBoundingSphere( SpatialReference* srs )
{
    if ( srs )
    {
        GeoPoint centroid = srs->transform( extent.getCentroid() );
        GeoPoint sw = srs->transform( extent.getSouthwest() );
        GeoPoint ne = srs->transform( extent.getNortheast() );
        return osg::BoundingSphere( centroid, (centroid-sw).length() );
    }
    else
    {
        GeoPoint centroid = extent.getCentroid();
        return osg::BoundingSphere(
            centroid,
            (centroid-extent.getSouthwest()).length() );
    }
}

// ============================================================================

MapLayer::MapLayer()
{
    grid_valid = false;
    aoi_manual = GeoExtent::invalid();
}

void
MapLayer::setAreaOfInterest( const GeoExtent& value )
{
    aoi_manual = value;
    grid_valid = false;
}

const GeoExtent&
MapLayer::getAreaOfInterest() const
{
    return aoi_manual.isValid()? aoi_manual : aoi_auto;
}

void
MapLayer::setCellWidth( double value )
{
    cell_width = value;
    grid_valid = false;
}

double
MapLayer::getCellWidth() const
{
    return cell_width;
}

void
MapLayer::setCellHeight( double value )
{
    cell_height = value;
    grid_valid = false;
}

double
MapLayer::getCellHeight() const
{
    return cell_height;
}

void
MapLayer::push(FeatureLayer* layer, FilterGraph* graph, 
               ResourcePackager* packager,
               float min_range, float max_range, 
               bool replace_previous, unsigned int depth )
{
    if ( layer && graph )
    {
        // update the automatic AOI:
        if ( !aoi_auto.isValid() )
            aoi_auto = layer->getExtent();
        else
            aoi_auto.expandToInclude( layer->getExtent() );

        // store the LOD definition:
        levels.push_back( new MapLayerLevelOfDetail( layer, graph, packager, min_range, max_range, replace_previous, depth ) );
        
        grid_valid = false;
    }  
}

unsigned int
MapLayer::getMaxDepth() const
{
    unsigned int max_depth = 0;
    for( MapLayerLevelsOfDetail::const_iterator i = levels.begin(); i != levels.end(); i++ )
    {
        if ( i->get()->getDepth() > max_depth )
            max_depth = i->get()->getDepth();
    }
    return max_depth;
}

SpatialReference*
MapLayer::getOutputSRS( Session* session, SpatialReference* terrain_srs ) const
{
    if ( !grid_valid || !output_srs.valid() )
    {
        if ( levels.size() > 0 && levels[0]->getFilterGraph() )
        {
            osg::ref_ptr<FilterEnv> env = session->createFilterEnv();
            env->setTerrainSRS( terrain_srs );

            const FilterList& filters = levels[0]->getFilterGraph()->getFilters();

            for( FilterList::const_reverse_iterator i = filters.rbegin(); i != filters.rend() && !output_srs.valid(); i++ )
            {
                if ( dynamic_cast<TransformFilter*>( i->get() ) )
                {
                    TransformFilter* xf = static_cast<TransformFilter*>( i->get() );
                    if ( xf->getUseTerrainSRS() )
                    {
                        if ( env->getTerrainSRS() )
                        {
                            const_cast<MapLayer*>(this)->output_srs = env->getTerrainSRS();
                        }
                    }
                    else if ( xf->getSRS() )
                    {
                        const_cast<MapLayer*>(this)->output_srs = const_cast<SpatialReference*>( xf->getSRS() );
                    }
                    else if ( xf->getSRSScript() )
                    {
                        ScriptResult r = env->getScriptEngine()->run( xf->getSRSScript(), env.get() );
                        if ( r.isValid() )
                        {
                            const_cast<MapLayer*>(this)->output_srs = session->getResources()->getSRS( r.asString() );
                        }
                    }
                }
            }
        }
    }

    return output_srs.get();
}

unsigned int
MapLayer::getNumCellRows() const 
{
    if ( !grid_valid )
        const_cast<MapLayer*>(this)->recalculateGrid();
    return num_rows;
}

unsigned int
MapLayer::getNumCellColumns() const
{
    if ( !grid_valid )
        const_cast<MapLayer*>(this)->recalculateGrid();
    return num_cols;
}

unsigned int
MapLayer::getNumCellLevels() const
{
    return levels.size();
}

MapLayerCell*
MapLayer::createCell( unsigned int col, unsigned int row, unsigned int level ) const
{
    if ( !grid_valid )
        const_cast<MapLayer*>(this)->recalculateGrid();

    MapLayerCell* result = NULL;

    const GeoExtent& aoi = getAreaOfInterest();

    if ( col < getNumCellColumns() && row < getNumCellRows() && level < getNumCellLevels() && aoi.isValid() )
    {
        double col_width  = col+1 < getNumCellColumns()? dx : last_dx;
        double row_height = row+1 < getNumCellRows()? dy : last_dy;

        const GeoPoint& sw = aoi.getSouthwest();
        const GeoPoint& ne = aoi.getNortheast();

        GeoExtent cell_extent(
            sw.x() + (double)col*dx, sw.y() + (double)row*dy,
            sw.x() + (double)col*dx + col_width, sw.y() + (double)row*dy + row_height,
            aoi.getSRS() );

        result = new MapLayerCell( col, row, level, cell_extent, levels[level].get() );
    }

    return result;
}

void
MapLayer::recalculateGrid()
{
    num_rows = 0;
    num_cols = 0;
    dx       = 0.0;
    last_dx  = 0.0;
    dy       = 0.0;
    last_dy  = 0.0;

    const GeoExtent& aoi = getAreaOfInterest();
    if ( aoi.isValid() )
    {
        const GeoPoint& sw = aoi.getSouthwest();
        const GeoPoint& ne = aoi.getNortheast();
        osg::ref_ptr<const SpatialReference> srs = aoi.getSRS();

        if ( getCellHeight() > 0.0 )
        {
            num_rows = (int) ::ceil( aoi.getHeight()/getCellHeight() );
            dy = getCellHeight();
            last_dy = ::fmod( aoi.getHeight(), getCellHeight() );
            if ( last_dy == 0.0 )
                last_dy = dy;
        }
        else
        {
            num_rows = 1;
            dy = last_dy = aoi.getHeight();
        }

        if ( getCellWidth() > 0.0 )
        {
            num_cols = (int) ::ceil( aoi.getWidth()/getCellWidth() );
            dx = getCellWidth();
            last_dx = ::fmod( aoi.getWidth(), getCellWidth() );
            if ( last_dx == 0.0 )
                last_dx = dx;
        }
        else
        {
            num_cols = 1;
            dx = last_dx = aoi.getWidth();
        }

        grid_valid = true;
    }
}