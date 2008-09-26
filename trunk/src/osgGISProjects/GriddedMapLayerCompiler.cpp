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

#include <osgGISProjects/GriddedMapLayerCompiler>
#include <osgGIS/FeatureLayerCompiler>
#include <osgGIS/ResourcePackager>
#include <osgGIS/Report>
#include <osgGIS/Session>
#include <osgGIS/Utils>
#include <osgGIS/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/NodeVisitor>
#include <osgText/Text>
#include <sstream>
#include <list>

using namespace osgGIS;
using namespace osgGISProjects;
using namespace OpenThreads;

#define MY_PRIORITY_SCALE 1.0f //0.5f


/*****************************************************************************/

GridProfile::GridProfile(const GeoExtent& _bounds)
: Profile(), bounds(_bounds)
{
    //NOP
}

void 
GridProfile::initByCellSize( double col_size, double row_size )
{
    if ( row_size <= 0.0 )
        row_size = bounds.getHeight();

    if ( col_size <= 0.0 )
        col_size = bounds.getWidth();

    num_rows = (int) ::ceil( bounds.getHeight()/row_size );
    dy = row_size;
    last_dy = ::fmod( bounds.getHeight(), row_size );
    if ( last_dy == 0.0 )
        last_dy = dy;

    num_cols = (int) ::ceil( bounds.getWidth()/col_size );
    dx = col_size;
    last_dx = ::fmod( bounds.getWidth(), col_size );
    if ( last_dx == 0.0 )
        last_dx = dx;
}

void 
GridProfile::initByCellCount( unsigned int _num_cols, unsigned int _num_rows )
{
    num_rows = std::max( _num_rows, (unsigned int)1 );
    dy = bounds.getHeight() / (double)num_rows;
    last_dy = dy;

    num_cols = std::max( _num_cols, (unsigned int)1 );
    dx = bounds.getWidth() / (double)num_cols;
    last_dx = dx;
}

const GeoExtent& 
GridProfile::getBounds() const { 
    return bounds;
}

unsigned int 
GridProfile::getNumColumns() const { 
    return num_cols;
}

unsigned int 
GridProfile::getNumRows() const { 
    return num_rows;
}

double 
GridProfile::getColumnSize( unsigned int col ) const { 
    return col < num_cols-1 ? dx : last_dx;
}

double 
GridProfile::getRowSize( unsigned int row ) const {
    return row < num_rows-1 ? dy : last_dy;
}

GeoExtent 
GridProfile::getExtent( unsigned int col, unsigned int row ) const {
    const GeoPoint& sw = bounds.getSouthwest();
    const GeoPoint& ne = bounds.getNortheast();
    return GeoExtent(
        sw.x() + dx*(double)col, 
        sw.y() + dy*(double)row,
        sw.x() + dx*(double)col + getColumnSize( col ),
        sw.y() + dy*(double)row + getRowSize( row ),
        bounds.getSRS() );
}

std::string
GridProfile::getCellId( unsigned int col, unsigned int row, unsigned int depth ) const
{
    std::stringstream buf;
    buf << "L" << depth << "_X" << col << "_Y" << row;
    return buf.str();
}

/*****************************************************************************/


GridCell::GridCell(unsigned int _col,
                   unsigned int _row,
                   unsigned int _depth,
                   const GeoExtent& _extent,
                   const CellStatus& _status ) :
Cell(_extent,_status),
col(_col),
row(_row),
depth(_depth)
{
    //NOP
}

unsigned int 
GridCell::getColumn() const {
    return col;
}

unsigned int 
GridCell::getRow() const {
    return row;
}

unsigned int 
GridCell::getDepth() const {
    return depth;
}


/*****************************************************************************/

class GridCellKey
{
public:
    GridCellKey( GridProfile* _profile ) : profile(_profile) { }
    GridCellKey( const GridCellKey& rhs ) 
        : col(rhs.col), row(rhs.row), level(rhs.level), profile(rhs.profile.get()) { }
    GridCellKey( unsigned int _col, unsigned int _row, unsigned int _level, GridProfile* _profile )
        : col(_col), row(_row), level(_level), profile(_profile) { }
    
    GeoExtent getExtent() const {
        return profile->getExtent( col, row );
    }

    std::string toString() const {
        return profile->getCellId( col, row, level );
        //std::stringstream s;
        //s << "L" << level << "_X" << col << "_Y" << row;
        //return s.str();
    }

    unsigned int getColumn() const {
        return col;
    }

    unsigned int getRow() const {
        return row;
    }

    unsigned int getLevel() const {
        return level;
    }

private:
    unsigned int col, row, level;
    osg::ref_ptr<GridProfile> profile;
};

typedef std::list<GridCellKey> GridCellKeyList;

/*****************************************************************************/

GridCellCursor::GridCellCursor( GridProfile* _profile, MapLayerCompiler* _compiler ) :
profile( _profile ),
compiler( _compiler )
{
    lod_i = col_i = row_i = 0;
}

bool
GridCellCursor::hasNext() const
{
    return
        lod_i < compiler->getMapLayer()->getLevels().size() &&
        col_i < profile->getNumColumns() &&
        row_i < profile->getNumRows();
}

osg::ref_ptr<Cell>
GridCellCursor::next()
{
    unsigned int old_col = col_i, old_row = row_i, old_lod = lod_i;

    GeoExtent extent = profile->getExtent( col_i, row_i );
    GridCellKey key( col_i, row_i, lod_i, profile.get() );
    std::string path = compiler->createAbsPathFromTemplate( "g" + key.toString() );
    CellStatus status( osgDB::fileExists( path ) );

    // advance the iterator
    if ( ++col_i > profile->getNumColumns() )
    {
        col_i = 0;
        if ( ++row_i > profile->getNumRows() )
        {
            row_i = 0;
            lod_i++;
        }                
    }

    return new GridCell( old_col, old_row, old_lod, extent, status );
}

/*****************************************************************************/

static MapLayerLevelOfDetail*
getLodForKey( const GridCellKey& key, MapLayer* map_layer )
{
    unsigned int level = 0;
    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++, level++ )
    {
        if ( key.getLevel() == level )
            return i->get();
    }
    return NULL;
}

static void
collectGeometryKeys( GridProfile* profile, MapLayer* map_layer, GridCellKeyList& out_keys )
{
    for( unsigned int col = 0; col < profile->getNumColumns(); col++ )
    {
        for( unsigned int row = 0; row < profile->getNumRows(); row++ )
        {
            for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
            {
                MapLayerLevelOfDetail* level_def = i->get();
                GridCellKey key( col, row, level_def->getDepth(), profile );
                out_keys.push_back( key );
            }
        }
    }
}

static Task*
createTask( const GridCellKey& key, MapLayerCompiler* compiler )
{
    Task* task = NULL;

    MapLayerLevelOfDetail* def = getLodForKey( key, compiler->getMapLayer() );
    if ( def )
    {
        // construct a filter environment template to use for all tasks:
        osg::ref_ptr<FilterEnv> cell_env = compiler->getSession()->createFilterEnv();

        cell_env->setTerrainNode( compiler->getTerrainNode() );
        cell_env->setTerrainSRS( compiler->getTerrainSRS() );

        std::string abs_path = compiler->createAbsPathFromTemplate( "g" + key.toString() );

        GeoExtent extent = compiler->getMapLayer()->getAreaOfInterest().getSRS()->transform( key.getExtent() );

        cell_env->setInputSRS( def->getFeatureLayer()->getSRS() );
        cell_env->setExtent( extent );
        cell_env->setProperty( Property( "compiler.cell_id", key.toString() ) );

        task = new CellCompiler(
            key.toString(),
            abs_path,
            def->getFeatureLayer(),
            def->getFilterGraph(),
            def->getMinRange(),
            def->getMaxRange(),
            cell_env.get(),
            def->getResourcePackager()? def->getResourcePackager() : compiler->getResourcePackager(),
            compiler->getArchive(),
            def->getUserData() );

        osgGIS::notify( osg::INFO )
            << "Task: Key = " << key.toString() << ", LOD = " << key.getLevel() << ", Extent = " << extent.toString() 
            << " (w=" << extent.getWidth() << ", h=" << extent.getHeight() << ")"
            << std::endl;
    }

    return task;
}

/*****************************************************************************/

GriddedMapLayerCompiler::GriddedMapLayerCompiler( MapLayer* _layer, Session* _session )
: MapLayerCompiler( _layer, _session )
{
    //NOP
}

Profile*
GriddedMapLayerCompiler::createProfile()
{

    // figure out the bounds of the compilation area and create a Q map. We want a sqaure AOI..maybe
    GeoExtent aoi = map_layer->getAreaOfInterest();

    // determine the working extent:
    //GeoExtent aoi = getAreaOfInterest( layer );
    if ( aoi.isInfinite() || !aoi.isValid() )
    {
        osg::ref_ptr<SpatialReference> geo_srs = osgGIS::Registry::SRSFactory()->createWGS84();
        //const SpatialReference* geo_srs = aoi.getSRS()->getGeographicSRS();

        aoi = GeoExtent(
            GeoPoint( -180.0, -90.0, geo_srs.get() ),
            GeoPoint(  180.0,  90.0, geo_srs.get() ) );
    }

    if ( !aoi.isValid() )
    {
        osgGIS::notify( osg::WARN ) << "Invalid area of interest in the map layer - no data?" << std::endl;
        return NULL;
    }

    osgGIS::notice()
        << "GRID: " << std::endl
        << "   Extent = " << aoi.toString() << ", w=" << aoi.getWidth() << ", h=" << aoi.getHeight() << std::endl
        << std::endl;

    GridProfile* profile = new GridProfile( aoi );
    profile->initByCellSize( map_layer->getCellWidth(), map_layer->getCellHeight() );    

    return profile;
}

CellCursor*
GriddedMapLayerCompiler::createCellCursor( Profile* _profile )
{
    return new GridCellCursor( dynamic_cast<GridProfile*>( _profile ), this );
}

unsigned int
GriddedMapLayerCompiler::queueTasks( Profile* _profile, TaskManager* task_man )
{
    GridProfile* profile = dynamic_cast<GridProfile*>( _profile );
    if ( profile )
    {
        // Now, build the index and collect the list of keys for which to compile data.
        GridCellKeyList keys;
        collectGeometryKeys( profile, map_layer.get(), keys );

        // make a build task for each quad cell we collected:
        //int total_tasks = keys.size();
        for( GridCellKeyList::iterator i = keys.begin(); i != keys.end(); i++ )
        {
            task_man->queueTask( createTask( *i, this ) );
        }

        return keys.size();
    }
    else
    {
        return 0;
    }
}

// builds and writes all the index nodes.
void
GriddedMapLayerCompiler::buildIndex( Profile* _profile, osg::Group* scene_graph )
{
    GridProfile* profile = dynamic_cast<GridProfile*>( _profile );
    if ( !profile ) return;

    osgGIS::notice() << "Rebuilding index..." << std::endl;

    // first, determine the SRS of the output scene graph so that we can
    // make pagedlod/lod centroids.
    SpatialReference* output_srs = map_layer->getOutputSRS( getSession(), getTerrainSRS() );

    osg::ref_ptr<SmartReadCallback> reader = new SmartReadCallback();

    for( unsigned int col = 0; col < profile->getNumColumns(); col++ )
    {
        for( unsigned int row = 0; row < profile->getNumRows(); row++ )
        {
            osg::PagedLOD* plod = NULL;
            osg::LOD*      proxy_lod = NULL;

            // nesting vs. serial is unimportant in terms of LOD for gridded.
            unsigned int level = 0;
            for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++, level++ )
            {
                MapLayerLevelOfDetail* def = i->get();

                GridCellKey key( col, row, level, profile );
                std::string path = createAbsPathFromTemplate( "g" + key.toString() );
                if ( osgDB::fileExists( path ) )
                {
                    if ( getPaged() )
                    {
                        if ( !plod )
                        {
                            plod = new osg::PagedLOD();
                            plod->setName( key.toString() );
                        }
                        unsigned int next_child = plod->getNumFileNames();
                        plod->setFileName( next_child, createRelPathFromTemplate( "g" + key.toString() ) );
                        plod->setRange( next_child, def->getMinRange(), def->getMaxRange() );
                    }
                    else
                    {
                        if ( !proxy_lod )
                        {
                            proxy_lod = new osg::LOD();
                            proxy_lod->setName( key.toString() );
                        }

                        osg::ProxyNode* proxy = new osg::ProxyNode();
                        proxy->setFileName( 0, createRelPathFromTemplate( "g" + key.toString() ) );
                        proxy_lod->addChild( proxy, def->getMinRange(), def->getMaxRange() );
                    }
                }
            }
            
            if ( plod )
            {
                GridCellKey key( col, row, 0, profile );
                setCenterAndRadius( plod, key.getExtent(), reader.get() );
                scene_graph->addChild( plod );
            }
            else if ( proxy_lod )
            {
                GridCellKey key( col, row, 0, profile );
                setCenterAndRadius( proxy_lod, key.getExtent(), reader.get() );
                scene_graph->addChild( proxy_lod );
            }
                
                //osg::Geode* geode = new osg::Geode();
                //osg::Vec3d p = plod->getCenter();
                //osg::Vec3d n = p; n.normalize();
                //osg::Geometry* g3 = new osg::Geometry();
                //osg::Vec3Array* v3 = new osg::Vec3Array(2);
                //(*v3)[0] = p; //osg::Vec3d(0,0,0);
                //(*v3)[1] = p + n * 100000.0;
                //g3->setVertexArray( v3 );
                //osg::Vec4Array* c3 = new osg::Vec4Array(1);
                //(*c3)[0].set(1,1,0,1);
                //g3->setColorArray( c3 );
                //g3->setColorBinding(osg::Geometry::BIND_OVERALL);
                //g3->addPrimitiveSet( new osg::DrawArrays(GL_LINES, 0, 2) );
                //g3->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
                //geode->addDrawable( g3 );

                //osgText::Text* t = new osgText::Text();
                //t->setText( key.toString() );
                //t->setPosition( (*v3)[1] );
                //t->setCharacterSizeMode( osgText::TextBase::SCREEN_COORDS );
                //t->setCharacterSize( 72.0f );
                //t->setAutoRotateToScreen( true );
                //t->setColor( osg::Vec4f(1,0,0,1) );
                //geode->addDrawable( t );

                //scene_graph->addChild( geode );
            //}
        }
    }
}
