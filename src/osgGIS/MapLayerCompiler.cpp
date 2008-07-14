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

#include <osgGIS/MapLayerCompiler>
#include <osgGIS/FeatureLayerCompiler>
#include <osgGIS/Session>
#include <osgGIS/Utils>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <sstream>

using namespace osgGIS;
using namespace OpenThreads;

/*****************************************************************************/
/* (internal)
 * Extends the basic compile task in order to track the parent node in the skeleton
 * scene graph to which the compiled cell belongs.
 */
class CellCompiler : public FeatureLayerCompiler
{
public:
    CellCompiler(const std::string& name, FeatureLayer* layer, FilterGraph* graph, FilterEnv* env )
        : FeatureLayerCompiler( name, layer, graph, env )
    {
        //NOP
    }

    virtual void run()
    {
        env->setTerrainReadCallback( new SmartReadCallback() );
        FeatureLayerCompiler::run();
    }

    virtual void runSynchronousPostProcess() { }
};

/*****************************************************************************/
/* (internal)
 * Extends the cell compiler to attach the resulting node to the parent after compilation.
 */
class NonPagedCellCompiler : public CellCompiler
{
public:
    NonPagedCellCompiler( const std::string& name, FeatureLayer* layer, FilterGraph* graph, FilterEnv* env, osg::Node* _placeholder )
        : CellCompiler( name, layer, graph, env ),
          placeholder( _placeholder )
    {
        //NOP
    }

    virtual void run()
    {
        // compile the cell:
        CellCompiler::run();
    }
    
    virtual void runSynchronousPostProcess()
    {
        // attach the result to its parent.
        // TODO: instead of doing this, we probably want to have a "postRun" method that gets called from
        // the main thread .. in case the user it doing this at runtime.
        if ( getResult().isOK() && placeholder.valid() && placeholder->getNumParents() > 0 )
        {
            placeholder->getParent(0)->replaceChild( placeholder.get(), getResultNode() );
        }
    }

private:
    osg::ref_ptr<osg::Node> placeholder;
};

/*****************************************************************************/
/* (internal)
 * Extends the cell compiler task to write the output to a paging file after compilation.
 */
class PagedCellCompiler : public CellCompiler
{
public:
    PagedCellCompiler(const std::string& _abs_output_uri,
                      FeatureLayer*      layer,
                      FilterGraph*       graph,
                      FilterEnv*         env,
                      osg::PagedLOD*     _parent,
                      osgDB::Archive*    _archive =NULL )
        : CellCompiler( _abs_output_uri, layer, graph, env ),
          parent( _parent ),
          abs_output_uri( _abs_output_uri ),
          archive( _archive )
    {
        //NOP
    }

public:
    virtual void run() // overrides FeatureLayerCompiler::run()
    {
        // Compile the cell:
        CellCompiler::run();

        // Write the resulting node graph to disk, first ensuring that the output folder exists:
        // TODO: consider whether this belongs in the runSynchronousPostProcess() method
        if ( getResult().isOK() && getResultNode() )
        {
            if ( osgDB::makeDirectoryForFile( abs_output_uri ) )
            {
                if ( archive.valid() )
                {
                    //todo
                }
                else if ( !osgDB::writeNodeFile( *getResultNode(), abs_output_uri ) )
                {
                    result = FilterGraphResult::error( "Cell built OK, but failed to write to disk" );
                }
            }
        }
    }

    virtual void runSynchronousPostProcess()
    {
        if ( getResultNode() )
        {
            osg::BoundingSphere bs = getResultNode()->getBound();
            if ( parent->getRadius() > 0.0 )
            {
                bs.expandBy( osg::BoundingSphere( parent->getCenter(), parent->getRadius() ) );
            }
            parent->setCenter( bs.center() );
            parent->setRadius( bs.radius() );
        }
    }

private:
    std::string abs_output_uri;
    osg::ref_ptr<osg::PagedLOD> parent;
    osg::ref_ptr<osgDB::Archive> archive;
};


/*****************************************************************************/

MapLayerCompiler::MapLayerCompiler( MapLayer* _layer, Session* _session )
{
    map_layer   = _layer;
    session     = _session;
    paged       = false;
    output_uri  = "";
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

osg::Node*
MapLayerCompiler::getSceneGraph()
{
    if ( !scene_graph.valid() )
    {
        buildSceneGraphSkeleton();
    }
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

void
MapLayerCompiler::buildSceneGraphSkeleton()
{
    // first, determine the SRS of the output scene graph so that we can
    // make pagedlod/lod centroids.
    SpatialReference* output_srs = map_layer->getOutputSRS( getSession(), getTerrainSRS() );

    // start a brand new scene graph:
    scene_graph = new osg::Group();

    for( unsigned int row = 0; row < map_layer->getNumCellRows(); row++ )
    {
        for( unsigned int col = 0; col < map_layer->getNumCellColumns(); col++ )
        {
            std::stringstream name_builder;
            name_builder << "X" << col << "Y" << row;

            if ( getPaged() )
            {
                osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD();
                plod->setName( name_builder.str() );

                unsigned int next = 0;
                for( unsigned int level = 0; level < map_layer->getNumCellLevels(); level++ )
                {
                    osg::ref_ptr<MapLayerCell> cell = map_layer->createCell( col, row, level );
                    if ( cell.valid() )
                    {
                        std::string cell_uri = cell->makeURIFromTemplate( output_uri );
                        plod->setFileName( next, osgDB::getSimpleFileName( cell_uri ) );
                        plod->setRange( next, cell->getDefinition()->getMinRange(), cell->getDefinition()->getMaxRange() );
                        next++;

                        // calculate and store the PagedLOD centroid/radius
                        osg::BoundingSphere bs = cell->getBoundingSphere( output_srs );
                        plod->setCenter( bs.center() );
                        plod->setRadius( bs.radius() );
                    }
                }

                if ( plod->getNumFileNames() > 0 )
                {
                    scene_graph->addChild( plod.get() );
                }
            }
            else
            {
                // TODO: need a matrix transform atop the lod
                osg::ref_ptr<osg::MatrixTransform> xform = new osg::MatrixTransform();
                osg::LOD* lod = new osg::LOD();
                xform->addChild( lod );
                lod->setName( name_builder.str() );

                for( unsigned int level = 0; level < map_layer->getNumCellLevels(); level++ )
                {
                    osg::ref_ptr<MapLayerCell> cell = map_layer->createCell( col, row, level );
                    if ( cell.valid() )
                    {
                        osg::Group* placeholder = new osg::Group();
                        placeholder->setName( cell->toString() );
                        placeholder->setDataVariance( osg::Object::STATIC ); // so it doesn't get optimized away

                        lod->addChild( placeholder, cell->getDefinition()->getMinRange(), cell->getDefinition()->getMaxRange() );

                        // calculate the positional information:
                        osg::BoundingSphere bs = cell->getBoundingSphere( output_srs );
                        lod->setRadius( bs.radius() );
                        xform->setMatrix( osg::Matrix::translate( bs.center() ) );
                    }
                }

                if ( lod->getNumChildren() > 0 )
                {
                    scene_graph->addChild( xform.get() );
                }
            }
        }
    }
}

// scans the scene graph, created tasks that will compile paged cell files.
void
MapLayerCompiler::assemblePagedTasks( TaskList& out_tasks )
{
    // construct a filter environment template to use for all tasks:
    osg::ref_ptr<FilterEnv> env_template = getSession()->createFilterEnv();

    env_template->setTerrainNode( getTerrainNode() );
    env_template->setTerrainSRS( getTerrainSRS() );

    for( unsigned int row = 0; row < map_layer->getNumCellRows(); row++ )
    {
        for( unsigned int col = 0; col < map_layer->getNumCellColumns(); col++ )
        {
            std::stringstream name;
            name << "X" << col << "Y" << row;
            
            osg::PagedLOD* plod = dynamic_cast<osg::PagedLOD*>( GeomUtils::findNamedNode( name.str(), scene_graph.get() ) );
            if ( plod )
            {
                for( unsigned int level = 0; level < map_layer->getNumCellLevels(); level++ )
                {
                    osg::ref_ptr<MapLayerCell> cell = map_layer->createCell( col, row, level );
                    if ( cell.valid() && plod->getNumFileNames() > level )
                    {
                        MapLayerLevelOfDetail* def = cell->getDefinition();

                        // Make a filter enrivonment suitable for this cell:
                        FilterEnv* cell_env = env_template->clone();
                        cell_env->setInputSRS( def->getFeatureLayer()->getSRS() );
                        cell_env->setExtent( cell->getExtent() );

                        // create and add the new task.
                        Task* task = new PagedCellCompiler(
                            cell->makeURIFromTemplate( output_uri ),
                            def->getFeatureLayer(),
                            def->getFilterGraph(),
                            cell_env,
                            plod );

                        out_tasks.push_back( task );
                    }
                }
            }

        }
    }
}

void
MapLayerCompiler::assembleNonPagedTasks( TaskList& out_tasks )
{
    // construct a filter environment template to use for all tasks:
    osg::ref_ptr<FilterEnv> env_template = getSession()->createFilterEnv();

    env_template->setTerrainNode( getTerrainNode() );
    env_template->setTerrainSRS( getTerrainSRS() );
    env_template->setTerrainReadCallback( new SmartReadCallback() );

    for( unsigned int row = 0; row < map_layer->getNumCellRows(); row++ )
    {
        for( unsigned int col = 0; col < map_layer->getNumCellColumns(); col++ )
        {
            for( unsigned int level = 0; level < map_layer->getNumCellLevels(); level++ )
            {
                osg::ref_ptr<MapLayerCell> cell = map_layer->createCell( col, row, level );
                if ( cell.valid() )
                {
                    osg::Node* placeholder = GeomUtils::findNamedNode( cell->toString(), scene_graph.get() );
                    if ( placeholder )
                    {
                        MapLayerLevelOfDetail* def = cell->getDefinition();

                        // Make a filter enrivonment suitable for this cell:
                        FilterEnv* cell_env = env_template->clone();
                        cell_env->setInputSRS( def->getFeatureLayer()->getSRS() );
                        cell_env->setExtent( cell->getExtent() );

                        // create and add the new task.
                        Task* task = new NonPagedCellCompiler(
                            osgDB::getSimpleFileName( cell->makeURIFromTemplate( output_uri ) ),
                            def->getFeatureLayer(),
                            def->getFilterGraph(),
                            cell_env,
                            placeholder );

                        out_tasks.push_back( task );
                    }
                }
            }
        }
    }
}

bool
MapLayerCompiler::compile( TaskManager* my_task_man )
{
    osg::ref_ptr<TaskManager> task_man = my_task_man;
    if ( !task_man.valid() )
        task_man = new TaskManager( 0 );

    // get/build the skeleton first.
    // TODO: consider the option of setting a pre-existing (and possibly partially-built) skeleton graph
    buildSceneGraphSkeleton();

    // traverse the skeleton, looking for cells that require compilation:
    TaskList tasks;

    if ( getPaged() )
        assemblePagedTasks( tasks );
    else
        assembleNonPagedTasks( tasks );

    // add all tasks to the task manager:
    for( TaskList::iterator i = tasks.begin(); i != tasks.end(); i++ )
    {
        task_man->queueTask( i->get() );
    }

    // run until we're done.
    //
    // TODO: in the future, we can change the semantics so that you call compileSome() over and over
    //       until everything is complete...and compileSome() will return a list of everything that
    //       was started and everything that completed during that pass. This will allow for
    //       intermediate processing for UI's etc.
    while( task_man->wait() )
    {
        osg::ref_ptr<Task> completed_task = task_man->getNextCompletedTask();
        if ( completed_task.valid() )
        {
            CellCompiler* compiler = reinterpret_cast<CellCompiler*>( completed_task.get() );
            if ( compiler->getResult().isOK() )
            {
                compiler->runSynchronousPostProcess();

                // TODO: after each task completes, we will need to "localizeResources" which 
                // flushes the "used-resource" list and copies files locally if necessary.
                // Consider moving the "used resource" list to the FilterEnv/FilterGraphResult
                // so that we can do it on a FilterGraph basis instead of storing it in the 
                // Session... or at least store the "single use" used-resource list in the 
                // FilterEnv.

                FilterEnv* output_env = compiler->getResult().getOutputFilterEnv();
                //TODO:localization of resources
            }
            else
            {
                //TODO: replace this with Report facility
                osg::notify( osg::WARN ) << "ERROR: compilation of cell "
                    << compiler->getName() << " failed : "
                    << compiler->getResult().getMessage()
                    << std::endl;
            }
        }
    }

    return true;
}

