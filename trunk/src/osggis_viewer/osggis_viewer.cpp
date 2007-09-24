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
 * osggis_viewer
 *
 * A simple viewer that lets you specify polygon offset, point size, and 
 * line width on the command line. Handy for previewing compiled vector
 * layers atop a terrain.
 *
 * Make your terrain the first model you load on the command line. By default,
 * this utility will apply a polygon offset of (1,1) to the first model loaded.
 */

#include <osg/ArgumentParser>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Notify>
#include <osg/Group>
#include <osg/PolygonOffset>
#include <osg/Point>
#include <osg/LineWidth>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgGA/TerrainManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>

#define NOUT osg::notify(osg::NOTICE)
#define ENDL std::endl


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

    NOUT << prog << " views feature geometry atop a terrain." << ENDL;
    NOUT << ENDL;
    NOUT << "Usage:" << ENDL;
    NOUT << "    " << prog << " [input files...] [options...]" << ENDL;
    NOUT << ENDL;
    NOUT << "Required:" << ENDL;
    NOUT << "    [terrain file] [feature file...]  - Files to view (put terrain first for polygon offset)" << ENDL;
    NOUT << ENDL;
    NOUT << "Optional:"<< ENDL;
    NOUT << "    --point-size <num>         - Sets the point size (default = 1)" << ENDL;
    NOUT << "    --line-width <num>         - Sets the line width (default = 1)" << ENDL;
    NOUT << "    --pre-compile              - Sets pre-compilation ON for the database pager (default = off)" << ENDL;
    NOUT << "    --polygon-offset <f,u>     - Sets the polygon offset for the terrain (default = 1,1)" << ENDL;
    NOUT << "" << ENDL;
    NOUT << "You may also use any argument supported by osgviewer." << ENDL;
}


int
main(int argc, char* argv[])
{
    osg::ArgumentParser args( &argc, argv );

    osgViewer::Viewer viewer( args );

    osg::ref_ptr<osg::Group> group = new osg::Group();
    std::string str;

    if ( args.read( "--help" ) || args.read( "-h" ) )
    {
        usage( argv[0], "Usage" );
        exit(0);
    }
    
    // A polygon offset for the terrain (first file loaded)
    float po_units = 1.0f;
    float po_factor = 1.0f;
    while( args.read( "--polygon-offset", str ) )
    {
        sscanf( str.c_str(), "%f,%f", &po_factor, &po_units );
    }

    // opengl point size
    while( args.read( "--point-size", str ) )
    {
        float point_size = 1.0f;    
        sscanf( str.c_str(), "%f", &point_size );
        osg::Point* point = new osg::Point();
        point->setSize( point_size );
        group->getOrCreateStateSet()->setAttribute( point, osg::StateAttribute::ON );
    }

    while( args.read( "--line-width", str ) )
    {
        float line_width = 1.0f;
        sscanf( str.c_str(), "%f", &line_width );
        group->getOrCreateStateSet()->setAttribute( 
            new osg::LineWidth( line_width ), osg::StateAttribute::ON );
    }

    bool pre_compile = args.read( "--pre-compile" );

    osgGA::MatrixManipulator* manip = new osgGA::TerrainManipulator();
    for( int i=1; i<argc; i++ )
    {
        std::string arg = argv[i];
        osg::Node* node = osgDB::readNodeFile( arg );
        if ( node ) 
        {
            group->addChild( node );
            if ( i == 1 )
            {
                manip->setNode( node );

                node->getOrCreateStateSet()->setAttributeAndModes(
                    new osg::PolygonOffset( po_factor, po_units ),
                    osg::StateAttribute::ON );
            }
        }
    }

    viewer.setSceneData( group.get() );
    viewer.setCameraManipulator( manip );
    
    viewer.addEventHandler( new osgViewer::ThreadingHandler() );
    viewer.addEventHandler( new osgViewer::WindowSizeHandler() );
    viewer.addEventHandler( new osgViewer::StatsHandler() );
    viewer.addEventHandler( new osgGA::StateSetManipulator(
        viewer.getCamera()->getOrCreateStateSet()) );

    viewer.getScene()->getDatabasePager()->setDoPreCompile( pre_compile );

    viewer.run();

	return 0;
}

