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

/**
 * osggis_simple - Simple vector layer compiler
 *
 * This simple command-line utility compiles a GIS vector layer into geometry.
 */

#include <osgGIS/Registry>
#include <osgGIS/Compiler>
#include <osgGIS/Script>
#include <osgGIS/BuildGeomFilter>
#include <osgGIS/BuildNodesFilter>
#include <osgGIS/TransformFilter>

#include <osg/ArgumentParser>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/notify>
#include <osg/Group>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <iostream>

#define NOUT osg::notify(osg::NOTICE)
#define ENDL std::endl


// Variables that are set by command line arguments:
std::string input_file;
std::string output_file;
osg::Vec4f color(1,1,1,1);


int
die( const std::string& msg )
{
	osg::notify( osg::FATAL ) << "ERROR: " << msg << ENDL;
	return -1;
}


static void usage( const char* prog, const char* msg )
{
    if ( msg )
        NOUT << msg << std::endl;

    NOUT << prog << " loads a vector file (e.g., a shapefile) and generates OSG geometry." << ENDL;
    NOUT << ENDL;
    NOUT << "Usage:" << ENDL;
    NOUT << "    " << prog << " --input vector_file --output output_file [options...]" << ENDL;
    NOUT << ENDL;
    NOUT << "Required:" << ENDL;
    NOUT << "    --input <filename>     - Vector data to compile" << ENDL;
    NOUT << ENDL;
    NOUT << "Optional:"<< ENDL;
    NOUT << "    --output <filename>    - Output geometry file (omit this to launch a viewer)" << ENDL;
    NOUT << "    --color <r,g,b,a>      - Color of output geometry (0->1)" << ENDL;
    NOUT << "    --random-colors        - Randomly assign feature colors" << ENDL;
}


void
parseCommandLine( int argc, char** argv )
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    osg::ArgumentParser arguments( &argc, argv );
    
    arguments.getApplicationUsage()->setApplicationName( arguments.getApplicationName() );
    arguments.getApplicationUsage()->setDescription( arguments.getApplicationName() + " clamps vector data to a terrain dataset." );
    arguments.getApplicationUsage()->setCommandLineUsage( arguments.getApplicationName() + " [--input vector_file [options..]");
    arguments.getApplicationUsage()->addCommandLineOption( "-h or --help", "Display all available command line paramters");

    // request for help:
    if ( arguments.read( "-h" ) || arguments.read( "--help" ) )
    {
        usage( arguments.getApplicationName().c_str(), 0 );
        exit(-1);
    }

    std::string str;

    // the input vector file that we will compile:
    while( arguments.read( "--input", input_file ) );

    // output file for vector geometry:
    while( arguments.read( "--output", output_file ) );

    // Color of the output geometry:
    while( arguments.read( "--color", str ) )
        sscanf( str.c_str(), "%f,%f,%f,%f", &color.x(), &color.y(), &color.z(), &color.a() );

    while( arguments.read( "--random-colors" ) )
        color.a() = 0.0;

    // validate arguments:
    if ( input_file.length() == 0 )
    {
        arguments.getApplicationUsage()->write(
            std::cout,
            osg::ApplicationUsage::COMMAND_LINE_OPTION );
        exit(-1);
    }
}


osgGIS::Script*
createScript()
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // The Script is a series of filters that will transform the GIS
    // feature data into a scene graph.
    osgGIS::Script* script = new osgGIS::Script();

    // Construct osg::Drawable's from the incoming feature batches:
    osgGIS::BuildGeomFilter* gf = new osgGIS::BuildGeomFilter();
    gf->setColor( color );
    gf->setRandomizedColors( color.a() == 0 );
    script->appendFilter( gf );

    // Bring all the drawables into a single collection so that they
    // all fall under the same osg::Geode.
    script->appendFilter( new osgGIS::CollectionFilter() );

    // Construct a Node that contains the drawables and adjust its state set.
    script->appendFilter( new osgGIS::BuildNodesFilter( 
        osgGIS::BuildNodesFilter::CULL_BACKFACES |
        osgGIS::BuildNodesFilter::DISABLE_LIGHTING ) );

    return script;
}


int
main(int argc, char* argv[])
{
    parseCommandLine( argc, argv );

    // The osgGIS registry is the starting point for loading new featue layers
    // and creating spatial reference systems.
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // Load up the feature layer that we want to clamp to a terrain:
    NOUT << "Loading feature layer and building spatial index..." << ENDL;
    osg::ref_ptr<osgGIS::FeatureLayer> layer = registry->createFeatureLayer( input_file );
    if ( !layer.valid() )
        return die( "Failed to create feature layer." );

    // Create a script that the compiler will use to build the geometry:
    osg::ref_ptr<osgGIS::Script> script = createScript();

    // Compile the feature layer into a scene graph.
    osgGIS::Compiler compiler( layer.get(), script.get() );
    osg::ref_ptr<osg::Node> output = compiler.compile();

    if ( !output.valid() )
        return die( "Compilation failed!" );

    // Launch a viewer to see the results on the reference terrain.
    if ( output_file.length() > 0 )
    {
        osgDB::makeDirectoryForFile( output_file );
        osgDB::writeNodeFile( *(output.get()), output_file );
    }
    else
    {
        osgViewer::Viewer viewer;
        viewer.setUpViewInWindow( 10, 10, 800, 600 );
        viewer.setSceneData( output.get() );
        viewer.run();
    }

	return 0;
}

