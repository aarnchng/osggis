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

#include <osgGISProjects/MapLayerCompiler>
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
#include <sstream>

using namespace osgGIS;
using namespace osgGISProjects;
using namespace OpenThreads;

#define MY_PRIORITY_SCALE 1.0f //0.5f

/*****************************************************************************/

MapLayerCompiler::CellCompiler::CellCompiler(const std::string& _abs_output_uri,
                                             FeatureLayer*      layer,
                                             FilterGraph*       graph,
                                             float              min_range,
                                             float              max_range,
                                             FilterEnv*         env,
                                             ResourcePackager*  _packager,
                                             osgDB::Archive*    _archive )
 : FeatureLayerCompiler( _abs_output_uri, layer, graph, env ),
   packager( _packager ),
   abs_output_uri( _abs_output_uri ),
   archive( _archive )
{
    //TODO: maybe the FilterEnv should just have one of these by default.
    SmartReadCallback* smart = new SmartReadCallback();
    smart->setMinRange( min_range );
    env->setTerrainReadCallback( smart );
}

void
MapLayerCompiler::CellCompiler::run() // overrides FeatureLayerCompiler::run()
{
    // first check to see whether this cell needs compiling:
    need_to_compile = archive.valid() || !osgDB::fileExists( abs_output_uri );

    if ( need_to_compile )
    {
        // Compile the cell:
        FeatureLayerCompiler::run();

        // Write the resulting node graph to disk, first ensuring that the output folder exists:
        // TODO: consider whether this belongs in the runSynchronousPostProcess() method
        if ( getResult().isOK() && getResultNode() )
        {
            has_drawables = GeomUtils::hasDrawables( getResultNode() );
        }
    }
    else
    {
        result = FilterGraphResult::ok();
        has_drawables = true;
    }
}

void
MapLayerCompiler::CellCompiler::runSynchronousPostProcess( Report* report )
{
    if ( need_to_compile )
    {
        if ( !getResult().isOK() )
        {
            osgGIS::notice() << getName() << " failed to compile: " << getResult().getMessage() << std::endl;
            return;
        }
        
        if ( !getResultNode() || !has_drawables )
        {
            osgGIS::info() << getName() << " resulted in no geometry" << std::endl;
            return;
        }

        if ( packager.valid() )
        {
            // TODO: we should probably combine the following two calls into one:

			// update any texture/model refs in preparation for packaging:
			packager->rewriteResourceReferences( getResultNode() );

			// copy resources to their final destination
			packager->packageResources( env->getResourceCache(), report );

			// write the node data itself
			if ( !packager->packageNode( getResultNode(), abs_output_uri ) )
			{
                osgGIS::warn() << getName() << " failed to package node to output location" << std::endl;
				result = FilterGraphResult::error( "Cell built OK, but failed to deploy to disk/archive" );
			}
		}
	}
}

/*****************************************************************************/

MapLayerCompiler::MapLayerCompiler( MapLayer* _layer, Session* _session )
{
    map_layer = _layer;
    session   = _session;
    paged     = true;
}

MapLayer*
MapLayerCompiler::getMapLayer() const {
    return map_layer.get();
}

void
MapLayerCompiler::setPaged( bool value )
{
    if ( paged != value )
    {
        paged = value;
        scene_graph = NULL; // invalidate it
    }
}

bool
MapLayerCompiler::getPaged() const {
    return paged;
}

void
MapLayerCompiler::setAbsoluteOutputURI( const std::string& value ) {
    output_uri = value;
}

void
MapLayerCompiler::setTerrain(osg::Node*              _terrain,
                             const SpatialReference* _terrain_srs,
                             const GeoExtent&        _terrain_extent )
{
    terrain_node   = _terrain;
    terrain_srs    = (SpatialReference*)_terrain_srs;
    terrain_extent = _terrain_extent;
}


void
MapLayerCompiler::setTerrain(osg::Node*              _terrain,
                             const SpatialReference* _terrain_srs )
{
    setTerrain( _terrain, _terrain_srs, GeoExtent::infinite() );
}

osg::Node*
MapLayerCompiler::getTerrainNode()
{
    return terrain_node.get();
}

SpatialReference* 
MapLayerCompiler::getTerrainSRS() const
{
    return terrain_srs.get();
}

const GeoExtent& 
MapLayerCompiler::getTerrainExtent() const
{
    return terrain_extent;
}

void
MapLayerCompiler::setArchive( osgDB::Archive* _archive, const std::string& _filename )
{
    archive = _archive;
    archive_filename = _filename;
}

osgDB::Archive*
MapLayerCompiler::getArchive() 
{
    return archive.get();
}

const std::string&
MapLayerCompiler::getArchiveFileName() const
{
    return archive_filename;
}

void
MapLayerCompiler::setResourcePackager( ResourcePackager* value ) {
    resource_packager = value;
}

ResourcePackager*
MapLayerCompiler::getResourcePackager() const {
    return resource_packager.get();
}

osg::Node*
MapLayerCompiler::getSceneGraph()
{
    return scene_graph.get();
}

Session*
MapLayerCompiler::getSession()
{
    if ( !session.valid() )
    {
        session = new Session();
    }
    return session.get();
}

std::string
MapLayerCompiler::createAbsPathFromTemplate( const std::string& core )
{
    return PathUtils::combinePaths(
        osgDB::getFilePath( output_uri ),
        core + "." + osgDB::getLowerCaseFileExtension( output_uri ) );
}

std::string
MapLayerCompiler::createRelPathFromTemplate( const std::string& core )
{
    return core + "." + osgDB::getLowerCaseFileExtension( output_uri );
}

void
MapLayerCompiler::setCenterAndRadius( osg::PagedLOD* plod, const GeoExtent& cell_extent, SmartReadCallback* reader )
{
    SpatialReference* srs = map_layer->getOutputSRS( getSession(), getTerrainSRS() );
    // first get the output srs centroid: 
    GeoPoint centroid = srs->transform( cell_extent.getCentroid() );
    GeoPoint sw = srs->transform( cell_extent.getSouthwest() );

    double radius = (centroid-sw).length();
    
    if ( terrain_node.valid() && terrain_srs.valid() )
    {
        centroid = GeomUtils::clampToTerrain( centroid, terrain_node.get(), terrain_srs.get(), reader );
    }

    plod->setCenter( centroid );
    plod->setRadius( radius );
}

bool
MapLayerCompiler::compile( TaskManager* my_task_man )
{
    // make a task manager to run the compilation:
    osg::ref_ptr<TaskManager> task_man = my_task_man;
    if ( !task_man.valid() )
        task_man = new TaskManager( 0 );

    // make a profile describing this compilation setup:
    osg::ref_ptr<Profile> profile = createProfile();

    // create and queue up all the tasks to run:
    unsigned int total_tasks = queueTasks( profile.get(), task_man.get() );

    // configure the packager so that skins and model end up in the right place:
    if ( resource_packager.valid() )
    {
        resource_packager->setArchive( getArchive() );
        resource_packager->setOutputLocation( osgDB::getFilePath( output_uri ) );
    }

    osg::ref_ptr<Report> default_report = new Report();

    // run until we're done.
    //
    // TODO: in the future, we can change the semantics so that you call compileSome() over and over
    //       until everything is complete...and compileSome() will return a list of everything that
    //       was started and everything that completed during that pass. This will allow for
    //       intermediate processing for UI's etc.

    osg::Timer_t start_time = osg::Timer::instance()->tick();
    int tasks_completed = 0;

    while( task_man->wait( 1000L ) )
    {
        osg::ref_ptr<Task> completed_task = task_man->getNextCompletedTask();
        if ( completed_task.valid() )
        {
            tasks_completed++;

            CellCompiler* compiler = reinterpret_cast<CellCompiler*>( completed_task.get() );
            if ( compiler->getResult().isOK() )
            {
                compiler->runSynchronousPostProcess( default_report.get() );

                // give the layer compiler an opportunity to do something:
                processCompletedTask( compiler );

                // progress
                // TODO: get rid of this.. replace with a callback or with the user calling
                // compileMore() multiple times.
                osg::Timer_t now = osg::Timer::instance()->tick();
                float p = 100.0f * (float)tasks_completed/(float)total_tasks;
                float elapsed = osg::Timer::instance()->delta_s( start_time, now );
                float avg_task_time = elapsed/(float)tasks_completed;
                float time_remaining = ((float)total_tasks-(float)tasks_completed)*avg_task_time;

                unsigned int hrs,mins,secs;
                TimeUtils::getHMSDuration( time_remaining, hrs, mins, secs );

                char buf[10];
                sprintf( buf, "%02d:%02d:%02d", hrs, mins, secs );
                osgGIS::notify(osg::NOTICE) << tasks_completed << "/" << total_tasks 
                    << " tasks (" << (int)p << "%) complete, " 
                    << buf << " remaining" << std::endl;
            }
            else
            {
                //TODO: replace this with Report facility
                osgGIS::notify( osg::WARN ) << "ERROR: compilation of cell "
                    << compiler->getName() << " failed : "
                    << compiler->getResult().getMessage()
                    << std::endl;
            }
        }
    }

    // build the index that will point to the keys:
    buildIndex( profile.get() );

    if ( getSceneGraph() )
    {
        // finally, resolve any references at the root level and optimize the root graph. (deprecated)
        //if ( resource_packager.valid() )
        //{
        //    resource_packager->rewriteResourceReferences( getSceneGraph() );
        //}

        osgUtil::Optimizer opt;
        opt.optimize( getSceneGraph(), 
            osgUtil::Optimizer::SPATIALIZE_GROUPS |
            osgUtil::Optimizer::STATIC_OBJECT_DETECTION |
            osgUtil::Optimizer::SHARE_DUPLICATE_STATE );
    }
    
    osg::Timer_t end_time = osg::Timer::instance()->tick();
    double duration_s = osg::Timer::instance()->delta_s( start_time, end_time );

    osgGIS::notify( osg::NOTICE )
        << "Compilation finished, total time = " << duration_s << " seconds"
        << std::endl;

    return true;
}
