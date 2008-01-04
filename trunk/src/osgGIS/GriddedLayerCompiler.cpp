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
#include <osgGIS/Compiler>
#include <osgGIS/FadeHelper>
#include <osgGIS/Task>
#include <osgGIS/TaskManager>
#include <osg/Node>
#include <osg/LOD>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/Group>
#include <osg/Timer>
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
        s << "Cell x" << col << ",y" << row;
        setName( s.str() );

        read_cb = new SmartReadCallback();
    }

    void run()
    {
        std::stringstream str;

        std::string output_dir = osgDB::getFilePath( output_file );
        std::string output_prefix = osgDB::getStrippedName( output_file );
        std::string output_extension = osgDB::getFileExtension( output_file );

        //str.clear();
        //str << "Starting row=" << row << " col=" << col << ", extent=" << tile_extent.toString() << "... " << std::endl;
        //report( str.str() );
            
        float min_range = FLT_MAX, max_range = FLT_MIN;
        osg::ref_ptr<osg::LOD> lod = new osg::LOD();
        LayerCompiler::FilterGraphRangeList& graphs = compiler.getFilterGraphs();

        for( LayerCompiler::FilterGraphRangeList::iterator i = graphs.begin(); i != graphs.end(); i++ )
        {
            osg::Node* range = compileLOD( i->graph.get() );
            if ( range )
            {
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

            if ( compiler.getArchive() )
            {
                compiler.getArchive()->writeNode( *(lod.get()), tile_filename );
            }
            else
            {
                std::string tile_path = osgDB::concatPaths( output_dir, tile_filename );
                osgDB::writeNodeFile( *(lod.get()), tile_path );
            }

            //str.clear();
            //str << "  Completed row=" << row << " col=" << col << " @ " << tile_filename << std::endl;
            //report( str.str() );

            result = plod;
            //root->addChild( plod );
        }
        else
        {
            //root->addChild( lod.get() );
            result = lod.get();
        }

        //str.clear();
        //str << "  Done; hits = " << read_cb->getMruHitRatio() << std::endl;
        //report( str.str() );
    }

public:
    osg::Node* getResult()
    {
        return result.get();
    }

private:
    osg::Node* compileLOD( FilterGraph* graph )
    {
        osg::ref_ptr<FilterEnv> env = new FilterEnv();
        env->setExtent( tile_extent );
        env->setTerrainNode( compiler.getTerrainNode() );
        env->setTerrainSRS( compiler.getTerrainSRS() );
        env->setTerrainReadCallback( read_cb.get() );
        Compiler compiler( layer.get(), graph, session.get() );
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
    GeoExtent extent = layer->getExtent();
    if ( extent.isInfinite() || !extent.isValid() )
    {
        const SpatialReference* srs = layer->getSRS()->getBasisSRS();

        extent = GeoExtent(
            GeoPoint( -180.0, -90.0, srs ),
            GeoPoint(  180.0,  90.0, srs ) );
    }

    if ( getFadeLODs() )
    {
        FadeHelper::enableFading( root->getOrCreateStateSet() );
    }

    if ( getRenderBinNumber() != INT_MAX )
    {
        const std::string& bin_name = root->getOrCreateStateSet()->getBinName();
        root->getOrCreateStateSet()->setRenderBinDetails( getRenderBinNumber(), bin_name );
    }

    if ( getSession() && getPreCompileExpr().length() > 0 )
    {
        getSession()->createScriptEngine()->run( new Script( getPreCompileExpr() ) );
    }
    
    if ( extent.isValid() )
    {
        const GeoPoint& sw = extent.getSouthwest();
        const GeoPoint& ne = extent.getNortheast();
        osg::ref_ptr<SpatialReference> srs = extent.getSRS();

        double dx = (ne.x() - sw.x()) / num_cols;
        double dy = (ne.y() - sw.y()) / num_rows;

        // Queue each tile as a parallelized task
        for( unsigned int col = 0; col < num_cols; col++ )
        {
            for( unsigned int row = 0; row < num_rows; row++ )
            {
                GeoExtent sub_extent(
                    GeoPoint( sw.x() + (double)col*dx, sw.y() + (double)row*dy, srs.get() ),
                    GeoPoint( sw.x() + (double)(col+1)*dx, sw.y() + (double)(row+1)*dy, srs.get() ) );

                
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
                    osg::notify(osg::NOTICE) << task->getName() << ": building..." << std::flush;

                    task->run();
                    if ( task->getResult() )
                    {
                        root->addChild( task->getResult() );
                    }

                    osg::notify(osg::NOTICE) << "completed" << std::endl;
                }
            }
        }

        // block until tasks are completed, and add the resulting nodes to the main graph.
        if ( getTaskManager() )
        {
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
                    }
                }        
            }
        }
    }

    // finally we organize the root graph better
    osgUtil::Optimizer opt;
    opt.optimize( root, osgUtil::Optimizer::SPATIALIZE_GROUPS );
    
    osg::Timer_t end = osg::Timer::instance()->tick();

    osg::notify(osg::FATAL) 
        << "[GriddedLayerCompiler] total time = " 
        << osg::Timer::instance()->delta_s( start, end )
        << "s" << std::endl;

    return root;
}

