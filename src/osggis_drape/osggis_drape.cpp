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
 * osggis_drape - Compiles a simple vector layer and drapes it on a terrain
 *                using a stencil buffer technique.
 */

#include <osgGIS/Registry>
#include <osgGIS/SimpleLayerCompiler>
#include <osgGIS/FilterGraph>
#include <osgGIS/BuildGeomFilter>
#include <osgGIS/BuildNodesFilter>
#include <osgGIS/TransformFilter>
#include <osgGIS/ExtrudeGeomFilter>
#include <osgGIS/BufferFilter>
#include <osgGIS/FadeHelper>
#include <osgGIS/ScriptEngine>
#include <osgGIS/Utils>

#include <osg/Stencil>
#include <osg/StencilTwoSided>
#include <osg/ArgumentParser>
#include <osgDB/FileUtils>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osg/Notify>
#include <osg/Group>
#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osgGA/StateSetManipulator>
#include <osgGA/TerrainManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osgUtil/Tessellator>
#include <osg/GLExtensions>
#include <osg/ColorMask>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/DisplaySettings>
#include <osg/BlendFunc>

#include <iostream>

#define NOUT osgGIS::notify(osg::NOTICE)
#define ENDL std::endl


// Variables that are set by command line arguments:
std::string input_file;
std::string terrain_file;
osg::Vec4f color(1,1,1,1);
bool fade_lods = false;
float buffer = 0.0;

#define EXTRUDE_SIZE 800000

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

    NOUT << prog << " loads a vector file (e.g., a shapefile) and generates OSG geometry." << ENDL;
    NOUT << ENDL;
    NOUT << "Usage:" << ENDL;
    NOUT << "    " << prog << " --input vector_file [options...]" << ENDL;
    NOUT << ENDL;
    NOUT << "Required:" << ENDL;
    NOUT << "    --terrain <filename>   - Terrain onto which to drape the geometry" << ENDL;
    NOUT << "    --input <filename>     - Vector data to drape" << ENDL;
    NOUT << ENDL;
    NOUT << "Optional:"<< ENDL;
    NOUT << "    --color <r,g,b,a>      - Color of output geometry (0->1)" << ENDL;
    NOUT << "    --random-colors        - Randomly assign feature colors" << ENDL;
}


void
parseCommandLine( int argc, char** argv )
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    osg::ArgumentParser arguments( &argc, argv );

    std::string str;

    // the input vector file that we will compile:
    while( arguments.read( "--input", input_file ) );

    // terrain:
    while( arguments.read( "--terrain", terrain_file ) );

    // Color of the output geometry:
    while( arguments.read( "--color", str ) )
        sscanf( str.c_str(), "%f,%f,%f,%f", &color.x(), &color.y(), &color.z(), &color.a() );

    while( arguments.read( "--random-colors" ) )
        color.a() = 0.0;

    //while( arguments.read( "--buffer", str ) )
    //    sscanf( str.c_str(), "%f", &buffer );

    // validate arguments:
    if ( input_file.empty() ) //|| terrain_file.empty() )
    {
        arguments.getApplicationUsage()->write(
            std::cout,
            osg::ApplicationUsage::COMMAND_LINE_OPTION );
        exit(-1);
    }
}


class BuildStencilVolumeFilter : public osgGIS::ExtrudeGeomFilter
{
public:
    BuildStencilVolumeFilter() { }
    BuildStencilVolumeFilter( const BuildStencilVolumeFilter& rhs ) {
        //TODO
    }
    virtual ~BuildStencilVolumeFilter() { }

    OSGGIS_META_FILTER( BuildStencilVolumeFilter );

protected: // FragmentFilter overrides

    virtual osgGIS::FragmentList process( osgGIS::FeatureList& input, osgGIS::FilterEnv* env )
    {
        return osgGIS::BuildGeomFilter::process( input, env );
    }

    virtual osgGIS::FragmentList process( osgGIS::Feature* input, osgGIS::FilterEnv* env )
    {
        osgGIS::FragmentList output;

        for( osgGIS::GeoShapeList::iterator j = input->getShapes().begin(); j != input->getShapes().end(); j++ )
        {
            osgGIS::GeoShape& shape = *j;
            osg::Vec4f color(1,1,1,1);

            osg::ref_ptr<osg::Geometry> walls = new osg::Geometry();
            osg::ref_ptr<osg::Geometry> top_cap = NULL;
            osg::ref_ptr<osg::Geometry> bottom_cap = NULL;

            if ( shape.getShapeType() == osgGIS::GeoShape::TYPE_POLYGON )
            {
                top_cap = new osg::Geometry();
                bottom_cap = new osg::Geometry();
                for( osgGIS::GeoPartList::iterator i = shape.getParts().begin(); i != shape.getParts().end(); i++ )
                {
                    osgGIS::GeomUtils::openPolygon( *i );
                }
            }

            if ( extrudeWallsUp( shape, env->getInputSRS(), EXTRUDE_SIZE, false, walls.get(), top_cap.get(), bottom_cap.get(), color, NULL ) )
            {     
                walls->getOrCreateStateSet()->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::OFF);

                // generate per-vertex normals
                generateNormals( walls.get() );

                osgGIS::Fragment* new_fragment = new osgGIS::Fragment( walls.get() );

                // tessellate and add the roofs if necessary:
                if ( top_cap.valid() )
                {
                    osgUtil::Tessellator tess;
                    tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
                    tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
                    tess.retessellatePolygons( *(top_cap.get()) );
                    generateNormals( top_cap.get() );
                    new_fragment->addDrawable( top_cap.get() );
                }

                if ( bottom_cap.valid() )
                {
                    osgUtil::Tessellator tess;
                    tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
                    tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
                    tess.retessellatePolygons( *(bottom_cap.get()) );
                    generateNormals( bottom_cap.get() );
                    new_fragment->addDrawable( bottom_cap.get() );
                }

                output.push_back( new_fragment );
            }   
        }

        return output;
    }        
};

#define ON_AND_PROTECTED \
    osg::StateAttribute::ON | osg::StateAttribute::PROTECTED

#define OFF_AND_PROTECTED \
    osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED
   


class BuildStencilGraphFilter : public osgGIS::NodeFilter
{
public:
    BuildStencilGraphFilter() { }
    BuildStencilGraphFilter( const BuildStencilGraphFilter& rhs ) : osgGIS::NodeFilter(rhs) { }
    virtual ~BuildStencilGraphFilter() { }

    OSGGIS_META_FILTER( BuildStencilGraphFilter );

protected:
    virtual osgGIS::AttributedNodeList process( osgGIS::AttributedNode* input, osgGIS::FilterEnv* env )
    {
        osgGIS::AttributedNodeList output;

        bool zFail = true;
        osg::Group* root = new osg::Group();
        //Just add the group
#if 0
        root->addChild( input->getNode() );
        root->getOrCreateStateSet()->setAttributeAndModes( new osg::CullFace(osg::CullFace::Mode::FRONT), osg::StateAttribute::ON);
        output.push_back( new osgGIS::AttributedNode( root, input->getAttributes() ) );        
        return output;
#endif

        bool s_EXT_stencil_wrap = osg::isGLExtensionSupported(0, "GL_EXT_stencil_wrap");
        bool s_EXT_stencil_two_side = osg::isGLExtensionSupported(0, "GL_EXT_stencil_two_side");

        osg::notify(osg::NOTICE) << "Stencil buffer wrap = " << s_EXT_stencil_wrap << std::endl;

        int priority = 1000;

        if ( s_EXT_stencil_two_side )
        {
            osg::notify(osg::NOTICE) << "Two-sided stenciling" << std::endl;

            osg::StencilTwoSided::Operation incrOp = s_EXT_stencil_wrap ? osg::StencilTwoSided::INCR_WRAP : osg::StencilTwoSided::INCR;
            osg::StencilTwoSided::Operation decrOp = s_EXT_stencil_wrap ? osg::StencilTwoSided::DECR_WRAP : osg::StencilTwoSided::DECR;
            osg::Group* stencil_group = new osg::Group();
            osg::StateSet* ss = stencil_group->getOrCreateStateSet();
            ss->setRenderBinDetails( priority++, "RenderBin" );
            
            if ( zFail )
            {
                osg::StencilTwoSided* stencil = new osg::StencilTwoSided();
                stencil->setFunction(osg::StencilTwoSided::BACK, osg::StencilTwoSided::ALWAYS, 1, ~0u); 
                stencil->setOperation(osg::StencilTwoSided::BACK, osg::StencilTwoSided::KEEP, incrOp, osg::StencilTwoSided::KEEP);

                stencil->setFunction(osg::StencilTwoSided::FRONT, osg::StencilTwoSided::ALWAYS, 1, ~0u); 
                stencil->setOperation(osg::StencilTwoSided::FRONT, osg::StencilTwoSided::KEEP, decrOp, osg::StencilTwoSided::KEEP);
                ss->setAttributeAndModes( stencil, ON_AND_PROTECTED );
            }
            else
            {
                osg::StencilTwoSided* stencil = new osg::StencilTwoSided();
                stencil->setFunction(osg::StencilTwoSided::FRONT, osg::StencilTwoSided::ALWAYS, 1, ~0u); 
                stencil->setOperation(osg::StencilTwoSided::FRONT, osg::StencilTwoSided::KEEP, osg::StencilTwoSided::KEEP, incrOp);

                stencil->setFunction(osg::StencilTwoSided::BACK, osg::StencilTwoSided::ALWAYS, 1, ~0u); 
                stencil->setOperation(osg::StencilTwoSided::BACK, osg::StencilTwoSided::KEEP, osg::StencilTwoSided::KEEP, decrOp);
                ss->setAttributeAndModes( stencil, ON_AND_PROTECTED );
            }

            ss->setAttributeAndModes(new osg::ColorMask(false,false,false,false), ON_AND_PROTECTED);
            ss->setAttributeAndModes(new osg::Depth(osg::Depth::LESS,0,1,false), ON_AND_PROTECTED);
            stencil_group->addChild( input->getNode() );

            root->addChild( stencil_group );
        }
        else
        {
            osg::notify(osg::NOTICE) << "One-sided stenciling" << std::endl;
            
            if ( !zFail )  // Z-Pass
            {
                osg::notify( osg::NOTICE) << "ZPass" << std::endl;

                osg::Group* front_group = new osg::Group();
                osg::StateSet* front_ss = front_group->getOrCreateStateSet();
                front_ss->setRenderBinDetails( priority++, "RenderBin" );

                // incrementing stencil op for front faces:
                osg::Stencil* front_stencil = new osg::Stencil();
                front_stencil->setFunction( osg::Stencil::ALWAYS, 1, ~0u );
                Stencil::Operation incrOp = s_EXT_stencil_wrap ? Stencil::INCR_WRAP : Stencil::INCR;
                front_stencil->setOperation( osg::Stencil::KEEP, osg::Stencil::KEEP, incrOp );
                front_ss->setAttributeAndModes( front_stencil, ON_AND_PROTECTED );

                front_ss->setAttributeAndModes( new osg::ColorMask(false,false,false,false), ON_AND_PROTECTED);
                front_ss->setAttributeAndModes( new osg::CullFace(osg::CullFace::BACK), ON_AND_PROTECTED);
                front_ss->setAttributeAndModes( new osg::Depth(osg::Depth::LESS,0,1,false), ON_AND_PROTECTED);

                front_group->addChild( input->getNode() );
                root->addChild( front_group );

                // decrementing stencil op for back faces:
                osg::Group* back_group = new osg::Group();
                osg::StateSet* back_ss = back_group->getOrCreateStateSet();
                back_ss->setRenderBinDetails( priority++, "RenderBin" );

                // incrementing stencil op for front faces:
                osg::Stencil* back_stencil = new osg::Stencil();
                back_stencil->setFunction( osg::Stencil::ALWAYS, 1, ~0u );
                Stencil::Operation decrOp = s_EXT_stencil_wrap ? Stencil::DECR_WRAP : Stencil::DECR;
                back_stencil->setOperation( osg::Stencil::KEEP, osg::Stencil::KEEP, decrOp );
                back_ss->setAttributeAndModes( back_stencil, ON_AND_PROTECTED );

                back_ss->setAttributeAndModes( new osg::ColorMask(false,false,false,false), ON_AND_PROTECTED);
                back_ss->setAttributeAndModes( new osg::CullFace(osg::CullFace::FRONT), ON_AND_PROTECTED);
                back_ss->setAttributeAndModes( new osg::Depth(osg::Depth::LESS,0,1,false), ON_AND_PROTECTED);

                back_group->addChild( input->getNode() );
                root->addChild( back_group );       
            }
            else
            {
                osg::notify( osg::NOTICE) << "ZFail" << std::endl;

                osg::Group* front_group = new osg::Group();
                osg::StateSet* front_ss = front_group->getOrCreateStateSet();
                front_ss->setRenderBinDetails( priority++, "RenderBin" );

                // incrementing stencil op for back faces:
                osg::Stencil* front_stencil = new osg::Stencil();
                front_stencil->setFunction( osg::Stencil::ALWAYS ); //, 1, ~0u );
                Stencil::Operation incrOp = s_EXT_stencil_wrap ? Stencil::INCR_WRAP : Stencil::INCR;
                front_stencil->setOperation( osg::Stencil::KEEP, incrOp, osg::Stencil::KEEP );
                front_ss->setAttributeAndModes( front_stencil, ON_AND_PROTECTED );

                front_ss->setAttributeAndModes( new osg::ColorMask(false,false,false,false), ON_AND_PROTECTED);
                front_ss->setAttributeAndModes( new osg::CullFace(osg::CullFace::FRONT), ON_AND_PROTECTED);
                front_ss->setAttributeAndModes( new osg::Depth(osg::Depth::LESS,0,1,false), ON_AND_PROTECTED);

                front_group->addChild( input->getNode() );
                root->addChild( front_group );

                // decrementing stencil buf for front faces
                osg::Group* back_group = new osg::Group();
                osg::StateSet* back_ss = back_group->getOrCreateStateSet();
                back_ss->setRenderBinDetails( priority++, "RenderBin" );

                osg::Stencil* back_stencil = new osg::Stencil();
                back_stencil->setFunction( osg::Stencil::ALWAYS ); //, 1, ~0u );
                Stencil::Operation decrOp = s_EXT_stencil_wrap ? Stencil::DECR_WRAP : Stencil::DECR;
                back_stencil->setOperation( osg::Stencil::KEEP, decrOp, osg::Stencil::KEEP );
                back_ss->setAttributeAndModes( back_stencil, ON_AND_PROTECTED );

                back_ss->setAttributeAndModes( new osg::ColorMask(false,false,false,false), ON_AND_PROTECTED);
                back_ss->setAttributeAndModes( new osg::CullFace(osg::CullFace::BACK), ON_AND_PROTECTED);
                back_ss->setAttributeAndModes( new osg::Depth(osg::Depth::LESS,0,1,false), ON_AND_PROTECTED);

                back_group->addChild( input->getNode() );
                root->addChild( back_group );
            }
        }

        // make a full screen quad:
        /*osg::Geometry* quad = new osg::Geometry();
        osg::Vec3Array* verts = new osg::Vec3Array(4);
        (*verts)[0].set( 0, 1, 0 );
        (*verts)[1].set( 0, 0, 0 );
        (*verts)[2].set( 1, 0, 0 );
        (*verts)[3].set( 1, 1, 0 );
        quad->setVertexArray( verts );
        osg::Vec4Array* colors = new osg::Vec4Array(1);
        (*colors)[0].set( 1, 1, 1, 1 );
        quad->setColorArray( colors );
        quad->setColorBinding( osg::Geometry::BIND_OVERALL );
        quad->addPrimitiveSet( new osg::DrawArrays( osg::PrimitiveSet::QUADS, 0, 4 ) );
        osg::Geode* quad_geode = new osg::Geode();
        quad_geode->addDrawable( quad );

        osg::StateSet* quad_ss = quad->getOrCreateStateSet();
        quad_ss->setRenderBinDetails( priority++, "RenderBin" );
        quad_ss->setMode( GL_CULL_FACE, OFF_AND_PROTECTED );
        quad_ss->setMode( GL_DEPTH_TEST, OFF_AND_PROTECTED );
        quad_ss->setMode( GL_LIGHTING, OFF_AND_PROTECTED );

        osg::BlendFunc* trans = new osg::BlendFunc;
        trans->setFunction(osg::BlendFunc::ONE,osg::BlendFunc::ONE);
        quad_ss->setAttributeAndModes( trans );

        osg::Stencil* quad_stencil = new osg::Stencil();
        quad_stencil->setFunction( osg::Stencil::NOTEQUAL, 128, (unsigned int)~0 );
        quad_stencil->setOperation( osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::KEEP );
        quad_ss->setAttributeAndModes( quad_stencil, ON_AND_PROTECTED );

        osg::MatrixTransform* abs = new osg::MatrixTransform();
        abs->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
        abs->setMatrix( osg::Matrix::identity() );
        abs->addChild( quad_geode );

        osg::Projection* proj = new osg::Projection();
        proj->setMatrix( osg::Matrix::ortho(0, 1, 0, 1, 0, -1) );
        proj->addChild( abs );

        root->addChild( proj );*/

        osg::Group* finalGroup = new osg::Group;
        finalGroup->addChild( input->getNode() );
        osg::StateSet* finalGroup_ss = finalGroup->getOrCreateStateSet();
        finalGroup_ss->setRenderBinDetails( priority++, "RenderBin" );
        //finalGroup_ss->setMode( GL_CULL_FACE, OFF_AND_PROTECTED );
        //finalGroup_ss->setMode( GL_DEPTH_TEST, OFF_AND_PROTECTED );
        finalGroup_ss->setMode( GL_LIGHTING, OFF_AND_PROTECTED );
        finalGroup_ss->setAttributeAndModes( new osg::CullFace(osg::CullFace::BACK), ON_AND_PROTECTED);

        osg::Stencil* finalGroup_stencil = new osg::Stencil();
        finalGroup_stencil->setFunction( osg::Stencil::NOTEQUAL, 128, (unsigned int)~0 );
        finalGroup_stencil->setOperation( osg::Stencil::KEEP, osg::Stencil::KEEP, osg::Stencil::KEEP );
        finalGroup_ss->setAttributeAndModes( finalGroup_stencil, ON_AND_PROTECTED );
        root->addChild( finalGroup );


        root->getOrCreateStateSet()->setMode( GL_BLEND, 1 );

        //osg::Camera* cam = new osg::Camera();
        //cam->setRenderOrder( osg::Camera::POST_RENDER );
        //cam->setClearMask( GL_STENCIL_BUFFER_BIT );
        //cam->setClearStencil( 128 );
        //cam->addChild( root );
        //cam->getOrCreateStateSet()->setRenderBinDetails( 1000, "RenderBin" );

        output.push_back( new osgGIS::AttributedNode( root, input->getAttributes() ) );        

        return output;
    }
};


osgGIS::FilterGraph*
createFilterGraph()
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // The FilterGraph is a series of filters that will transform the GIS
    // feature data into a scene graph.
    osgGIS::FilterGraph* graph = new osgGIS::FilterGraph();

    // First, sink the features so they make a bottom cap.
    osgGIS::TransformFilter* sink = new osgGIS::TransformFilter();
    sink->setMatrix( osg::Matrix::translate( 0, 0, -EXTRUDE_SIZE/2 ) );
    graph->appendFilter( sink );

    // Buffer if necessary:
    if ( buffer != 0.0f )
        graph->appendFilter( new osgGIS::BufferFilter( buffer ) );

    osgGIS::TransformFilter* xform = new osgGIS::TransformFilter();
    xform->setUseTerrainSRS( true );
    xform->setLocalize( true );
    graph->appendFilter( xform );

    // Construct osg::Drawable's from the incoming feature batches:
    //osgGIS::BuildGeomFilter* gf = new osgGIS::BuildGeomFilter();
    //gf->setColor( color );
    //if ( color.a() == 0 )
    //    gf->setColorScript( new osgGIS::Script( "vec4(math.random(),math.random(),math.random(),1)" ) );
    //graph->appendFilter( gf );

    BuildStencilVolumeFilter* stencil = new BuildStencilVolumeFilter();
    graph->appendFilter( stencil );

    // Bring all the drawables into a single collection so that they
    // all fall under the same osg::Geode.
    graph->appendFilter( new osgGIS::CollectionFilter() );

    // Construct a Node that contains the drawables and adjust its state set.
    osgGIS::BuildNodesFilter* bnf = new osgGIS::BuildNodesFilter();
    bnf->setCullBackfaces( false );
    //bnf->setDisableLighting( true );

    bnf->setAlphaBlending( true );
    bnf->setOptimize( false );

    graph->appendFilter( bnf );

    BuildStencilGraphFilter* sgf = new BuildStencilGraphFilter();
    graph->appendFilter( sgf );

    return graph;
}

int
main(int argc, char* argv[])
{
    parseCommandLine( argc, argv );

    // The osgGIS registry is the starting point for loading new featue layers
    // and creating spatial reference systems.
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // load up the terrain:
    NOUT << "Loading terrain..." << ENDL;
    osg::ref_ptr<osg::Node> terrain = osgDB::readNodeFile( terrain_file );
    //if ( !terrain.valid() )
    //    return die( "Failed to load terrain." );

    // Load up the feature layer:
    NOUT << "Loading feature layer and building spatial index..." << ENDL;
    osg::ref_ptr<osgGIS::FeatureLayer> layer = registry->createFeatureLayer( input_file );
    if ( !layer.valid() )
        return die( "Failed to create feature layer." );

    // Create a graph that the compiler will use to build the geometry:
    osg::ref_ptr<osgGIS::FilterGraph> graph = createFilterGraph();

    osgGIS::SimpleLayerCompiler compiler;
    if ( terrain.valid() )
        compiler.setTerrain( terrain.get(), osgGIS::Registry::SRSFactory()->createSRSfromTerrain( terrain.get() ) );
    osgGIS::FeatureCursor cursor = layer->getCursor();
    osg::ref_ptr<osg::Node> output = compiler.compile( layer.get(), cursor, graph.get() );
    if ( !output.valid() )
        return die( "Compilation failed!" );

    osg::Group* root = new osg::Group();
    root->addChild( output.get() );
    if ( terrain.valid() )
        root->addChild( terrain.get() );

    
    osg::DisplaySettings::instance()->setMinimumNumStencilBits( 8 );

    osgViewer::Viewer viewer;
    viewer.setUpViewInWindow( 10, 10, 1024, 768 );
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator( new osgGA::TerrainManipulator() );
    viewer.setSceneData( root );
    viewer.getCamera()->setClearStencil(128);
    viewer.getCamera()->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    viewer.run();

	return 0;
}

