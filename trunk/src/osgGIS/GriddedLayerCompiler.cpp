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

#include <osgGIS/GriddedLayerCompiler>
#include <osgGIS/GeoExtent>
#include <osgGIS/SceneGraphCompiler>
#include <osgGIS/FadeHelper>
#include <osgGIS/Task>
#include <osgGIS/TaskManager>
#include <osgGIS/Units>
#include <osgGIS/Utils>
#include <osg/Node>
#include <osg/LOD>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/Group>
#include <osg/Timer>
#include <osg/Depth>
#include <osgUtil/Optimizer>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <sstream>

using namespace osgGIS;


GriddedLayerCompiler::GriddedLayerCompiler()                            
{
    num_rows = 10;
    num_cols = 10;
    row_size = 0.0;
    col_size = 0.0;
    paged = false;
}


GriddedLayerCompiler::~GriddedLayerCompiler()
{
    //NOP
}

void
GriddedLayerCompiler::setNumRows( int value )
{
    num_rows = value;
}

int
GriddedLayerCompiler::getNumRows() const
{
    return num_rows;
}

void
GriddedLayerCompiler::setNumColumns( int value )
{
    num_cols = value;
}

int
GriddedLayerCompiler::getNumColumns() const
{
    return num_cols;
}

void 
GriddedLayerCompiler::setRowSize( double value )
{
    row_size = value;
}

double 
GriddedLayerCompiler::getRowSize() const
{
    return row_size;
}

void 
GriddedLayerCompiler::setColumnSize( double value )
{
    col_size = value;
}

double 
GriddedLayerCompiler::getColumnSize() const
{
    return col_size;
}


Properties
GriddedLayerCompiler::getProperties()
{
    Properties props = LayerCompiler::getProperties();
    //TODO - populate!
    return props;
}

void
GriddedLayerCompiler::setProperties( Properties& input )
{
    setNumRows( input.getIntValue( "num_rows", getNumRows() ) );
    setNumColumns( input.getIntValue( "num_cols", getNumColumns() ) );
    setNumColumns( input.getIntValue( "num_columns", getNumColumns() ) );
    setRowSize( input.getDoubleValue( "row_size", getRowSize() ) );
    setColumnSize( input.getDoubleValue( "col_size", getColumnSize() ) );
    setColumnSize( input.getDoubleValue( "column_size", getColumnSize() ) );
    LayerCompiler::setProperties( input );
}


osg::Node*
GriddedLayerCompiler::compile( FeatureLayer* layer )
{
    return compile( layer, "" );
}


static OpenThreads::Mutex report_mutex;

static void
report( const std::string& str )
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> sl(report_mutex);
    osg::notify(osg::NOTICE) << str;
}


class CompileTileTask : public Task
{
public:
    CompileTileTask(int                _row, 
                    int                _col,
                    const GeoExtent&   _extent, 
                    FeatureLayer*      _layer,
                    Session*           _session,
                    const std::string& _output_file,
                    LayerCompiler&     _compiler ) :
      row(_row),
      col(_col),
      tile_extent( _extent ),
      layer( _layer ),
      session( _session ),
      output_file( _output_file ),
      compiler( _compiler )
    {
        std::stringstream s;
        s << "x" << col << ",y" << row;
//        s << "Cell x" << col << ",y" << row << " EXT: (" << tile_extent.toString() << ") CEN: (" << tile_extent.getCentroid().toString() << ")";
        setName( s.str() );

        read_cb = new SmartReadCallback();
    }

    void run()
    {
        std::stringstream str;

        std::string output_dir = osgDB::getFilePath( output_file );
        std::string output_prefix = osgDB::getStrippedName( output_file );
        std::string output_extension = osgDB::getFileExtension( output_file );
            
        float min_range = FLT_MAX, max_range = FLT_MIN;
        osg::ref_ptr<osg::LOD> lod = new osg::LOD();
        lod->setName( getName() );
        LayerCompiler::FilterGraphRangeList& graphs = compiler.getFilterGraphs();

        for( LayerCompiler::FilterGraphRangeList::iterator i = graphs.begin(); i != graphs.end(); i++ )
        {
            osg::Node* range = compileLOD( i->graph.get() );
            if ( range )
            {
                range->setName( i->graph->getName() );
                lod->addChild( range, i->min_range, i->max_range );
                if ( i->min_range < min_range ) min_range = i->min_range;
                if ( i->max_range > max_range ) max_range = i->max_range;

                //TODO: need to set centroid and cluster culling
                
                if ( compiler.getFadeLODs() )
                {
                    FadeHelper::setOuterFadeDistance( i->max_range, range->getOrCreateStateSet() );
                    FadeHelper::setInnerFadeDistance( (float)(i->max_range - .2*(i->max_range - i->min_range)), range->getOrCreateStateSet() );
                }
            }
        }


        if ( GeomUtils::getNumGeodes( lod.get() ) > 0 )
        {
            if ( compiler.getPaged() )
            {
                osg::PagedLOD* plod = new osg::PagedLOD();
                std::stringstream str;
                str << output_prefix << "_x" << col << "_y" << row  << "." << output_extension;
                std::string tile_filename = str.str();
                plod->setFileName( 0, tile_filename );
                plod->setRange( 0, min_range, max_range );
                plod->setCenter( lod->getBound().center() );
                plod->setRadius( lod->getBound().radius() );
                
                compiler.localizeResourceReferences( lod.get() );

                if ( compiler.getArchive() )
                {
                    // archive has a serializer mutex, so this is ok:
                    compiler.getArchive()->writeNode( 
                        *(lod.get()), 
                        tile_filename,
                        osgDB::Registry::instance()->getOptions() );
                }
                else
                {
                    std::string tile_path = osgDB::concatPaths( output_dir, tile_filename );
                    osgDB::writeNodeFile(
                        *(lod.get()),
                        tile_path,
                        osgDB::Registry::instance()->getOptions() );
                }

                result = plod;
            }
            else
            {
                result = lod.get();
            }
        }
        else
        {
            result = NULL;
        }
    }

public:
    osg::Node* getResult()
    {
        return result.get();
    }

private:
    osg::Node* compileLOD( FilterGraph* graph )
    {
        osg::ref_ptr<FilterEnv> env = session->createFilterEnv();
        env->setProperty( Property( "compiler.grid_row", row ) );
        env->setProperty( Property( "compiler.grid_col", col ) );
        env->setExtent( tile_extent );
        env->setTerrainNode( compiler.getTerrainNode() );
        env->setTerrainSRS( compiler.getTerrainSRS() );
        env->setTerrainReadCallback( read_cb.get() );
        SceneGraphCompiler compiler( layer.get(), graph );
        return compiler.compile( env.get() );
    }

private:
    int row, col;
    osg::ref_ptr<SmartReadCallback> read_cb;
    GeoExtent tile_extent;
    osg::ref_ptr<FeatureLayer> layer;
    osg::ref_ptr<Session> session;
    std::string output_file;
    LayerCompiler& compiler;
    osg::ref_ptr<osg::Node> result;
};


osg::Node*
GriddedLayerCompiler::compile( FeatureLayer* layer, const std::string& output_file )
{
    osg::Timer_t start = osg::Timer::instance()->tick();

    osg::Group* root = new osg::Group();

    // determine the working extent:
    GeoExtent aoi = getAreaOfInterest( layer );
    if ( aoi.isInfinite() || !aoi.isValid() )
    {
        const SpatialReference* srs = layer->getSRS()->getGeographicSRS();

        aoi = GeoExtent(
            GeoPoint( -180.0, -90.0, srs ),
            GeoPoint(  180.0,  90.0, srs ) );
    }

    if ( getFadeLODs() )
    {
        FadeHelper::enableFading( root->getOrCreateStateSet() );
    }

    if ( getSession() && getPreCompileExpr().length() > 0 )
    {
        //TODO: new Script() is a memory leak in the following line!!
        getSession()->createScriptEngine()->run( new Script( getPreCompileExpr() ) );
    }
    
    if ( aoi.isValid() )
    {
        const GeoPoint& sw = aoi.getSouthwest();
        const GeoPoint& ne = aoi.getNortheast();
        osg::ref_ptr<SpatialReference> srs = aoi.getSRS();

        double dx, dy;
        double last_dx, last_dy;

        if ( row_size > 0.0 )
        {
            //if ( aoi.getSRS()->isGeographic() )
            //{
            //    osg::Vec2d vec( 0, row_size );
            //    osg::Vec2d centroid( aoi.getCentroid().x(), aoi.getCentroid().y() );
            //    osg::Vec2d output;
            //    Units::convertLinearToAngularVector( vec, Units::METERS, Units::DEGREES, centroid, output );
            //    row_size = output.y();
            //}

            num_rows = (int) ::ceil( aoi.getHeight()/row_size );
            dy = row_size;
            last_dy = ::fmod( aoi.getHeight(), row_size );
            if ( last_dy == 0.0 )
                last_dy = dy;
        }
        else
        {
            dy = aoi.getHeight() / std::max( num_rows, 1 );
            last_dy = dy;
        }

        if ( col_size > 0.0 )
        {
            //if ( aoi.getSRS()->isGeographic() )
            //{
            //    osg::Vec2d vec( col_size, 0 );
            //    osg::Vec2d centroid( aoi.getCentroid().x(), aoi.getCentroid().y() );
            //    osg::Vec2d output;
            //    Units::convertLinearToAngularVector( vec, Units::METERS, Units::DEGREES, centroid, output );
            //    row_size = output.x();
            //}

            num_cols = (int) ::ceil( aoi.getWidth()/col_size );
            dx = col_size;
            last_dx = ::fmod( aoi.getWidth(), col_size );
            if ( last_dx == 0.0 )
                last_dx = dx;
        }
        else
        {
            dx = aoi.getWidth() / std::max( num_cols, 1 );
            last_dx = dx;
        }

        // Queue each tile as a parallelized task
        for( unsigned int col = 0; col < num_cols; col++ )
        {
            double col_width = col+1 < num_cols? dx : last_dx;

            for( unsigned int row = 0; row < num_rows; row++ )
            {
                double row_height = row+1 < num_rows? dy : last_dy;

                GeoExtent sub_extent(
                    sw.x() + (double)col*dx, sw.y() + (double)row*dy,
                    sw.x() + (double)col*dx + col_width, sw.y() + (double)row*dy + row_height,
                    srs.get() );
                
                osg::ref_ptr<CompileTileTask> task = new CompileTileTask( 
                        row, col,
                        sub_extent, 
                        layer, 
                        getSession(),
                        output_file, 
                        *this );

                if ( getTaskManager() )
                {
                    getTaskManager()->queueTask( task.get() );
                }
                else
                {

                    osg::notify(osg::NOTICE) 
                        << task->getName() << " starting... " 
                        << sub_extent.toString() << "..."
                        << std::flush;

                    task->run();
                    if ( task->getResult() )
                    {
                        root->addChild( task->getResult() );
                        localizeResources( osgDB::getFilePath( output_file ) );
                    }

                    osg::notify(osg::NOTICE) << "completed" << std::endl;
                }
            }
        }

        // block until tasks are completed, and add the resulting nodes to the main graph.
        if ( getTaskManager() )
        {
            unsigned int total = num_rows * num_cols;
            unsigned int count = 0;

            while( getTaskManager()->wait() )
            {
                osg::ref_ptr<Task> completed_task = getTaskManager()->getNextCompletedTask();
                if ( completed_task.valid() )
                {
                    CompileTileTask* task = dynamic_cast<CompileTileTask*>( completed_task.get() );
                    osg::Node* node = task->getResult();
                    if ( node )
                    {
                        root->addChild( node );

                        // need to do this each time a task completes so that we don't collect
                        // gobs of resources in memory
                        localizeResources( osgDB::getFilePath( output_file ) );
                    }   

                    int perc = (int)(100.0f * ((float)++count)/(float)total);
                    osg::notify(osg::NOTICE) << "..." << perc << "% done" << std::endl << std::flush;
                } 
            }
        }
    }

    // write any textures to the archive:
    localizeResourceReferences( root );

    // finally we organize the root graph better
    osgUtil::Optimizer opt;
    opt.optimize( root, osgUtil::Optimizer::SPATIALIZE_GROUPS );
    
    if ( getRenderOrder() >= 0 )
    {
        const std::string& bin_name = root->getOrCreateStateSet()->getBinName();
        root->getOrCreateStateSet()->setRenderBinDetails( getRenderOrder(), bin_name );
        root->getOrCreateStateSet()->setAttributeAndModes( new osg::Depth( osg::Depth::ALWAYS ), osg::StateAttribute::ON );
    }
    
    osg::Timer_t end = osg::Timer::instance()->tick();

    osg::notify(osg::FATAL) 
        << "[GriddedLayerCompiler] total time = " 
        << osg::Timer::instance()->delta_s( start, end )
        << "s" << std::endl;

    return root;
}

