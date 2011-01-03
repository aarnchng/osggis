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
#include <osg/Timer>
#include <osg/GLExtensions>
#include <osg/GraphicsThread>
#include <osg/LineWidth>
#include <osg/Camera>
#include <osg/TexEnv>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/CullFace>
#include <osg/Version>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgViewer/Viewer>
#include <osgSim/OverlayNode>
#include <osgGA/TerrainManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/Tessellator>
#include <osgViewer/ViewerEventHandlers>
#include <OpenThreads/Thread>
#include <iostream>
#include <osgGIS/Utils>

#define NOUT osgGIS::notify(osg::NOTICE)
#define ENDL std::endl

#if OSG_MIN_VERSION_REQUIRED(2,9,8)
#include <osgGA/CameraManipulator>
namespace osgGA {
    typedef CameraManipulator MatrixManipulator;
};
#else
#include <osgGA/MatrixManipulator>
#endif


int
die( const std::string& msg )
{
	osgGIS::notify( osg::FATAL ) << "ERROR: " << msg << ENDL;
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
    NOUT << "    --unlit-terrain            - Disables lighting on the first model loaded (the terrain)" << ENDL;
    NOUT << "    --overlay <filename>       - Loads a compiled feature layer as a projected overlay" << ENDL;
    NOUT << "" << ENDL;
    NOUT << "You may also use any argument supported by osgviewer." << ENDL;
}


int
main(int argc, char* argv[])
{
    osg::ArgumentParser args( &argc, argv );

    osgDB::Registry::instance()->getOrCreateSharedStateManager();

    osgViewer::Viewer viewer( args );

    osg::ref_ptr<osg::Group> group = new osg::Group();
    std::string str;

    if ( args.read( "--help" ) || args.read( "-h" ) )
    {
        usage( argv[0], "Usage" );
        exit(0);
    }

    bool unlit_terrain = args.read( "--unlit-terrain" );
    osg::Node* terrain_node = NULL;
    bool file_is_overlay = false;

    osgGA::MatrixManipulator* manip = new osgGA::TerrainManipulator();
    for( int i=1; i<argc; i++ )
    {
        std::string arg = argv[i];

        if ( arg == "--overlay" )
        {
            file_is_overlay = true;
            continue;
        }

        osg::Node* node = osgDB::readNodeFile( arg );
        if ( node ) 
        {
            if ( file_is_overlay )
            {
                // an "overlay" graph will decorate the previously graph only:
                osgSim::OverlayNode* ov = new osgSim::OverlayNode( osgSim::OverlayNode::OBJECT_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY );
                ov->setOverlaySubgraph( node );
                ov->setOverlayTextureSizeHint( 1024 );
                if ( group->getNumChildren() > 0 )
                {
                    osg::Node* last_child = group->getChild( group->getNumChildren()-1 );
                    ov->addChild( last_child );
                    group->replaceChild( last_child, ov );
                }
                file_is_overlay = false;
            }

            else
            {
                group->addChild( node );

                // Consider the first loaded file to be the terrain:
                if ( !terrain_node )
                {
                    terrain_node = node;
                    
                    // Apply a polygon offset so that clamped vector layers display properly:
                    terrain_node->getOrCreateStateSet()->setAttributeAndModes(
                        new osg::PolygonOffset( 1.0, 1.0 ),
                        osg::StateAttribute::ON );

                    terrain_node->getOrCreateStateSet()->setAttributeAndModes(
                        new osg::CullFace(),
                        osg::StateAttribute::ON );

                    terrain_node->getOrCreateStateSet()->setMode( GL_BLEND, osg::StateAttribute::ON );

                    // Optionally disable lighting on the terrain:
                    if ( unlit_terrain )
                    {
                        terrain_node->getOrCreateStateSet()->setMode(
                            GL_LIGHTING,
                            osg::StateAttribute::OFF );
                    }
                }
            }
        }
    }

                   
    viewer.setSceneData( group.get() );    
    viewer.setCameraManipulator( manip );
    
    viewer.addEventHandler( new osgViewer::ThreadingHandler() );
    viewer.addEventHandler( new osgViewer::WindowSizeHandler() );
    viewer.addEventHandler( new osgViewer::StatsHandler() );
    viewer.addEventHandler( new osgGA::StateSetManipulator( viewer.getCamera()->getOrCreateStateSet()) );

    viewer.realize();
    viewer.frame();

    // Must wait until after the first frame to do this, otherwise it doesn't work
    if ( terrain_node )
        manip->setNode( terrain_node );
    
    viewer.run();

	return 0;
}

