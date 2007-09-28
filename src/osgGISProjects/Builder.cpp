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

#include <osgGISProjects/Builder>
#include <osgGISProjects/Build>
#include <osgGIS/FeatureLayer>
#include <osgGIS/PagedLayerCompiler>
#include <osgGIS/Registry>
#include <osgGIS/Script>
#include <osg/Notify>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

using namespace osgGISProjects;
using namespace osgGIS;


Builder::Builder( Project* _project, const std::string& _base_uri )
{
    project = _project;
    base_uri = _base_uri;
}


static bool
isUriRooted( const std::string& path )
{
    //TODO
    return false;
}


std::string
Builder::resolveURI( const std::string& input )
{
    if ( isUriRooted( input ) || base_uri.length() == 0 )
    {
        return input;
    }
    else
    {
        return osgDB::concatPaths( base_uri, input );
    }
}


bool
Builder::build()
{
    if ( !project.valid() )
        return false;

    return project->getBuilds().size() > 0?
        build( project->getBuilds().front().get() ) :
        false;
}


bool
Builder::build( const std::string& build_name )
{
    if ( !project.valid() )
        return false;

    Build* b = project->getBuild( build_name );
    return b? build( b ) : false;
}


bool
Builder::build( Build* b )
{
    if ( !project.valid() || !b )
        return false;

    for( BuildLayerList::iterator i = b->getLayers().begin(); i != b->getLayers().end(); i++ )
    {
        BuildLayer* layer = i->get();
        
        Source* source = layer->getSource();
        if ( !source )
        {
            //TODO: log error
            osg::notify( osg::WARN ) << "No source for build " << b->getName() << std::endl;
            continue;
        }

        osg::ref_ptr<FeatureLayer> feature_layer = Registry::instance()->createFeatureLayer(
            resolveURI( source->getURI() ) );
        if ( !feature_layer.valid() )
        {
            //TODO: log error
            osg::notify( osg::WARN ) << "Cannot create feature layer for build " << b->getName() << std::endl;
            continue;
        }

        // The reference terrain:
        osg::ref_ptr<osg::Node>        terrain_node;
        osg::ref_ptr<SpatialReference> terrain_srs;

        Terrain* terrain = layer->getTerrain();

        if ( terrain && terrain->getURI().length() > 0 )
        {
            std::string terrain_uri = resolveURI( terrain->getURI() );
            terrain_node = osgDB::readNodeFile( terrain_uri );
            if ( terrain_node.valid() )
            {
                terrain_srs = Registry::instance()->getSRSFactory()->createSRSfromTerrain( terrain_node.get() );
            }
            else
            {
                osg::notify( osg::WARN ) << "Unable to load terrain node from " << terrain_uri << std::endl;
                continue;
            }
        }
        
        //TODO: parameterize this..
        GeoExtent terrain_extent( 
            -180, -90, 180, 90, 
            Registry::instance()->getSRSFactory()->createWGS84() );

        // output file:
        const std::string& output_file = resolveURI( layer->getTarget() );
        osgDB::makeDirectoryForFile( output_file );
        if ( !osgDB::fileExists( osgDB::getFilePath( output_file ) ) )
        {
            osg::notify( osg::WARN ) << "Unable to establish target location for output: " << output_file << std::endl;
            continue;
        }

        // for now we just support the single slice:
        if ( layer->getSlices().size() > 0 )
        {
            BuildLayerSlice* slice = layer->getSlices().front().get();
            Script* script = slice->getScript();

            int min_res_level = slice->getMinResolutionLevel();
            int max_res_level = slice->getMaxResoltuionLevel();
            float priority_offset = 1.0f;

            PagedLayerCompiler compiler;
            osg::ref_ptr<osg::Node> output = compiler.compile(
                feature_layer.get(),
                script,
                terrain_node.get(),
                terrain_srs.get(),
                terrain_extent,
                min_res_level,
                max_res_level,
                priority_offset,
                output_file );
        }
    }

    return true;
}