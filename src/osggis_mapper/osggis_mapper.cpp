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
#include <osg/LightSource>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgViewer/Viewer>
#include <osgSim/OverlayNode>
#include <osgGA/TerrainManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgText/Text>
#include <osgUtil/Tessellator>
#include <osgViewer/ViewerEventHandlers>
#include <OpenThreads/Thread>
#include <iostream>
#include <sstream>
#include <osgGIS/Utils>
#include <osgGIS/Registry>
#include <osgGIS/ChangeShapeTypeFilter>
#include <osgGIS/BufferFilter>
#include <osgGIS/ClampFilter>
#include <osgGIS/TransformFilter>
#include <osgGIS/BuildGeomFilter>
#include <osgGIS/CollectionFilter>
#include <osgGIS/BuildNodesFilter>
#include <osgGIS/SceneGraphCompiler>
#include <osgGISProjects/Project>
#include <osgGISProjects/XmlSerializer>

#define NOUT osg::notify(osg::NOTICE)
#define ENDL std::endl
#define TEXT_SIZE 14.0f

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


/*****************************************************************************/

// The event handler that will process clicks and highlight features

class FeatureLayerQueryHandler : public osgGA::GUIEventHandler
{
public:
    FeatureLayerQueryHandler(
        osg::Node*                _root, 
        osgGIS::FeatureLayer*     _layer,
        osgGIS::SpatialReference* _terrain_srs, 
        osgSim::OverlayNode*      _overlay,
        osgText::Text*            _hud_text )

        : root( _root ), layer( _layer ), terrain_srs( _terrain_srs ), overlay( _overlay ), hud_text(_hud_text) { }

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
                osg::Vec3d hit = first.getWorldIntersectPoint() - first.getWorldIntersectNormal()*0.2;
                osgGIS::GeoPoint world( hit, terrain_srs.get() );
                osgGIS::GeoPoint result = terrain_srs->getGeographicSRS()->transform( world );

                osgGIS::FeatureCursor cursor = layer->getCursor( result );
                highlight( cursor );

                std::stringstream buf;

                buf << "Location: " << world.toString() << std::endl
                    << "SRS: " << terrain_srs->getName() << std::endl;
                int line_count = 2;

                for( cursor.reset(); cursor.hasNext(); )
                {
                    osgGIS::Feature* f = cursor.next();
                    osgGIS::AttributeList attrs = f->getAttributes();
                    for( osgGIS::AttributeList::const_iterator i = attrs.begin(); i != attrs.end(); i++ )
                    {
                        std::string key = i->getKey();
                        if ( key.length() > 0 )
                        {
                            buf << key << " : " << i->asString() << std::endl;
                            line_count++;
                        }
                    }
                    break;
                }

                if ( buf.str().length() == 0 )
                {
                    buf << "Control-Left-Click to query a feature";
                    line_count++;
                }
                hud_text->setText( buf.str() );
                hud_text->setPosition( osg::Vec3( 10, line_count*TEXT_SIZE*1.1f, 0 ) );
                hud_text->dirtyDisplayList();
            }
        }

        return false; // never "handled"
    }

    // Builds an overlay subgraph to project down onto the scene. This is the highlighting
    // geometry for the selected feature.
    void highlight( osgGIS::FeatureCursor& cursor )
    {
        osgGIS::FilterGraph* graph = new osgGIS::FilterGraph();

        osgGIS::ChangeShapeTypeFilter* change = new osgGIS::ChangeShapeTypeFilter();
        change->setNewShapeType( osgGIS::GeoShape::TYPE_POLYGON );
        graph->appendFilter( change );

        osgGIS::BufferFilter* buffer = new osgGIS::BufferFilter( 1.25 ); //meters
        graph->appendFilter( buffer );
        
        osgGIS::TransformFilter* xform = new osgGIS::TransformFilter();
        xform->setSRS( terrain_srs.get() );
        xform->setLocalize( true );
        graph->appendFilter( xform );

        osgGIS::BuildGeomFilter* geom = new osgGIS::BuildGeomFilter();
        geom->setColorScript( new osgGIS::Script( "vec4(1,1,0,.4)" ) );
        graph->appendFilter( geom );

        graph->appendFilter( new osgGIS::CollectionFilter() );

        osgGIS::BuildNodesFilter* nodes = new osgGIS::BuildNodesFilter();
        nodes->setDisableLighting( true );
        graph->appendFilter( nodes );

        osgGIS::SceneGraphCompiler compiler( layer.get(), graph );
        osg::Group* result = compiler.compile( cursor );

        overlay->setOverlaySubgraph( result );
        overlay->dirtyOverlayTexture();
    }

private:
    osg::ref_ptr<osg::Node> root;
    osg::ref_ptr<osgSim::OverlayNode> overlay;
    osg::ref_ptr<osgGIS::FeatureLayer> layer;
    osg::ref_ptr<osgGIS::SpatialReference> terrain_srs;
    osg::ref_ptr<osgText::Text> hud_text;
};



/*****************************************************************************/

// Creates the heads-up display that will show feature attributes and coordinates

osg::Camera*
createHUD( osgText::Text* text )
{
    osg::Camera* cam = new osg::Camera();
    cam->setProjectionMatrix( osg::Matrix::ortho2D( 0, 1280, 0, 1024 ) );
    cam->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
    cam->setViewMatrix( osg::Matrix::identity() );
    cam->setClearMask( GL_DEPTH_BUFFER_BIT );
    cam->setRenderOrder( osg::Camera::POST_RENDER );

    osg::Geode* geode = new osg::Geode();
    text->setPosition( osg::Vec3( 10, 10, 0 ) );
    text->setColor( osg::Vec4( 1, 1, 0, 1 ) );
    text->setBackdropColor( osg::Vec4( 0, 0, 0, 1 ) );
    text->setBackdropType( osgText::Text::DROP_SHADOW_BOTTOM_RIGHT );
    text->setLineSpacing( 0.1f );
    text->setFont( osgText::readFontFile( "arialbd.ttf" ) );
    text->setCharacterSize( TEXT_SIZE );
    text->setDataVariance( osg::Object::DYNAMIC );
    text->setText( "Control-Left-Click to query a feature" );
    geode->addDrawable( text );
    geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode->setDataVariance( osg::Object::DYNAMIC );
    cam->addChild( geode );

    return cam;
}


/*****************************************************************************/


int
main(int argc, char* argv[])
{
    std::string project_file = "project.xml";
    std::string map_name;
    std::string temp;
    bool unlit_terrain = false;
    osg::ref_ptr<osgText::Text> text = new osgText::Text();

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

    osg::ref_ptr<osg::Group> map_node = new osg::Group();

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

    map_node->addChild( terrain_node->asGroup()->getChild(0) );

    // construct an overlay node for highlighting:
    osgSim::OverlayNode* overlay = new osgSim::OverlayNode( osgSim::OverlayNode::OBJECT_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY );
    overlay->setOverlaySubgraph( NULL );
    overlay->setOverlayTextureSizeHint( 1024 );
    overlay->addChild( map_node.get() );

    // replicate the terrain's CSN above the overlay:
    osg::Group* root = overlay;
    if ( dynamic_cast<osg::CoordinateSystemNode*>( terrain_node ) )
    {
        root = new osg::CoordinateSystemNode( *static_cast<osg::CoordinateSystemNode*>( terrain_node ) );
        root->addChild( overlay );
    }        

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
                    map_node->addChild( node );
            }

            if ( map_layer->getSearchable() && map_layer->getSearchLayer()->getSource() )
            {
                // open a feature layer to the source data for this node:
                osgGIS::FeatureLayer* feature_layer = registry->createFeatureLayer( 
                    map_layer->getSearchLayer()->getSource()->getAbsoluteURI() );
                
                if ( !node )
                    node = terrain_node;

                if ( feature_layer )
                {
                    viewer.addEventHandler( new FeatureLayerQueryHandler( 
                        node, feature_layer, terrain_srs.get(), overlay, text.get() ) );
                }
            }
        }
    }

    // Attach the HUD for feature attribute readout:
    root->addChild( createHUD( text.get() ) );

    // configure some decent lighting
    viewer.setLightingMode( osg::View::SKY_LIGHT );
    osg::Light* light = viewer.getLight();
    light->setAmbient( osg::Vec4( .4, .4, .4, 1 ) );
    light->setDiffuse( osg::Vec4( 1, 1, .8, 1 ) );
    light->setPosition( osg::Vec4( 1, 0, 1, 0 ) );
    osg::Vec3 dir( -1, -1, -1 ); dir.normalize();
    light->setDirection( dir );

    viewer.setSceneData( root );

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

