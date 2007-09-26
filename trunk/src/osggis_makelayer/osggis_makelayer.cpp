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

/*
 * osggis_makelayer
 *
 * Compiles a vector layer against a PagedLOD reference terrain that was
 * built with VirtualPlanetBuilder/osgdem.
 *
 * Demonstrates:
 *  - How to connect to a feature layer
 *  - How to build a script and use various filters
 *  - How to use the PagedLayerCompiler
 */

#include <osgGIS/Registry>
#include <osgGIS/FeatureStore>
#include <osgGIS/Compiler>
#include <osgGIS/Script>
#include <osgGIS/CropFilter>
#include <osgGIS/TransformFilter>
#include <osgGIS/ChangeShapeTypeFilter>
#include <osgGIS/DecimateFilter>
#include <osgGIS/BuildGeomFilter>
#include <osgGIS/ExtrudeGeomFilter>
#include <osgGIS/BuildNodesFilter>
#include <osgGIS/RemoveHolesFilter>
#include <osgGIS/CollectionFilter>
#include <osgGIS/ConvexHullFilter>
#include <osgGIS/ClampFilter>
#include <osgGIS/PagedLayerCompiler>

#include <osg/ArgumentParser>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Notify>
#include <osg/Group>
#include <osg/PolygonOffset>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>



#define NOUT osg::notify(osg::NOTICE)
#define ENDL std::endl


// Variables that are set by command line arguments:
std::string input_file;
std::string output_file;
std::string terrain_file;
float range_far = -1;
float range_near = -1;
int min_level = -1;
int max_level = -1;
bool make_paged_lods = false;
osg::Vec4f color(1,1,1,1);
bool include_grid = false;
bool preview = false;
osgGIS::GeoExtent terrain_extent( 
        -180, -90, 180, 90, 
        osgGIS::Registry::instance()->getSRSFactory()->createWGS84() );
osgGIS::GeoShape::ShapeType shape_type = osgGIS::GeoShape::TYPE_UNSPECIFIED;
bool lighting = true;
bool geocentric = false;
bool extrude = false;
double extrude_height = -1;
bool extrude_range = false;
double extrude_min_height = -1;
double extrude_max_height = -1;
std::string extrude_height_attr;
double extrude_height_scale = 1.0;
double decimate_threshold = 0.0;
float priority_offset = 0.0;
bool convex_hull = false;



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

    NOUT << prog << " takes a vector file (e.g., a shapefile) and clamps it to a reference terrain that was built with osgdem/VPB." << ENDL;
    NOUT << ENDL;
    NOUT << "Usage:" << ENDL;
    NOUT << "    " << prog << " --input vector_file --terrain terrain_file --output output_file [options...]" << ENDL;
    NOUT << ENDL;
    NOUT << "Required:" << ENDL;
    NOUT << "    --input <filename>       - Vector data to clamp to the terrain" << ENDL;
    NOUT << "    --terrain <filename>     - Terrain data file to which to clamp vectors" << ENDL;
    NOUT << "    --output <filename>      - Where to store clamped geometry" << ENDL;
    NOUT << ENDL;
    NOUT << "Optional:"<< ENDL;
    NOUT << "    --terrain_extent <long_min,lat_min,long_max,lat_max>" << ENDL;
    NOUT << "                               - Extent of terrain in long/lat degrees (default is whole earth)" << ENDL;
    NOUT << "    --geocentric               - Generate geocentric output geometry to match a globe" << ENDL;
    NOUT << "    --min-level                - Minimum PagedLOD level for which to generate geometry" << ENDL;
    NOUT << "    --max-level                - Maximum PagedLOD level for which to generate geometry" << ENDL;
    NOUT << "    --points                   - Treat the vector data as points" << ENDL;
    NOUT << "    --lines                    - Treat the vector data as lines" << ENDL;
    NOUT << "    --polygons                 - Treat the vector data as polygons" << ENDL;
    NOUT << "    --extrude-height <num>     - Extrude shapes to this height above the terrain" << ENDL;
    NOUT << "    --extrude-range <min,max>  - Randomly extrude shapes to heights in this range" << ENDL;
    NOUT << "    --extrude-attr <name>      - Attribute whose value holds the extrusion height" << ENDL;
    NOUT << "    --extrude-scale <num>      - Multiply extrusion height by this scale factor" << ENDL;
    NOUT << "    --decimate <num>           - Decimate feature shapes to this threshold" << ENDL;
    NOUT << "    --convex-hull              - Replace feature data with its convex hull" << ENDL;
    NOUT << "    --near-lod <num>           - Near LOD range for output geometry (not yet implemented)" << ENDL;
    NOUT << "    --far-lod <num>            - Far LOD range for output geometry (not yet implemented)" << ENDL;
    NOUT << "    --priority-offset <num>    - Paging priority of vectors relative to terrain tiles (default = 0)" << ENDL;
    NOUT << "    --color <r,g,b,a>          - Color of output geometry (0->1)" << ENDL;
    NOUT << "    --random-colors            - Randomly assign feature colors" << ENDL;
    NOUT << "    --include-grid             - Includes geometry for the PagedLOD grid structure" << ENDL;
    NOUT << "    --no-lighting              - Disables lighting on the output geometry; good for points and lines" << ENDL;
}


void
parseCommandLine( int argc, char** argv )
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    osg::ArgumentParser arguments( &argc, argv );
    
    arguments.getApplicationUsage()->setApplicationName( arguments.getApplicationName() );
    arguments.getApplicationUsage()->setDescription( arguments.getApplicationName() + " clamps vector data to a terrain dataset." );
    arguments.getApplicationUsage()->setCommandLineUsage( 
        arguments.getApplicationName() + " [options] --input vector_file --terrain terrain_file --output output_file");
    arguments.getApplicationUsage()->addCommandLineOption( "-h or --help", "Display all available command line paramters");

    // request or help:
    if ( arguments.read( "-h" ) || arguments.read( "--help" ) )
    {
        usage( arguments.getApplicationName().c_str(), 0 );
        exit(-1);
    }

    std::string str;

    while( arguments.read( "--input", input_file ) );

    while( arguments.read( "--terrain", terrain_file ) );

    while( arguments.read( "--output", output_file ) );

    while( arguments.read( "--geocentric" ) )
        geocentric = true;

    while( arguments.read( "--near-lod", str ) )
        sscanf( str.c_str(), "%f", &range_near );
    
    while( arguments.read( "--far-lod", str ) )
        sscanf( str.c_str(), "%f", &range_far );

    while( arguments.read( "--points" ) )
        shape_type = osgGIS::GeoShape::TYPE_POINT;

    while( arguments.read( "--lines" ) )
        shape_type = osgGIS::GeoShape::TYPE_LINE;

    while( arguments.read( "--polygons" ) )
        shape_type = osgGIS::GeoShape::TYPE_POLYGON;

    while( arguments.read( "--min-level", str ) )
        sscanf( str.c_str(), "%d", &min_level );

    while( arguments.read( "--max-level", str ) )
        sscanf( str.c_str(), "%d", &max_level );

    while( arguments.read( "--priority-offset", str ) )
        sscanf( str.c_str(), "%f", &priority_offset );

    while( arguments.read( "--convex-hull" ) )
        convex_hull = true;

    double xmin = 0.0, xmax = 0.0, ymin = 0.0, ymax = 0.0;
    while( arguments.read( "--terrain-extent", str ) )
    {
        if ( sscanf( str.c_str(), "%f,%f,%f,%f", &xmin, &ymin, &xmax, &ymax ) == 4 )
        {
            terrain_extent = osgGIS::GeoExtent(
                xmin, ymin, xmax, ymax, 
                registry->getSRSFactory()->createWGS84() );
        }
    }

    while( arguments.read( "--color", str ) )
        sscanf( str.c_str(), "%f,%f,%f,%f", &color.x(), &color.y(), &color.z(), &color.a() );

    while( arguments.read( "--random-colors" ) )
        color.a() = 0.0;

    while( arguments.read( "--extrude-height", str ) ) {
        extrude = true;
        sscanf( str.c_str(), "%lf", &extrude_height );
    }

    while( arguments.read( "--extrude-range", str ) ) {
        extrude = true;
        extrude_range = true;
        sscanf( str.c_str(), "%lf,%lf", &extrude_min_height, &extrude_max_height );
    }

    while( arguments.read( "--extrude-attr", extrude_height_attr ) ) {
        extrude = true;
        extrude_range = false;
    }

    while( arguments.read( "--extrude-scale", str ) )
        sscanf( str.c_str(), "%lf", &extrude_height_scale );

    while( arguments.read( "--decimate", str ) )
        sscanf( str.c_str(), "%lf", &decimate_threshold );

    while( arguments.read( "--include-grid" ) )
        include_grid = true;

    while( arguments.read( "--no-lighting" ) )
        lighting = false;


    // validate required arguments:
    if (input_file.length() == 0 ||
        terrain_file.length() == 0 ||
        output_file.length() == 0 )
    {
        arguments.getApplicationUsage()->write(
            std::cout,
            osg::ApplicationUsage::COMMAND_LINE_OPTION );
        exit(-1);
    }
}



osgGIS::Script*
createScript(const osgGIS::SpatialReference* terrain_srs )
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // The "script" is a series of filters that will transform the GIS
    // feature data into a scene graph.
    osgGIS::Script* script = new osgGIS::Script();
    
    // Treat the incoming feature data as lines:
    if ( shape_type != osgGIS::GeoShape::TYPE_UNSPECIFIED )
    {
        script->appendFilter( new osgGIS::ChangeShapeTypeFilter(
            shape_type ) );
    }

    // Remove holes in polygons as an optimization
    //script->appendFilter( new osgGIS::RemoveHolesFilter() );

    // Replace features with convex hull
    if ( convex_hull )
    {
        script->appendFilter( new osgGIS::CollectionFilter() );
        script->appendFilter( new osgGIS::ConvexHullFilter() );
    }

    // Crop the feature data to the compiler environment's working extent:
    script->appendFilter( new osgGIS::CropFilter( 
        include_grid? osgGIS::CropFilter::SHOW_CROP_LINES : 0 ) );

    // Transform the features to the target spatial reference system,
    // localizing them to a local origin:
    script->appendFilter( new osgGIS::TransformFilter( 
        terrain_srs,
        osgGIS::TransformFilter::LOCALIZE ) );

    // Decimate shapes to a point-to-point-distance threshold. Doing this
    // after the transform means we're dealing in meters.
    if ( decimate_threshold > 0.0 )
    {
        script->appendFilter( new osgGIS::DecimateFilter( 
            decimate_threshold ) );
    }

    // Adjust the spatial data so that it conforms to the reference
    // terrain skin:
    script->appendFilter( new osgGIS::ClampFilter() );

    // Construct osg::Drawable's from the incoming feature batches:
    if ( extrude )
    {
        osgGIS::ExtrudeGeomFilter* gf = new osgGIS::ExtrudeGeomFilter();
        gf->setColor( color );
        gf->setRandomizeColors( color.a() == 0 );
        if ( extrude_height_attr.length() > 0 ) {
            gf->setHeightAttribute( extrude_height_attr );
        }
        else if ( extrude_range ) {
            gf->setMinHeight( extrude_min_height );
            gf->setMaxHeight( extrude_max_height );
            gf->setRandomizeHeights( true );
        }
        else {
            gf->setHeight( extrude_height );
        }
        gf->setHeightScale( extrude_height_scale );
        script->appendFilter( gf );
    }
    else
    {
        osgGIS::BuildGeomFilter* gf = new osgGIS::BuildGeomFilter();
        gf->setColor( color );
        gf->setRandomizeColors( color.a() == 0 );
        script->appendFilter( gf );
    }

    // Bring all the drawables into a single collection so that they
    // all fall under the same osg::Geode.
    script->appendFilter( new osgGIS::CollectionFilter() );

    // Construct a Node that contains the drawables and adjust its
    // state set.
    osgGIS::BuildNodesFilter* bnf = new osgGIS::BuildNodesFilter(
        osgGIS::BuildNodesFilter::CULL_BACKFACES |
        osgGIS::BuildNodesFilter::OPTIMIZE |
        (!lighting? osgGIS::BuildNodesFilter::DISABLE_LIGHTING : 0) );

    script->appendFilter( bnf );

    return script;
}



int
main(int argc, char* argv[])
{
    parseCommandLine( argc, argv );

    // The osgGIS registry is the starting point for loading new feature layers
    // and creating spatial reference systems.
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // Load up the feature layer that we want to clamp to a terrain:
    NOUT << "Loading feature layer and building spatial index..." << ENDL;

    osg::ref_ptr<osgGIS::FeatureLayer> layer = registry->createFeatureLayer( input_file );
    if ( !layer.valid() )
        return die( "Failed to create feature layer." );

    osgGIS::FeatureStore* store = layer->getFeatureStore();
    NOUT << "Connected to feature store: " << ENDL;
	NOUT << "  Name = "   << store->getName() << ENDL;
	NOUT << "  Count = "  << store->getFeatureCount() << ENDL;
	NOUT << "  Extent = " << store->getExtent().toString() << ENDL;

	osg::ref_ptr<osgGIS::SpatialReference> srs = layer->getSRS();
    if ( srs.valid() )
		NOUT << "  SRS = " << srs->getName() << ENDL;
	else
		NOUT << "  WARNING: No spatial reference" << ENDL;

    // Next we load the terrain file to which we will clamp the vector data:
    NOUT << "Loading terrain..." << ENDL;
    osg::ref_ptr<osg::Node> terrain = osgDB::readNodeFile( terrain_file );
    if ( !terrain.valid() )
        return die( "Terrain load failed!" );

    // determine the terrain's SRS:
    osg::ref_ptr<osgGIS::SpatialReference> terrain_srs =
        registry->getSRSFactory()->createSRSfromTerrain( terrain.get() );

    if ( !terrain_srs.valid() )
        return die( "Unable to determine the spatial reference of the terrain." );

    // Go earth-centered if necessary:
    if ( geocentric )
        terrain_srs = registry->getSRSFactory()->createGeocentricSRS( terrain_srs.get() );

    NOUT << "  terrain SRS = " << terrain_srs->getName() << std::endl;

    // Next we create a script that the compiler will use to build the geometry:
    NOUT << "Compiling..." << ENDL;
    osg::ref_ptr<osgGIS::Script> script = createScript( terrain_srs.get() );

    // Set a reference terrain in the compilation environment:
    osg::ref_ptr<osgGIS::FilterEnv> env = new osgGIS::FilterEnv();
    env->setTerrainNode( terrain.get() );

    // Create the output folder if necessary:
    if ( !osgDB::makeDirectoryForFile( output_file ) )
        return die( "Unable to create output directory!" );

    // Compile the feature layer into a paged scene graph. This utility class
    // will traverse an entire PagedLOD scene graph and generate a geometry 
    // tile for each terrain tile.
    osgGIS::PagedLayerCompiler compiler;
    osg::ref_ptr<osg::Node> output = compiler.compile(
        layer.get(),
        script.get(),
        terrain.get(),
        terrain_extent,
        min_level,
        max_level,
        priority_offset,
        output_file );

    if ( !output.valid() )
        return die( "Compilation failed!" );

	return 0;
}

