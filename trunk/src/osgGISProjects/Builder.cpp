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

#include <osgGISProjects/Builder>
#include <osgGISProjects/Build>
#include <osgGISProjects/MapLayerCompiler>
#include <osgGIS/FeatureLayer>
#include <osgGIS/SimpleLayerCompiler>
#include <osgGIS/PagedLayerCompiler>
#include <osgGIS/GriddedLayerCompiler>
#include <osgGIS/Registry>
#include <osgGIS/FilterGraph>
#include <osgGIS/Resource>
#include <osgGIS/FeatureStoreCompiler>
#include <osgGIS/Utils>
#include <osgGIS/ResourcePackager>
#include <osg/Notify>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <osgDB/Archive>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>

using namespace osgGISProjects;
using namespace osgGIS;

/*** Statics ********************************************************/

static bool
getTerrainData(Terrain*                        terrain,
               osg::ref_ptr<osg::Node>&        out_terrain_node,
               osg::ref_ptr<SpatialReference>& out_terrain_srs,
               GeoExtent&                      out_terrain_extent)
{
    if ( terrain && terrain->getURI().length() > 0 )
    {
        out_terrain_node = osgDB::readNodeFile( terrain->getAbsoluteURI() );

        out_terrain_srs = terrain->getExplicitSRS();

        if ( out_terrain_node.valid() )
        {
            if ( !out_terrain_srs.valid() )
            {
                out_terrain_srs = Registry::instance()->getSRSFactory()->createSRSfromTerrain( out_terrain_node.get() );
            }

            osg::notify( osg::NOTICE )
                << "Loaded TERRAIN from \"" << terrain->getAbsoluteURI() << "\", SRS = "
                << (out_terrain_srs.valid()? out_terrain_srs->getName() : "unknown")
                << std::endl;
        }
        else
        {
            osg::notify( osg::WARN )
                << "Unable to load data for terrain \""
                << terrain->getName() << "\"." 
                << std::endl;

            return false;
        }
    }
    
    out_terrain_extent = GeoExtent(
        -180, -90, 180, 90, 
        Registry::instance()->getSRSFactory()->createWGS84() );

    return true;
}


/*** Class methods ***************************************************/

Builder::Builder( Project* _project )
{
    project = _project;
    num_threads = 0;
}

void
Builder::setNumThreads( int _num_threads )
{
    num_threads = _num_threads;
}

bool
Builder::build()
{
    if ( !project.valid() )
        return false;

    VERBOSE_OUT <<
        "No targets specified; building all layers." << std::endl;

    bool ok = true;

    for( BuildLayerList::iterator i = project->getLayers().begin(); i != project->getLayers().end() && ok; i++ )
    {
        ok = build( i->get() );
    }

    return ok;
}


bool
Builder::build( const std::string& target_name )
{
    if ( !project.valid() )
        return false;

    if ( target_name.length() > 0 )
    {
        BuildTarget* target = project->getTarget( target_name );
        if ( !target )
            VERBOSE_OUT << "No target " << target_name << " found in the project." << std::endl;
        return target? build( target ) : false;
    }
    else
    {
        return build();
    }
}


bool
Builder::build( BuildTarget* target )
{
    if ( !project.valid() || !target )
        return false;

    VERBOSE_OUT <<
        "Building target \"" << target->getName() << "\"." << std::endl;

    bool ok = true;

    for( BuildLayerList::const_iterator i = target->getLayers().begin(); i != target->getLayers().end() && ok; i++ )
    {
        ok = build( i->get() );
    }
    
    if ( ok )
    {
        VERBOSE_OUT <<
            "Done building target \"" << target->getName() << "\"." << std::endl;
    }
    return ok;
}


// builds a source, if necessary.
bool
Builder::build( Source* source, Session* session )
{
    osg::notify(osg::NOTICE) << "Building source " << source->getName() << std::endl;

    // only need to build intermediate sources.
    if ( !source->isIntermediate() )
    {
        osg::notify(osg::NOTICE) << "...source " << source->getName() << " does not need building (not intermediate)." << std::endl;
        return true;
    }

    Source* parent = source->getParentSource();
    if ( !parent )
    {
        osg::notify(osg::WARN) << "...ERROR: No parent source found for intermediate source \"" << source->getName() << "\"" << std::endl;
        return false;
    }

    // check whether a rebuild is required:
    if ( parent->getTimeLastModified() < source->getTimeLastModified() )
    {
        osg::notify(osg::NOTICE) << "...source " << source->getName() << " does not need building (newer than parent)." << std::endl;
        return true;
    }

    // build it's parent first:
    if ( !build( parent, session ) )
    {
        osg::notify(osg::WARN) << "...ERROR: Failed to build source \"" << parent->getName() << "\", parent of source \"" << source->getName() << "\"" << std::endl;
        return false;
    }

    // validate the existence of a filter graph:
    FilterGraph* graph = source->getFilterGraph();
    if ( !graph )
    {
        osg::notify(osg::WARN) << "...ERROR: No filter graph set for intermediate source \"" << source->getName() << "\"" << std::endl;
        return false;
    }

    // establish a feature layer for the parent source:
    osg::ref_ptr<FeatureLayer> feature_layer = Registry::instance()->createFeatureLayer(
        parent->getAbsoluteURI() );
    if ( !feature_layer.valid() )
    {
        osg::notify( osg::WARN ) << "...ERROR: Cannot access source \"" << source->getName() << "\"" << std::endl;
        return false;
    }

    //TODO: should we allow terrains for a source compile?? No. Because we would need to transform
    // the source into terrain SRS space, which we do not want to do until we're building nodes.

    // initialize a source data compiler. we use a temporary session because this source build
    // is unrelated to the current "master" build. If we used the same session, the new feature
    // store would hang around and not get properly closed out for use in the next round.
    osg::ref_ptr<Session> temp_session = session->derive();
    osg::ref_ptr<FilterEnv> source_env = temp_session->createFilterEnv();

    FeatureStoreCompiler compiler( feature_layer.get(), graph );

    if ( !compiler.compile( source->getAbsoluteURI(), source_env.get() ) )
    {
        osg::notify( osg::WARN ) << "...ERROR: failure compiling source \"" << source->getName() << "\"" << std::endl;
        return false;
    }

    osg::notify(osg::NOTICE) << "...done compiling source \"" << source->getName() << "\"" << std::endl;

    return true;
}


bool
Builder::addSlicesToMapLayer(BuildLayerSliceList& slices,
                             MapLayer* map_layer,
                             ResourcePackager* default_packager,
                             unsigned int depth, 
                             Session* session,
                             Source* parent_source )
{
    for( BuildLayerSliceList::iterator i = slices.begin(); i != slices.end(); i++ )
    {    
        BuildLayerSlice* slice = i->get();

        if ( slice->getSource() && !build( slice->getSource(), session ) )
        {
            osg::notify( osg::WARN )
                << "Unable to build source \"" << slice->getSource()->getName() << "\" or one of its dependencies." 
                << std::endl;
            return false;
        }

        Source* slice_source = slice->getSource()? slice->getSource() : parent_source;

        osg::ref_ptr<ResourcePackager> packager = default_packager? default_packager->clone() : NULL;
        if ( packager.valid() )
        {
            packager->setMaxTextureSize(
                slice->getProperties().getIntValue( "max_texture_size", default_packager->getMaxTextureSize() ) );
            packager->setCompressTextures(
                slice->getProperties().getBoolValue( "compress_textures", default_packager->getCompressTextures() ) );
            packager->setInlineTextures(
                slice->getProperties().getBoolValue( "inline_textures", default_packager->getInlineTextures() ) );
        }        

        if ( slice_source )
        {
            FeatureLayer* feature_layer = Registry::instance()->createFeatureLayer(
                slice_source->getAbsoluteURI() );

            if ( !feature_layer )
            {
                osg::notify( osg::WARN ) 
                    << "Cannot access source \"" << slice_source->getName() << std::endl;
                return false;
            }

            map_layer->push(
                feature_layer,
                slice->getFilterGraph(),
                packager.get(),
                slice->getMinRange(),
                slice->getMaxRange(),
                true,
                depth );
        }

        // now add any sub-slice children:
        if ( !addSlicesToMapLayer( slice->getSubSlices(), map_layer, packager.get(), depth+1, session, slice_source ) )
        {
            return false;
        }
    }  

    return true;
}


bool
Builder::build( BuildLayer* layer )
{
    std::string work_dir_name = project->getAbsoluteWorkingDirectory();
    if ( work_dir_name.length() == 0 )
        work_dir_name = ".osggis-" + project->getName();

    std::string work_dir = PathUtils::combinePaths( 
        project->getBaseURI(),
        work_dir_name );

    if ( osgDB::makeDirectory( work_dir ) )
    {
        Registry::instance()->setWorkDirectory( work_dir );
    }

    VERBOSE_OUT <<
        "Building layer \"" << layer->getName() << "\"." << std::endl;

    // first create and initialize a Session that will share data across the build.
    osg::ref_ptr<Session> session = new Session();

    // add shared scripts to the session:
    for( ScriptList::iterator i = project->getScripts().begin(); i != project->getScripts().end(); i++ )
        session->addScript( i->get() );

    // add shared resources to the session:
    for( ResourceList::iterator i = project->getResources().begin(); i != project->getResources().end(); i++ )
        session->getResources()->addResource( i->get() );

    // now establish the source data record form this layer and open a feature layer
    // that connects to that source.
    Source* source = layer->getSource(); // default source.. may be overriden in slices
    //if ( !source )
    //{
    //    //TODO: log error
    //    osg::notify( osg::WARN ) 
    //        << "No source specified for layer \"" << layer->getName() << "\"." << std::endl;
    //    return false;
    //}



    // recursively build any sources that need building.
    if ( source && !build( source, session.get() ) )
    {
        osg::notify( osg::WARN )
            << "Unable to build source \"" << source->getName() << "\" or one of its dependencies." 
            << std::endl;
        return false;
    }    

    osg::ref_ptr<FeatureLayer> feature_layer;

    if ( source )
    {
        feature_layer = Registry::instance()->createFeatureLayer( source->getAbsoluteURI() );

        if ( !feature_layer.valid() )
        {
            //TODO: log error
            osg::notify( osg::WARN ) 
                << "Cannot access source \"" << source->getName() 
                << "\" for layer \"" << layer->getName() << "\"." << std::endl;
            return false;
        }
    }

    // The reference terrain:
    osg::ref_ptr<osg::Node>        terrain_node;
    osg::ref_ptr<SpatialReference> terrain_srs;
    GeoExtent                      terrain_extent;

    Terrain* terrain = layer->getTerrain();
    if ( !getTerrainData( terrain, terrain_node, terrain_srs, terrain_extent ) )
        return false;

    // output file:
    std::string output_file = layer->getAbsoluteTargetPath();
    osgDB::makeDirectoryForFile( output_file );
    if ( !osgDB::fileExists( osgDB::getFilePath( output_file ) ) )
    {
        osg::notify( osg::WARN ) 
            << "Unable to establish target location for layer \""
            << layer->getName() << "\" at \"" << output_file << "\"." 
            << std::endl;
        return false;
    }

    // whether to include textures in IVE files:
    bool inline_ive_textures = layer->getProperties().getBoolValue( "inline_textures", false );
    
    // TODO: deprecate this as we move towards the ResourcePackager...
    osgDB::ReaderWriter::Options* options;
    if ( inline_ive_textures )
    {
        options = new osgDB::ReaderWriter::Options( "noWriteExternalReferenceFiles useOriginalExternalReferences" );
    }
    else
    {
        options = new osgDB::ReaderWriter::Options( "noTexturesInIVEFile noWriteExternalReferenceFiles useOriginalExternalReferences" );
    }
    
    osgDB::Registry::instance()->setOptions( options );


    osg::ref_ptr<osgDB::Archive> archive;
    std::string archive_file = output_file;

    if ( osgDB::getLowerCaseFileExtension( output_file ) == "osga" )
    {
        archive = osgDB::openArchive( output_file, osgDB::Archive::CREATE, 4096 );
        output_file = "out.ive";

        // since there's no way to set the master file name...fake it out
        osg::ref_ptr<osg::Group> dummy = new osg::Group();
        archive->writeNode( *(dummy.get()), output_file );
    }

    // intialize a task manager if necessary:
    osg::ref_ptr<TaskManager> manager = 
        num_threads > 1? new TaskManager( num_threads ) :
        num_threads < 1? new TaskManager() :
        NULL;

    if ( layer->getType() == BuildLayer::TYPE_QUADTREE ) // testing out the NEW process
    {
        MapLayer* map_layer = new MapLayer();
        
        // a resource packager if necessary will copy ext-ref files to the output location:
        osg::ref_ptr<ResourcePackager> packager;
        if ( layer->getProperties().getBoolValue( "localize_resources", false ) )
        {
            packager = new ResourcePackager();
            packager->setArchive( archive.get() );
            packager->setOutputLocation( osgDB::getFilePath( output_file ) );
            packager->setMaxTextureSize( layer->getProperties().getIntValue( "max_texture_size", 0 ) );
            packager->setCompressTextures( layer->getProperties().getBoolValue( "compress_textures", false ) );
            packager->setInlineTextures( layer->getProperties().getBoolValue( "inline_textures", false ) );
        }

        if ( !addSlicesToMapLayer( layer->getSlices(), map_layer, packager.get(), 0, session.get(), source ) )
        {
            osg::notify(osg::WARN) << "Failed to add all slices to layer " << layer->getName() << std::endl;
            return false;
        }
        
        // calculate the grid cell size:
        double col_size = layer->getProperties().getDoubleValue( "col_size", -1.0 );
        double row_size = layer->getProperties().getDoubleValue( "row_size", -1.0 );
        if ( col_size <= 0.0 || row_size <= 0.0 )
        {
            int num_cols = std::max( 1, layer->getProperties().getIntValue( "num_cols", 1 ) );
            int num_rows = std::max( 1, layer->getProperties().getIntValue( "num_rows", 1 ) );
            col_size = map_layer->getAreaOfInterest().getWidth() / (double)num_cols;
            row_size = map_layer->getAreaOfInterest().getHeight() / (double)num_rows;
        }
        map_layer->setCellWidth( col_size );
        map_layer->setCellHeight( row_size );

        // create and confiugure the compiler:
        MapLayerCompiler compiler( map_layer, session.get() );

        compiler.setAbsoluteOutputURI( output_file );
        compiler.setPaged( layer->getProperties().getBoolValue( "paged", false ) );
        compiler.setTerrain( terrain_node.get(), terrain_srs.get(), terrain_extent );
        compiler.setArchive( archive.get(), archive_file );
        compiler.setResourcePackager( packager.get() );
                
        // build the layer and write the root file to output:
        bool ok = compiler.compile( manager.get() );

        if ( ok )
        {
            osgDB::ReaderWriter::Options* options = osgDB::Registry::instance()->getOptions();
            if ( archive.valid() )
            {
                archive->writeNode( *compiler.getSceneGraph(), output_file, options );
            }
            else
            {
                osgDB::writeNodeFile( *compiler.getSceneGraph(), output_file, options );
            }
        }
    }

    // if we have a valid terrain, use the paged layer compiler. otherwise
    // use a simple compiler.
    else
    if ( terrain && layer->getType() == BuildLayer::TYPE_CORRELATED )
    {
        PagedLayerCompiler compiler;

        compiler.setProperties( layer->getProperties() );
        compiler.setSession( session.get() );
        compiler.setTaskManager( manager.get() );
        compiler.setTerrain( terrain_node.get(), terrain_srs.get(), terrain_extent );
        compiler.setArchive( archive.get(), archive_file );
        
        for( BuildLayerSliceList::iterator i = layer->getSlices().begin(); i != layer->getSlices().end(); i++ )
        {
            compiler.addFilterGraph(
                i->get()->getMinRange(),
                i->get()->getMaxRange(),
                i->get()->getFilterGraph() );
        }

        compiler.compile(
            feature_layer.get(),
            output_file );
    }

    else if ( layer->getType() == BuildLayer::TYPE_GRIDDED )
    {
        GriddedLayerCompiler compiler;

        compiler.setProperties( layer->getProperties() );
        compiler.setSession( session.get() );
        compiler.setTaskManager( manager.get() );
        compiler.setTerrain( terrain_node.get(), terrain_srs.get(), terrain_extent );
        compiler.setArchive( archive.get(), archive_file );
        
        for( BuildLayerSliceList::iterator i = layer->getSlices().begin(); i != layer->getSlices().end(); i++ )
        {
            compiler.addFilterGraph(
                i->get()->getMinRange(),
                i->get()->getMaxRange(),
                i->get()->getFilterGraph() );
        }

        osg::ref_ptr<osg::Node> node = compiler.compile(
            feature_layer.get(),
            output_file );

        if ( node.valid() )
        {
            if ( archive.valid() )
            {
                archive->writeNode( 
                    *(node.get()), 
                    output_file,
                    osgDB::Registry::instance()->getOptions() );
            }
            else
            {
                osgDB::writeNodeFile( 
                    *(node.get()),
                    output_file,
                    osgDB::Registry::instance()->getOptions() );
            }
        }        
    }
    else
    {
        SimpleLayerCompiler compiler;

        compiler.setProperties( layer->getProperties() );
        compiler.setSession( session.get() );
        compiler.setTaskManager( manager.get() );
        compiler.setTerrain( terrain_node.get(), terrain_srs.get(), terrain_extent );
        compiler.setArchive( archive.get(), archive_file );
        
        for( BuildLayerSliceList::iterator i = layer->getSlices().begin(); i != layer->getSlices().end(); i++ )
        {
            compiler.addFilterGraph(
                i->get()->getMinRange(),
                i->get()->getMaxRange(),
                i->get()->getFilterGraph() );
        }

        osg::ref_ptr<osg::Node> node = compiler.compile(
            feature_layer.get(),
            output_file );

        if ( node.valid() )
        {
            if ( archive.valid() )
            {
                archive->writeNode( 
                    *(node.get()), 
                    output_file,
                    osgDB::Registry::instance()->getOptions() );
            }
            else
            {
                osgDB::writeNodeFile( 
                    *(node.get()),
                    output_file,
                    osgDB::Registry::instance()->getOptions() );
            }
        }        
    }

    if ( archive.valid() )
    {
        archive->close();
    }

    VERBOSE_OUT <<
        "Done building layer \"" << layer->getName() << "\"." << std::endl;

    return true;
}

