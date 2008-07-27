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

    virtual void runSynchronousPostProcess( Report* report ) { }
};

class QuadCellCompiler : public CellCompiler
{
public:
    QuadCellCompiler(const std::string& _abs_output_uri,
                      FeatureLayer*      layer,
                      FilterGraph*       graph,
                      FilterEnv*         env,
                      ResourcePackager*  _packager,
                      osgDB::Archive*    _archive =NULL )
        : CellCompiler( _abs_output_uri, layer, graph, env ),
          packager( _packager ),
          abs_output_uri( _abs_output_uri ),
          archive( _archive )
    {
        //NOP
    }

public:
    virtual void run() // overrides FeatureLayerCompiler::run()
    {
        // first check to see whether this cell needs compiling:
        need_to_compile = archive.valid() || !osgDB::fileExists( abs_output_uri );

        if ( need_to_compile )
        {
            // Compile the cell:
            CellCompiler::run();

            // Write the resulting node graph to disk, first ensuring that the output folder exists:
            // TODO: consider whether this belongs in the runSynchronousPostProcess() method
            if ( getResult().isOK() && getResultNode() )
            {
                has_drawables = GeomUtils::hasDrawables( getResultNode() );

                if ( has_drawables )
                {
                    if ( packager.valid() )
                    {
                        // update any texture/model refs in preparation for packaging:
                        packager->rewriteResourceReferences( getResultNode() );
                    }

                    if ( archive.valid() )
                    {
                        std::string file = osgDB::getSimpleFileName( abs_output_uri );
                        osgDB::ReaderWriter::WriteResult r = archive->writeNode( *getResultNode(), file );
                        if ( !r.success() )
                        {
                            result = FilterGraphResult::error( "Cell built OK, but failed to write to archive" );
                        }
                    }
                    else
                    {
                        bool write_ok = osgDB::makeDirectoryForFile( abs_output_uri );
                        if ( write_ok )
                        {
                            write_ok = osgDB::writeNodeFile( *getResultNode(), abs_output_uri );
                        }
                        if ( !write_ok )
                        {
                            result = FilterGraphResult::error( "Cell built OK, but failed to write to disk" );
                        }
                    }
                }
            }
        }
        else
        {
            result = FilterGraphResult::ok();
            has_drawables = true;
        }
    }

    virtual void runSynchronousPostProcess( Report* report )
    {
        if ( getResult().isOK() && getResultNode() && has_drawables && need_to_compile )
        {
            if ( packager.valid() )
            {
                packager->packageResources( env->getSession(), report );
            }
        }
    }


private:
    std::string abs_output_uri;
    osg::ref_ptr<osgDB::Archive> archive;
    osg::ref_ptr<ResourcePackager> packager;
    bool need_to_compile;
    bool has_drawables;
};


/*****************************************************************************/

MapLayerCompiler::MapLayerCompiler( MapLayer* _layer, Session* _session )
{
    map_layer = _layer;
    session   = _session;
    paged     = false;
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

Task*
MapLayerCompiler::createQuadKeyTask( const QuadKey& key )
{
    // construct a filter environment template to use for all tasks:
    osg::ref_ptr<FilterEnv> cell_env = getSession()->createFilterEnv();

    cell_env->setTerrainNode( getTerrainNode() );
    cell_env->setTerrainSRS( getTerrainSRS() );

    std::string abs_path = createAbsPathFromTemplate( "g" + key.toString() );

    Task* task = NULL;

    MapLayerLevelOfDetail* def = map_layer->getDefinition( key.createParentKey() );
    if ( def )
    {
        cell_env->setInputSRS( def->getFeatureLayer()->getSRS() );
        cell_env->setExtent( map_layer->getAreaOfInterest().getSRS()->transform( key.getExtent() ) );
        cell_env->setProperty( Property( "compiler.cell_id", key.toString() ) );

        task = new QuadCellCompiler(
            abs_path,
            def->getFeatureLayer(),
            def->getFilterGraph(),
            cell_env.get(),
            resource_packager.get(),
            getArchive() );

        osg::notify( osg::NOTICE )
            << "Task: Key = " << key.toString() << ", LOD = " << key.getLOD() << ", Extent = " << key.getExtent().toString() 
            << " (w=" << key.getExtent().getWidth() << ", h=" << key.getExtent().getHeight() << ")"
            << std::endl;
    }

    return task;
}

//osg::Vec3d
//MapLayerCompiler::calculateCentroid( const QuadKey& key )
//{
//    // first get the output srs centroid:
//    const GeoExtent& cell_extent = key.getExtent();    
//    GeoPoint centroid = map_layer->getOutputSRS( getSession(), terrain_srs.get() )->transform( cell_extent.getCentroid() );
//    
//    if ( terrain_node.valid() && terrain_srs.valid() )
//    {
//        centroid = GeomUtils::clampToTerrain( centroid, terrain_node.get(), terrain_srs.get(), read_callback.get() );
//    }
//    return centroid;
//}

void
MapLayerCompiler::setCenterAndRadius( osg::PagedLOD* plod, const QuadKey& key )
{
    SpatialReference* srs = map_layer->getOutputSRS( getSession(), terrain_srs.get() );
    // first get the output srs centroid:
    const GeoExtent& cell_extent = key.getExtent();    
    GeoPoint centroid = srs->transform( cell_extent.getCentroid() );
    GeoPoint sw = srs->transform( cell_extent.getSouthwest() );

    double radius = (centroid-sw).length();
    
    if ( terrain_node.valid() && terrain_srs.valid() )
    {
        centroid = GeomUtils::clampToTerrain( centroid, terrain_node.get(), terrain_srs.get(), read_callback.get() );
    }

    plod->setCenter( centroid );
    plod->setRadius( radius );
}



// NEW VERSION
bool
MapLayerCompiler::compile( TaskManager* my_task_man )
{
    // make a task manager to run the compilation:
    osg::ref_ptr<TaskManager> task_man = my_task_man;
    if ( !task_man.valid() )
        task_man = new TaskManager( 0 );

    // for clampin
    read_callback = new SmartReadCallback();

    // determine the output SRS:
    osg::ref_ptr<SpatialReference> out_srs = map_layer->getOutputSRS( getSession(), terrain_srs.get() );
    if ( !out_srs.valid() )
    {
        osg::notify( osg::WARN ) << "Unable to figure out the output SRS; aborting." << std::endl;
        return false;
    }

    // figure out the bounds of the compilation area and create a Q map. We want a sqaure AOI..maybe
    const GeoExtent& aoi = map_layer->getAreaOfInterest();
    if ( !aoi.isValid() )
    {
        osg::notify( osg::WARN ) << "Invalid area of interest in the map layer - no data?" << std::endl;
        return false;
    }

    QuadMap qmap;
    if ( out_srs->isGeocentric() )
    {
        // for a geocentric map, use a modified GEO quadkey:
        // (yes, that MIN_LAT of -180 is correct)
        qmap = QuadMap( GeoExtent( -180.0, -180.0, 180.0, 90.0, Registry::SRSFactory()->createWGS84() ) );
    }
    else
    {
        double max_span = std::max( aoi.getWidth(), aoi.getHeight() );
        GeoPoint sw = aoi.getSouthwest();
        GeoPoint ne( sw.x() + max_span, sw.y() + max_span, aoi.getSRS() );
        qmap = QuadMap( GeoExtent( sw, ne ) );
    }

    osg::notify( osg::NOTICE )
        << "QMAP: " << std::endl
        << "   Base LOD = " << qmap.getStartingLodForCellsOfSize( map_layer->getCellWidth(), map_layer->getCellHeight() ) << std::endl
        << "   Extent = " << qmap.getBounds().toString() << ", w=" << qmap.getBounds().getWidth() << ", h=" << qmap.getBounds().getHeight() << std::endl
        << std::endl;

    // Now, build the index and collect the list of keys for which to compile data.
    QuadKeyList keys;
    collectGeometryKeys( qmap, keys );

    // make a build task for each quad cell we collected:
    int total_tasks = keys.size();
    for( QuadKeyList::iterator i = keys.begin(); i != keys.end(); i++ )
    {
        task_man->queueTask( createQuadKeyTask( *i ) );
    }

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

    while( task_man->wait( 5000L ) )
    {
        osg::ref_ptr<Task> completed_task = task_man->getNextCompletedTask();
        if ( completed_task.valid() )
        {
            tasks_completed++;

            CellCompiler* compiler = reinterpret_cast<CellCompiler*>( completed_task.get() );
            if ( compiler->getResult().isOK() )
            {
                compiler->runSynchronousPostProcess( default_report.get() );

                // TODO: after each task completes, we will need to "localizeResources" which 
                // flushes the "used-resource" list and copies files locally if necessary.
                // Consider moving the "used resource" list to the FilterEnv/FilterGraphResult
                // so that we can do it on a FilterGraph basis instead of storing it in the 
                // Session... or at least store the "single use" used-resource list in the 
                // FilterEnv.

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
                osg::notify(osg::NOTICE) << tasks_completed << "/" << total_tasks 
                    << " tasks (" << (int)p << "%) complete, " 
                    << buf << " remaining" << std::endl;
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

    // build the index that will point to the keys:
    buildQuadMapIndex( qmap );

    // scrub empties (obselete now..)
    //scrubQuadKeyIndex( keys );

    // finally, resolve any references at the root level and optimize the root graph.
    if ( getSceneGraph() )
    {
        if ( resource_packager.valid() )
        {
            resource_packager->rewriteResourceReferences( getSceneGraph() );
        }

        osgUtil::Optimizer opt;
        opt.optimize( getSceneGraph(), 
            osgUtil::Optimizer::SPATIALIZE_GROUPS |
            osgUtil::Optimizer::STATIC_OBJECT_DETECTION |
            osgUtil::Optimizer::SHARE_DUPLICATE_STATE );
    }
    
    osg::Timer_t end_time = osg::Timer::instance()->tick();
    double duration_s = osg::Timer::instance()->delta_s( start_time, end_time );

    osg::notify( osg::NOTICE )
        << "Compilation finished, total time = " << duration_s << " seconds"
        << std::endl;

    return true;
}

// builds an index node (pointers to quadkey geometry nodes) that references 
// subtiles.
osg::Node*
MapLayerCompiler::createIntermediateIndexNode( const QuadKey& key, float min_range, float max_range )
{
    osg::Group* group = NULL;

    for( unsigned int quadrant = 0; quadrant < 4; quadrant++ )
    {
        QuadKey subkey = key.createSubKey( quadrant );

        if ( osgDB::fileExists( createAbsPathFromTemplate( "g"+subkey.toString() ) ) )
        {
            if ( !group )
            {
                group = new osg::Group();
                group->setName( key.toString() );
            }

            // enter the geometry as a proxy node (so we can rebuild it without modifying
            // the index)
            //osg::PagedLOD* proxy = new osg::PagedLOD();
            osg::ProxyNode* proxy = new osg::ProxyNode();
            proxy->setLoadingExternalReferenceMode( osg::ProxyNode::LOAD_IMMEDIATELY );
            proxy->setFileName( 0, createRelPathFromTemplate( "g" + subkey.toString() ) );
            //proxy->setRange( 0, 0, 1e10 );
            //proxy->setCenter( calculateCentroid( key ) );
        
            // enter the subtile set as a paged index node reference:
            osg::PagedLOD* plod = new osg::PagedLOD();
            plod->addChild( proxy, max_range, 1e10 );
            plod->setFileName( 1, createRelPathFromTemplate( "i" + subkey.toString() ) );
            //plod->setRange( 1, min_range, max_range ); // last one should always be min=0
            plod->setRange( 1, 0, max_range ); // last one should always be min=0
            //plod->setCenter( calculateCentroid( key ) );
            setCenterAndRadius( plod, key );

            group->addChild( plod );
        }
    }

    return group;
}

// creates an index node (pointer to quadkey geometry nodes) that has no
// children.
osg::Node*
MapLayerCompiler::createLeafIndexNode( const QuadKey& key )
{
    osg::Group* group = NULL;

    for( unsigned int i=0; i<4; i++ )
    {
        QuadKey quadrant_key = key.createSubKey( i );

        if ( osgDB::fileExists( createAbsPathFromTemplate( "g"+quadrant_key.toString() ) ) )
        {
            if ( !group )
            {
                group = new osg::Group();
                group->setName( key.toString() );
            }

            //osg::PagedLOD* proxy = new osg::PagedLOD();
            osg::ProxyNode* proxy = new osg::ProxyNode();
            proxy->setLoadingExternalReferenceMode( osg::ProxyNode::LOAD_IMMEDIATELY );
            proxy->setFileName( 0, createRelPathFromTemplate( "g" + quadrant_key.toString() ) );
            //proxy->setCenter( calculateCentroid( quadrant_key ) );
            //proxy->setRange( 0, 0, 1e10 );

            group->addChild( proxy );
        }
    }

    return group;
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

struct ScrubIndexVisitor : public osg::NodeVisitor {
    ScrubIndexVisitor( const std::string& _base ) 
        : base(_base), changes(0), osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) { }
    void apply( osg::ProxyNode& node ) {
        if ( !osgDB::fileExists( PathUtils::combinePaths( base, node.getFileName(0) ) ) ) {
            node.setNodeMask( 0 );
            changes++;
        }
        osg::NodeVisitor::apply( node );
    }
    void apply( osg::PagedLOD& node ) {
        if ( node.getNumFileNames() > node.getNumChildren() ) { //todo... all filename children
            if ( !osgDB::fileExists( PathUtils::combinePaths( base, node.getFileName( node.getNumFileNames()-1 ) ) ) ) {
                node.removeChildren( node.getNumFileNames()-1, 1 );
                changes++;
            }
        }
        osg::NodeVisitor::apply( node );
    }
    std::string base;
    int changes;
};


void
MapLayerCompiler::scrubQuadKeyIndex( const QuadKeyList& build_keys )
{
    // convert the built-keys list into a unique set of parent keys:
    QuadKeySet keys;
    for( QuadKeyList::const_iterator i = build_keys.begin(); i != build_keys.end(); i++ )
        keys.insert( (*i).createParentKey() );

    // disable the automatic resolution of ProxyNodes
    osg::ref_ptr<osgDB::ReaderWriter::Options> options = osgDB::Registry::instance()->getOptions()?
        dynamic_cast<osgDB::ReaderWriter::Options*>(osgDB::Registry::instance()->getOptions()->clone( osg::CopyOp::DEEP_COPY_ALL ) ) :
        new osgDB::ReaderWriter::Options();
    options->setOptionString( options->getOptionString() + " noLoadExternalReferenceFiles" );

    unsigned int scrubs = 0;

    osg::notify( osg::NOTICE ) << "Scrubbing....." << std::endl;

    for( QuadKeySet::const_iterator j = keys.begin(); j != keys.end(); j++ )
    {
        const QuadKey& key = *j;
        std::string file = createAbsPathFromTemplate( "i" + key.toString() );
        osg::Node* index = osgDB::readNodeFile( file, options.get() );
        if ( index )
        {
            ScrubIndexVisitor v( osgDB::getFilePath( output_uri ) );
            index->accept( v );
            scrubs += v.changes;
            if ( v.changes > 0 )
            {
                osgDB::writeNodeFile( *index, file );
            }
        }
    }

    osg::notify( osg::NOTICE ) << "Scrubbed " << scrubs << " empty cells." << std::endl;
}


// assembles the list of quadkeys for which to build geometry cells. We can derive the set
// of index cells from this set of geometry cells if necessary.
void
MapLayerCompiler::collectGeometryKeys( const QuadMap& qmap, QuadKeyList& geom_keys )
{
    // the starting LOD is the best fit the the cell size:
    unsigned int starting_lod = qmap.getStartingLodForCellsOfSize( map_layer->getCellWidth(), map_layer->getCellHeight() );

    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
    {
        MapLayerLevelOfDetail* level_def = i->get();
        unsigned int lod = starting_lod + level_def->getDepth();
        
        // get the extent of tiles that we will build based on the AOI:
        unsigned int cell_xmin, cell_ymin, cell_xmax, cell_ymax;
        qmap.getCells(
            map_layer->getAreaOfInterest(), lod,
            cell_xmin, cell_ymin, cell_xmax, cell_ymax );

        for( unsigned int y = cell_ymin; y <= cell_ymax; y++ )
        {
            for( unsigned int x = cell_xmin; x <= cell_xmax; x++ )
            {
                QuadKey key( x, y, lod, qmap ); 
                for( unsigned int k=0; k<4; k++ )
                {
                    geom_keys.push_back( key.createSubKey( k ) );
                }
            }
        }
    }
}

// builds and writes all the index nodes.
void
MapLayerCompiler::buildQuadMapIndex( const QuadMap& qmap ) //, QuadKeyList& keys_to_build )
{
    // first, determine the SRS of the output scene graph so that we can
    // make pagedlod/lod centroids.
    SpatialReference* output_srs = map_layer->getOutputSRS( getSession(), getTerrainSRS() );

    // first build the very top level.
    scene_graph = new osg::Group();

    // the starting LOD is the best fit the the cell size:
    unsigned int starting_lod = qmap.getStartingLodForCellsOfSize( map_layer->getCellWidth(), map_layer->getCellHeight() );

    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
    {
        MapLayerLevelOfDetail* level_def = i->get();
        unsigned int lod = starting_lod + level_def->getDepth();

        MapLayerLevelOfDetail* sub_level_def = i+1 != map_layer->getLevels().end()? (i+1)->get() : NULL;
        float min_range, max_range;
        if ( sub_level_def )
        {
            min_range = sub_level_def->getMinRange();
            max_range = sub_level_def->getMaxRange();
        }
        
        // get the extent of tiles that we will build based on the AOI:
        unsigned int cell_xmin, cell_ymin, cell_xmax, cell_ymax;
        qmap.getCells(
            map_layer->getAreaOfInterest(), lod,
            cell_xmin, cell_ymin, cell_xmax, cell_ymax );

        for( unsigned int y = cell_ymin; y <= cell_ymax; y++ )
        {
            for( unsigned int x = cell_xmin; x <= cell_xmax; x++ )
            {
                osg::ref_ptr<osg::Node> node;

                QuadKey key( x, y, lod, qmap );  

                //osg::notify( osg::NOTICE )
                //    << "Cell: " << std::endl
                //    << "   Quadkey = " << key.toString() << std::endl
                //    << "   LOD = " << key.getLOD() << std::endl
                //    << "   Extent = " << key.getExtent().toString() << " (w=" << key.getExtent().getWidth() << ", h=" << key.getExtent().getHeight() << ")" << std::endl
                //    << std::endl;

                node = sub_level_def ?
                    createIntermediateIndexNode( key, min_range, max_range ) :
                    createLeafIndexNode( key );
                
                if ( node.valid() )
                {
                    std::string out_file = createAbsPathFromTemplate( "i" + key.toString() );

                    if ( !osgDB::writeNodeFile( *(node.get()), out_file ) )
                    {
                        osg::notify(osg::WARN) << "FAILED to write index file " << out_file << std::endl;
                    }
                    
                    // at the top level, assemble the root node
                    if ( i == map_layer->getLevels().begin() )
                    {
                        double top_min_range = sub_level_def? 0 : level_def->getMinRange();

                        osg::PagedLOD* plod = new osg::PagedLOD();
                        plod->setName( key.toString() );
                        plod->setFileName( 0, createRelPathFromTemplate( "i" + key.toString() ) );
                        plod->setRange( 0, top_min_range, level_def->getMaxRange() );
                        //plod->setCenter( calculateCentroid( key ) );
                        setCenterAndRadius( plod, key );

                        scene_graph->addChild( plod );
                    }
                }
            }
        }
    }
}
