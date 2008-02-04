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
 *  - How to build a filter graph and use various filters
 *  - How to use the PagedLayerCompiler
 */

#include <osgGIS/Registry>
#include <osgGIS/FeatureStore>
#include <osgGIS/Compiler>
#include <osgGIS/FilterGraph>
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
#include <osgGIS/GriddedLayerCompiler>
#include <osgGIS/SimpleLayerCompiler>
#include <osgGIS/TaskManager>

#include <osg/ArgumentParser>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/Archive>
#include <osg/Notify>
#include <osg/Group>
#include <osg/PolygonOffset>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>


#define NOUT osg::notify(osg::NOTICE)
#define ENDL std::endl


// Variables that are set by command line arguments:
std::string input_file;
std::string output_file;
std::string terrain_file;
float range_far = 1e10;
float range_near = 0.0f;
bool paged = false;
bool correlated = false;
bool gridded = false;
int grid_rows = 1;
int grid_cols = 1;
osg::Vec4f color(1,1,1,1);
std::string color_expr;
bool include_grid = false;
bool preview = false;
osgGIS::GeoExtent terrain_extent( 
        -180, -90, 180, 90, 
        osgGIS::Registry::instance()->getSRSFactory()->createWGS84() );
osgGIS::GeoShape::ShapeType shape_type = osgGIS::GeoShape::TYPE_UNSPECIFIED;
bool lighting = true;
bool geocentric = false;
bool extrude = false;
std::string extrude_height_expr;
bool extrude_range = false;
double extrude_min_height = -1;
double extrude_max_height = -1;
double decimate_threshold = 0.0;
float priority_offset = 0.0;
bool convex_hull = false;
bool remove_holes = false;
bool fade_lods = false;
int num_threads = 0;
bool overlay = false;
osg::ref_ptr<osgGIS::SpatialReference> terrain_srs;

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
    NOUT << "    " << prog << " --input vector_file --output output_file [options...]" << ENDL;
    NOUT << ENDL;
    NOUT << "Required:" << ENDL;
    NOUT << "    --input <filename>        - Vector data to clamp to the terrain" << ENDL;
    NOUT << "    --output <filename>       - Where to store clamped geometry" << ENDL;
    NOUT << ENDL;
    NOUT << "Optional:"<< ENDL;
    NOUT << "    --terrain <filename>      - Terrain data file to which to clamp vectors" << ENDL;
    NOUT << "    --terrain-extent <long_min,lat_min,long_max,lat_max>" << ENDL;
    NOUT << "                               - Extent of terrain in long/lat degrees (default is whole earth)" << ENDL;
    NOUT << "    --terrain-srs              - File (usually .prj) containing projection of terain" << ENDL;
    NOUT << "    --geocentric               - Generate geocentric output geometry to match a PagedLOD globe" << ENDL;
    NOUT << "    --threads <num>            - Number of parallel compiler threads (default = # of logical procs)" << ENDL;
    NOUT << ENDL;
    NOUT << "  Layer options:" << ENDL;
    NOUT << "    --gridded                  - Generate simple gridded output (optionally combine with --paged)" << ENDL;
    NOUT << "    --grid-rows                - Number of rows to generate (implies --gridded)" << ENDL;
    NOUT << "    --grid-cols                - Number of columns to generate (implies --gridded)" << ENDL;
    NOUT << "    --paged                    - Generate PagedLOD output" << ENDL;
    NOUT << "    --overlay                  - Hints that the output will be used in an osgSim::OverlayNode (disables clamping)" << ENDL;
    NOUT << "    --correlated               - Generate PagedLODs that correlate one-to-one with terrain PagedLODs (forces --paged)" << ENDL;
    NOUT << ENDL;
    NOUT << "  Geometry options:" << ENDL;
    NOUT << "    --points                   - Treat the vector data as points" << ENDL;
    NOUT << "    --lines                    - Treat the vector data as lines" << ENDL;
    NOUT << "    --polygons                 - Treat the vector data as polygons" << ENDL;
    NOUT << "    --range-near <num>         - Near LOD range for output geometry (default = 0)" << ENDL;
    NOUT << "    --range-far <num>          - Far LOD range for output geometry (default = 1e+10)" << ENDL;
    NOUT << "    --no-lighting              - Disables lighting on the output geometry; good for points and lines" << ENDL;
    NOUT << "    --priority-offset <num>    - Paging priority of vectors relative to terrain tiles (when using --paged)" << ENDL;
    NOUT << "    --fade-lods                - Enable fade-in of LOD nodes (lines and points only)" << ENDL;
    //NOUT << "    --include-grid             - Includes geometry for the PagedLOD grid structure (when using --paged)" << ENDL;
    NOUT << ENDL;
    NOUT << "  Feature options:" << ENDL;
    NOUT << "    --extrude-height <expr>    - Extrude shapes to this height above the terrain (expression)" << ENDL;
    NOUT << "    --extrude-range <min,max>  - Randomly extrude shapes to heights in this range" << ENDL;
    NOUT << "    --remove-holes             - Removes holes in polygons" << ENDL;
    NOUT << "    --decimate <num>           - Decimate feature shapes to this threshold" << ENDL;
    //NOUT << "    --convex-hull              - Replace feature data with its convex hull" << ENDL;
    NOUT << "    --color <expr>             - Color, as a Lua expression (e.g. \"vec4(1,0,0,1)\")" << ENDL;
    NOUT << "    --random-colors            - Randomly assign feature colors" << ENDL;

}


void
parseCommandLine( int argc, char** argv )
{
    osg::Referenced::setThreadSafeReferenceCounting( true );
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
    
    while( arguments.read( "--paged" ) )
        paged = true;

    while( arguments.read( "--gridded" ) )
        gridded = true;

    while( arguments.read( "--grid-rows", str ) ) {
        sscanf( str.c_str(), "%d", &grid_rows );
        gridded = true;
    }

    while( arguments.read( "--grid-cols", str ) ) {
        sscanf( str.c_str(), "%d", &grid_cols );
        gridded = true;
    }

    while( arguments.read( "--correlated" ) )
        correlated = true;

    while( arguments.read( "--range-near", str ) )
        sscanf( str.c_str(), "%f", &range_near );
    
    while( arguments.read( "--range-far", str ) )
        sscanf( str.c_str(), "%f", &range_far );

    while( arguments.read( "--points" ) )
        shape_type = osgGIS::GeoShape::TYPE_POINT;

    while( arguments.read( "--lines" ) )
        shape_type = osgGIS::GeoShape::TYPE_LINE;

    while( arguments.read( "--polygons" ) )
        shape_type = osgGIS::GeoShape::TYPE_POLYGON;

    while( arguments.read( "--priority-offset", str ) )
        sscanf( str.c_str(), "%f", &priority_offset );

    while( arguments.read( "--convex-hull" ) )
        convex_hull = true;

    while( arguments.read( "--remove-holes" ) )
        remove_holes = true;

    while( arguments.read( "--fade-lods" ) )
        fade_lods = true;

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

    std::string prj_file;
    while( arguments.read( "--terrain-srs", prj_file ) )
    {
        std::stringstream prj;
        std::ifstream infile( prj_file.c_str() );
        std::istream_iterator<std::string> begin( infile ), end;
        while( begin != end ) prj << *begin++;
        terrain_srs = registry->getSRSFactory()->createSRSfromWKT( prj.str() );

        if ( terrain_srs.valid() )
        {
            osg::notify(osg::NOTICE) << "Read terrain SRS from file => " << 
                terrain_srs->getName() << std::endl;
        }
    }

    while( arguments.read( "--color", color_expr ) );

    while( arguments.read( "--random-colors" ) )
        color.a() = 0.0;

    while( arguments.read( "--extrude-height", str ) ) {
        extrude = true;
        extrude_height_expr = str;
    }

    while( arguments.read( "--extrude-range", str ) ) {
        extrude = true;
        extrude_range = true;
        sscanf( str.c_str(), "%lf,%lf", &extrude_min_height, &extrude_max_height );
    }

    while( arguments.read( "--decimate", str ) )
        sscanf( str.c_str(), "%lf", &decimate_threshold );

    while( arguments.read( "--include-grid" ) )
        include_grid = true;

    while( arguments.read( "--no-lighting" ) )
        lighting = false;

    while( arguments.read( "--threads", str ) )
        sscanf( str.c_str(), "%d", &num_threads );

    while( arguments.read( "--overlay" ) )
        overlay = true;


    // validate required arguments:
    if (input_file.length() == 0 || output_file.length() == 0 )
    {
        usage( arguments.getApplicationName().c_str(), 0 );
        exit(-1);
    }
}



osgGIS::FilterGraph*
createFilterGraph()
{
	osgGIS::Registry* registry = osgGIS::Registry::instance();

    // The "filter graph" is a series of filters that will transform the GIS
    // feature data into a scene graph.
    osgGIS::FilterGraph* graph = new osgGIS::FilterGraph();
    
    // Treat the incoming feature data as lines:
    if ( shape_type != osgGIS::GeoShape::TYPE_UNSPECIFIED )
    {
        graph->appendFilter( new osgGIS::ChangeShapeTypeFilter(
            shape_type ) );
    }

    // Remove holes in polygons as an optimization
    if ( remove_holes )
    {
        graph->appendFilter( new osgGIS::RemoveHolesFilter() );
    }

    // Replace features with convex hull
    if ( convex_hull )
    {
        graph->appendFilter( new osgGIS::CollectionFilter() );
        graph->appendFilter( new osgGIS::ConvexHullFilter() );
    }

    // Crop the feature data to the compiler environment's working extent:
    graph->appendFilter( new osgGIS::CropFilter( 
        include_grid? osgGIS::CropFilter::SHOW_CROP_LINES : 0 ) );

    // Transform the features to the target spatial reference system,
    // localizing them to a local origin:
    graph->appendFilter( new osgGIS::TransformFilter(
        osgGIS::TransformFilter::USE_TERRAIN_SRS |
        osgGIS::TransformFilter::LOCALIZE ) );

    // Decimate shapes to a point-to-point-distance threshold. Doing this
    // after the transform means we're dealing in meters.
    if ( decimate_threshold > 0.0 )
    {
        graph->appendFilter( new osgGIS::DecimateFilter( 
            decimate_threshold ) );
    }

    // Adjust the spatial data so that it conforms to the reference
    // terrain skin (unless we are overlay-ing)
    if ( !overlay )
    {
        graph->appendFilter( new osgGIS::ClampFilter() );
    }

    // Construct osg::Drawable's from the incoming feature batches:
    if ( extrude )
    {
        osgGIS::ExtrudeGeomFilter* gf = new osgGIS::ExtrudeGeomFilter();
        //gf->setRandomizeColors( color.a() == 0 );
        if ( extrude_height_expr.length() > 0 ) {
            gf->setHeightExpr( extrude_height_expr );
        }
        else if ( extrude_range ) {
            gf->setMinHeight( extrude_min_height );
            gf->setMaxHeight( extrude_max_height );
            gf->setRandomizeHeights( true );
        }
        gf->setColorExpr( color_expr );
        if ( color.a() == 0 )
            gf->setColorExpr( "vec4(math.random(),math.random(),math.random(),1)" );

        graph->appendFilter( gf );
    }
    else
    {
        osgGIS::BuildGeomFilter* gf = new osgGIS::BuildGeomFilter();
        //gf->setRandomizeColors( color.a() == 0 );
        gf->setColorExpr( color_expr );
        if ( color.a() == 0 )
            gf->setColorExpr( "vec4(math.random(),math.random(),math.random(),1)" );
        graph->appendFilter( gf );
    }

    // Bring all the drawables into a single collection so that they
    // all fall under the same osg::Geode.
    graph->appendFilter( new osgGIS::CollectionFilter() );

    // Construct a Node that contains the drawables and adjust its
    // state set.
    osgGIS::BuildNodesFilter* bnf = new osgGIS::BuildNodesFilter(
        osgGIS::BuildNodesFilter::CULL_BACKFACES |
        osgGIS::BuildNodesFilter::OPTIMIZE |
        (!lighting? osgGIS::BuildNodesFilter::DISABLE_LIGHTING : 0) );

    // cluster culling causes overlay geometry not to work properly.
    if ( !overlay )
    {
        bnf->setApplyClusterCulling( true );
    }

    graph->appendFilter( bnf );

    graph->setName( "Default" );
    return graph;
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
    osg::ref_ptr<osg::Node> terrain;

    if ( terrain_file.length() > 0 )
    {
        NOUT << "Loading terrain..." << ENDL;
        terrain = osgDB::readNodeFile( terrain_file );
        if ( !terrain.valid() )
            return die( "Terrain load failed!" );

        // determine the terrain's SRS (required):
        if ( !terrain_srs.valid() )
        {
            terrain_srs = registry->getSRSFactory()->createSRSfromTerrain( terrain.get() );
            if ( !terrain_srs.valid() )
                return die( "Unable to determine the spatial reference of the terrain." );

            NOUT << "  terrain SRS = " << terrain_srs->getName() << std::endl;
        }
    }

    // Go earth-centered if necessary:
    if ( geocentric )
    {
        // will use WGS84 as the basis if terrain_srs is not already set
        terrain_srs = registry->getSRSFactory()->createGeocentricSRS( 
            terrain_srs.get() );
    }


    // Next we create a filter graph that the compiler will use to build the geometry:
    NOUT << "Compiling..." << ENDL;
    osg::ref_ptr<osgGIS::FilterGraph> graph = createFilterGraph();

    // Create the output folder if necessary:
    if ( osgDB::getFilePath( output_file ).length() > 0 && !osgDB::makeDirectoryForFile( output_file ) )
        return die( "Unable to create output directory!" );

    // Open an archive if necessary
    osg::ref_ptr<osgDB::Archive> archive;
    std::string archive_file;

    if ( osgDB::getLowerCaseFileExtension( output_file ) == "osga" )
    {
        archive = osgDB::openArchive( output_file, osgDB::Archive::CREATE, 4096 );
        archive_file = output_file;
        output_file = "out.ive";

        // since there's no way to set the master file name...fake it out
        osg::ref_ptr<osg::Group> dummy = new osg::Group();
        archive->writeNode( *(dummy.get()), output_file );
    }

    // task manager for parallel build, if necessary:
    osg::ref_ptr<osgGIS::TaskManager> manager =
        num_threads > 1? new osgGIS::TaskManager( num_threads ) :
        num_threads < 1? new osgGIS::TaskManager() :
        NULL;

    // for a PagedLOD terrain, generating matching PagedLOD geometry.
    if ( correlated )
    {
        // Compile the feature layer into a paged scene graph. This utility class
        // will traverse an entire PagedLOD scene graph and generate a geometry 
        // tile for each terrain tile.
        osgGIS::PagedLayerCompiler compiler;

        compiler.addFilterGraph( range_near, range_far, graph.get() );
        compiler.setTerrain( terrain.get(), terrain_srs.get(), terrain_extent );
        compiler.setPriorityOffset( priority_offset );
        compiler.setFadeLODs( fade_lods );
        //compiler.setOverlay( overlay );
        compiler.setArchive( archive.get(), archive_file );
        compiler.setTaskManager( manager.get() );

        compiler.compile(
            layer.get(),
            output_file );
    }

    else if ( gridded )
    {
        osgGIS::GriddedLayerCompiler compiler;

        compiler.setNumRows( grid_rows );
        compiler.setNumColumns( grid_cols );
        compiler.setPaged( paged );
        compiler.setFadeLODs( fade_lods );
        //compiler.setOverlay( overlay );

        compiler.addFilterGraph( range_near, range_far, graph.get() );
        compiler.setTerrain( terrain.get(), terrain_srs.get(), terrain_extent );
        compiler.setArchive( archive.get(), archive_file );
        compiler.setTaskManager( manager.get() );

        osg::ref_ptr<osg::Node> node = new osg::Group();
        
        node = compiler.compile( layer.get(), output_file );

        if ( node.valid() )
        {
            if ( archive.valid() )
                archive->writeNode( *(node.get()), output_file );
            else
                osgDB::writeNodeFile( *(node.get()), output_file );
        }
    }

    // otherwise, just compile a simple LOD-based graph.
    else
    {
        osgGIS::SimpleLayerCompiler compiler;
        
        compiler.addFilterGraph( range_near, range_far, graph.get() );
        compiler.setTerrain( terrain.get(), terrain_srs.get(), terrain_extent );
        compiler.setArchive( archive.get(), archive_file );
        compiler.setFadeLODs( fade_lods );
        //compiler.setOverlay( overlay );
        compiler.setTaskManager( manager.get() );

        osg::ref_ptr<osg::Node> node = compiler.compile( layer.get() );
        if ( node.valid() )
        {
            if ( archive.valid() )
                archive->writeNode( *(node.get()), output_file );
            else
                osgDB::writeNodeFile( *(node.get()), output_file );
        }
    }

    if ( archive.valid() )
    {
        archive->close();
    }

    return 0;
}

