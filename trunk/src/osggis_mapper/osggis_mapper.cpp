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
 * osggis_mapper
 *
 * A viewer application that lets you visualize an osgGIS project and query GIS
 * features. It reads in the same Project XML file that osggis_build uses to create
 * the datasets, and looks for "map" instances. You can click on GIS features and pull
 * up their attributes.
 *
 *   Usage:
 *      osggis_mapper [-f project-file]
 *
 * By default, the app will look for "project.xml" in the current directory. In the
 * project XML file, it looks for "map" definitions like:
 *
 *   <map name="default" terrain="full">
 *       <maplayer layer="buildings" searchable="true"/>
 *       <maplayer layer="streets"/>
 *   </map>
 *
 * The application will load each of the target layers specified in the map. If a layer is
 * marked as "searchable", it will connect to the GIS data source that was used to build
 * the layer, and you can them click on a feature in that layer and pull up the attribute
 * list.
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
#include <osgGIS/Registry>
#include <osgGISProjects/Project>
#include <osgGISProjects/XmlSerializer>

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

    NOUT << prog << " views an osgGIS project." << ENDL;
    NOUT << ENDL;
    NOUT << "Usage:" << ENDL;
    NOUT << "    " << prog << " [-f project-file]" << ENDL;
    NOUT << ENDL;
    NOUT << "You may also use any argument supported by osgviewer." << ENDL;
}


class FeatureLayerQueryHandler : public osgGA::GUIEventHandler
{
public:
    FeatureLayerQueryHandler( osg::Node* _root, osgGIS::FeatureLayer* _layer, osgGIS::SpatialReference* _terrain_srs ) 
        : root( _root ), layer( _layer ), terrain_srs( _terrain_srs ) { }

public:
    bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    { 
        if ( ea.getHandled() ) return false;
        if ( ea.getEventType() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON &&
            (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL) != 0 &&
            terrain_srs.valid() &&
            layer.valid() )
        {
            osgViewer::View* view = dynamic_cast<osgViewer::View*>( &aa );
            if ( !view ) return false;
            osgUtil::LineSegmentIntersector::Intersections hits;
            if ( view->computeIntersections( ea.getX(), ea.getY(), hits ) )
            {
                osgUtil::LineSegmentIntersector::Intersection& first = *hits.begin();
                osgGIS::GeoPoint world( first.getWorldIntersectPoint(), terrain_srs.get() );
                osgGIS::GeoPoint result = terrain_srs->getBasisSRS()->transform( world );

                int count = 0;
                osg::ref_ptr<osgGIS::FeatureCursor> c = layer->createCursor( osgGIS::GeoExtent( result, result ) );
                while( c->hasNext() )
                {
                    osgGIS::Feature* f = c->next();
                    if ( count++ == 0 )
                    {
                        osg::notify( osg::ALWAYS ) 
                            << result.x() << ", " << result.y() << " (" << terrain_srs->getName() << ") "
                            << " Layer " << layer->getName() << ": OIDS=";
                        osg::notify( osg::ALWAYS ) << " " << f->getOID() << std::endl;
                    }
                }
            }
        }

        return false; // never "handled"
    }

private:
    osg::ref_ptr<osg::Node> root;
    osg::ref_ptr<osgGIS::FeatureLayer> layer;
    osg::ref_ptr<osgGIS::SpatialReference> terrain_srs;
};


int
main(int argc, char* argv[])
{
    std::string project_file = "project.xml";
    std::string map_name;
    std::string temp;
    bool unlit_terrain = false;

    // Begin by parsing the command-line arguments:
    osg::ArgumentParser args( &argc, argv );

    if ( args.read( "--help" ) || args.read( "-h" ) )
    {
        usage( argv[0], "Usage" );
        exit(0);
    }

    if ( args.read( "--project-file", temp ) || args.read( "-f", temp ) )
    {
        project_file = temp;
    }

    if ( args.read( "--map", temp ) )
    {
        map_name = temp;
    }

    unlit_terrain = args.read( "--unlit-terrain" );

    
    // set up the viewer:
    osgViewer::Viewer viewer( args );
    osgGA::MatrixManipulator* manip = new osgGA::TerrainManipulator();
    viewer.setCameraManipulator( manip );
    viewer.addEventHandler( new osgViewer::ThreadingHandler() );
    viewer.addEventHandler( new osgViewer::WindowSizeHandler() );
    viewer.addEventHandler( new osgViewer::StatsHandler() );
    viewer.addEventHandler( new osgGA::StateSetManipulator( viewer.getCamera()->getOrCreateStateSet()) );


    // Load up the project file:
    osgGIS::Registry* registry = osgGIS::Registry::instance();

    osg::ref_ptr<osgGISProjects::Project> project = osgGISProjects::XmlSerializer::loadProject( project_file );
    if ( !project.valid() )
        return die( "Project file not found; exiting." );
    
    std::string workdir = project->getAbsoluteWorkingDirectory();
    if ( workdir.length() == 0 )
        workdir = PathUtils::combinePaths( project->getBaseURI(), ".osggis-" + project->getName() );
    if ( osgDB::makeDirectory( workdir ) )
        registry->setWorkDirectory( workdir );


    // Look for the specified build target. If not found, fall back on "default"; otherwise fall back on the
    // first one found.
    osg::ref_ptr<osgGISProjects::Map> map;
    if ( map_name.length() > 0 )
        map = project->getMap( map_name );
    if ( !map.valid() )
        map = project->getMap( "default" );
    if ( !map.valid() && project->getMaps().size() > 0 )
        map = project->getMaps().front();
    if ( !map.valid() )
        return die( "Unable to find a suitable target within the project file; exiting." );


    // Load up all the content by scanning the project contents:

    osg::ref_ptr<osg::Group> root = new osg::Group();


    // Start by loading up the terrain:
    osgGISProjects::Terrain* terrain = map->getTerrain();
    if ( !terrain )
        return die( "Target does not specify a valid terrain; exiting." );

    osg::Node* terrain_node = osgDB::readNodeFile( terrain->getAbsoluteURI() );
    if ( !terrain_node )
        return die( "Unable to load terrain file; exiting." );

    osg::ref_ptr<osgGIS::SpatialReference> terrain_srs =
        registry->getSRSFactory()->createSRSfromTerrain( terrain_node );

    // activate a polygon offset so that draped vector layers don't z-fight:
    terrain_node->getOrCreateStateSet()->setAttributeAndModes(
        new osg::PolygonOffset( 1.0, 1.0 ),
        osg::StateAttribute::ON );

    // Optionally disable lighting on the terrain:
    if ( unlit_terrain )
    {
        terrain_node->getOrCreateStateSet()->setMode(
            GL_LIGHTING,
            osg::StateAttribute::OFF );
    }

    root->addChild( terrain_node );


    // Now find and load each feature layer. For each feature layer, install an event handler
    // that will perform the spatial lookup when you click on the map.
    for( osgGISProjects::MapLayerList::const_iterator i = map->getMapLayers().begin(); i != map->getMapLayers().end(); i++ )
    {
        osgGISProjects::MapLayer* map_layer = i->get();
        osgGISProjects::BuildLayer* layer = map_layer->getBuildLayer();
        if ( layer )
        {
            osg::Node* node = NULL;

            if ( map_layer->getVisible() )
            {
                osg::Node* node = osgDB::readNodeFile( layer->getAbsoluteTargetPath() );
                if ( node )
                    root->addChild( node );
            }

            if ( map_layer->getSearchable() && map_layer->getSearchLayer()->getSource() )
            {
                // open a feature layer to the source data for this node:
                osgGIS::FeatureLayer* feature_layer = registry->createFeatureLayer( 
                    map_layer->getSearchLayer()->getSource()->getAbsoluteURI() );
                
                if ( !node )
                    node = terrain_node;

                if ( feature_layer )
                    viewer.addEventHandler( new FeatureLayerQueryHandler( node, feature_layer, terrain_srs.get() ) );
            }
        }
    }

       
    viewer.setSceneData( root.get() );    

    osgDB::Registry::instance()->getOrCreateSharedStateManager();

    // We must wait until after the first frame to apply the manipulator node, otherwise it will
    // use the root node instead of our terrain node:
    viewer.realize();
    viewer.frame();
    if ( terrain_node )
        manip->setNode( terrain_node );
    
    // Run until the user quits.
    viewer.run();
	return 0;
}

