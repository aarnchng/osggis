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

#include <osgGISProjects/XmlSerializer>
#include <osgGISProjects/Project>
#include <osgGISProjects/Builder>

#include <osg/ArgumentParser>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Notify>
#include <osg/Group>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <iostream>

#define NOUT osg::notify(osg::NOTICE)
#define ENDL std::endl


// Variables that are set by command line arguments:
std::string project_file = "project.xml";
std::string target_name = "";

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
    NOUT << "    " << prog << " [--project-file|-f xml_project_file] [target]" << ENDL;
    NOUT << ENDL;
    NOUT << "Required:" << ENDL;
    NOUT << "    <xml_project_file>   - XML project definition" << ENDL;
    NOUT << "    <target>             - build target in project file" << ENDL;

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

    std::string temp;

    if ( arguments.read( "--project-file", temp ) || arguments.read( "-f", temp ) )
    {
        project_file = temp;
    }

    if ( argc > 1 )
    {
        target_name = argv[argc-1];
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
        return die( "Cannot load project file " + project_file );

    osg::ref_ptr<osgGISProjects::Project> project = ser.readProject( doc.get() );
    if ( !project.valid() )
        return die( "Project file does not contain a valid project!" );

    VERBOSE_OUT << "Project \"" << project->getName() << "\" loaded." << std::endl;

    std::string base_uri = osgDB::getFilePath( project_file );
    osgGISProjects::Builder builder( project.get(), base_uri );
    builder.build( target_name );

    VERBOSE_OUT << "Done." << std::endl;
	return 0;
}

