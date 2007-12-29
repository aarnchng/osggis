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
#include <osg/Timer>
#include <osg/GLExtensions>
#include <osg/GraphicsThread>
#include <osg/LineWidth>
#include <osg/Camera>
#include <osg/TexEnv>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgViewer/Viewer>
#include <osgSim/OverlayNode>
#include <osgGA/TerrainManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <OpenThreads/Thread>
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
    NOUT << "    --overlay <filename>       - Loads a compiled feature layer as a projected overlay" << ENDL;
    NOUT << "    --point-size <num>         - Sets the point size (default = 1)" << ENDL;
    NOUT << "    --line-width <num>         - Sets the line width (default = 1)" << ENDL;
//    NOUT << "    --no-pre-compile           - Disables pre-compilation for the database pager (default = on)" << ENDL;
    NOUT << "    --polygon-offset <f,u>     - Sets the polygon offset for the terrain (default = 1,1)" << ENDL;
    NOUT << "    --frame-rate <fps>         - Sets the target frame rate, disabling VSYNC (default = 60)" << ENDL;
    NOUT << "" << ENDL;
    NOUT << "You may also use any argument supported by osgviewer." << ENDL;
}


typedef bool (APIENTRY *VSYNCFUNC)(int);
struct ToggleVsyncOperation : public osg::Operation
{
    ToggleVsyncOperation( bool _enable ) : osg::Operation( "ToggleVsync", true ), enable(_enable) { }
    bool enable;
    void operator()( osg::Object* gc ) {
        if ( osg::isGLExtensionSupported( 0, "WGL_EXT_swap_control" ) ) {
            VSYNCFUNC fp = (VSYNCFUNC)osg::getGLExtensionFuncPtr( "wglSwapIntervalEXT" );
            if ( fp ) fp( enable? 1 : 0 );
        }
    }
};




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

    double fps = 0.0;
    while( args.read( "--frame-rate", str ) )
    {
        sscanf( str.c_str(), "%lf", &fps );        
    }
    osg::Timer_t min_frame_time = (osg::Timer_t)
        ((1.0/fps)/(double)osg::Timer::instance()->getSecondsPerTick());

    bool no_pre_compile = args.read( "--no-pre-compile" );

    osg::NodeList overlays;
    while( args.read( "--overlay", str ) )
    {
        osg::Node* o_node = osgDB::readNodeFile( str );
        if ( o_node )
        {
            osgSim::OverlayNode* ov = new osgSim::OverlayNode( osgSim::OverlayNode::VIEW_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY );
            ov->setOverlaySubgraph( o_node );
            ov->setOverlayTextureSizeHint( 2048 );
            overlays.push_back( ov );
        }
    }

    osg::Node* terrain_node = NULL;
    osgGA::MatrixManipulator* manip = new osgGA::TerrainManipulator();
    for( int i=1; i<argc; i++ )
    {
        std::string arg = argv[i];
        osg::Node* node = osgDB::readNodeFile( arg );
        if ( node ) 
        {
            group->addChild( node );

            if ( !terrain_node )
            {
                terrain_node = node;
                
                terrain_node->getOrCreateStateSet()->setAttributeAndModes(
                    new osg::PolygonOffset( po_factor, po_units ),
                    osg::StateAttribute::ON );

                if ( overlays.size() > 0 )
                {
                    osg::Group* csn = terrain_node->asGroup();
                        
                    for( int i=0; i<csn->getNumChildren(); i++ )
                    {
                        for( int j=0; j<overlays.size(); j++ )
                        {
                            overlays[i].get()->asGroup()->addChild( csn->getChild( i ) );
                        }
                    }
                    csn->removeChildren( 0, csn->getNumChildren() );
                    for( int i=0; i<overlays.size(); i++ )
                    {
                        csn->addChild( overlays[i].get() );
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

    if ( no_pre_compile )
    {
        viewer.getScene()->getDatabasePager()->setDoPreCompile( false );
    }

    // for a target frame rate, disable VSYNC if possible
    if ( fps > 0.0 )
    {
        viewer.setRealizeOperation( new ToggleVsyncOperation( false ) );
    }


    viewer.realize();
    viewer.frame();
    if ( terrain_node )
        manip->setNode( terrain_node );
    
    while( !viewer.done() )
    {
        osg::Timer_t t0 = osg::Timer::instance()->tick();

        viewer.frame();

        if ( fps > 0.0 )
        {
            osg::Timer_t t1 = osg::Timer::instance()->tick();
            osg::Timer_t frame_time = t1 - t0;
            int gap_us = osg::Timer::instance()->delta_u( frame_time, min_frame_time );
            if ( gap_us > 0 )
                OpenThreads::Thread::CurrentThread()->microSleep( gap_us );
        }
    }

	return 0;
}

