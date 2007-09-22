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
 * osggis_build - Project builder
 *
 * Builds multiple geometry sets based on XML project and script definitions.
 */

#include <osgGIS/Registry>
#include <osgGIS/Script>
#include <osgGIS/PagedLayerCompiler>

#include <osgGISProjects/XmlSerializer>
#include <osgGISProjects/Project>

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
std::string project_file;
std::string input_file;
std::string terrain_file;
std::string output_file;


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

    NOUT << prog << " Builds OSG geometry based on information in an XML project file." << ENDL;
    NOUT << ENDL;
    NOUT << "Usage:" << ENDL;
    NOUT << "    " << prog << " --project xml_file" << ENDL;
    NOUT << ENDL;
    NOUT << "Required:" << ENDL;
    NOUT << "    --project <filename>   - XML script definition" << ENDL;
    NOUT << ENDL;
    NOUT << "Optional: (these may be specified in the XML project file)" << ENDL;
    NOUT << "    --terrain <filename>   - Terrain against which to build" << ENDL;
    NOUT << "    --input <filename>     - Vector geometry to build" << ENDL;
    NOUT << "    --output <filename>    - Output geometry file" << ENDL;

}


void
parseCommandLine( int argc, char** argv )
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    osg::ArgumentParser arguments( &argc, argv );
    
    arguments.getApplicationUsage()->setApplicationName( arguments.getApplicationName() );
    arguments.getApplicationUsage()->addCommandLineOption( "-h or --help", "Display all available command line paramters");

    // request for help:
    if ( arguments.read( "-h" ) || arguments.read( "--help" ) )
    {
        usage( arguments.getApplicationName().c_str(), 0 );
        exit(-1);
    }

    std::string str;

    // the project file holding the spec:
    while( arguments.read( "--project", project_file ) );

    // output file for vector geometry:
    while( arguments.read( "--output", output_file ) );

    // the input vector file that we will compile:
    while( arguments.read( "--input", input_file ) );

    // terrain file for clamping
    while( arguments.read( "--terrain", terrain_file ) );

    // validate arguments:
    if ( project_file.length() == 0 )
    {
        arguments.getApplicationUsage()->write(
            std::cout,
            osg::ApplicationUsage::COMMAND_LINE_OPTION );
        exit(-1);
    }
}


int
main(int argc, char* argv[])
{
    parseCommandLine( argc, argv );

	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // loads the project file:
    osgGISProjects::XmlSerializer ser;
    osg::ref_ptr<osgGISProjects::Document> doc = ser.load( project_file );    
    if ( !doc.valid() )
        return die( "Cannot load XML project/script file!" );

    osg::ref_ptr<osgGISProjects::Project> project = ser.readProject( doc.get() );
    if ( !project.valid() )
        return die( "XML file does not contain a valid project!" );

    //TODO
    osg::notify( osg::ALWAYS ) << "Project loaded succesfully. Exiting." << std::endl;

	return 0;
}

