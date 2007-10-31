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

#include <osgGIS/PagedLayerCompiler>
#include <osgGIS/Registry>
#include <osgGIS/Compiler>
#include <osgGIS/Registry>
#include <osg/CoordinateSystemNode>
#include <osg/Notify>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgUtil/Optimizer>
#include <OpenThreads/Thread>

using namespace osgGIS;

static std::vector<std::string> indent;

// Visitor to locate the first CoordinateSystemNode in a graph.
class FindCSNodeVisitor : public osg::NodeVisitor
{
public:
    FindCSNodeVisitor() : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) { }

    void apply( osg::CoordinateSystemNode& node ) {
        cs_node = &node;
        // by not calling "traverse(node)" the visitor will stop
    }

    osg::ref_ptr<osg::CoordinateSystemNode> cs_node;
};


// Visitor to set the database path on all PagedLOD nodes in a (resident) graph.
class SetDatabasePathVisitor : public osg::NodeVisitor
{
public:
    SetDatabasePathVisitor( const std::string& _db_path ) : 
      osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ),
      db_path( _db_path ) { }

    void apply( osg::PagedLOD& node ) {
        node.setDatabasePath( db_path );
        traverse( node );
    }
    std::string db_path;
};


//===========================================================================


PagedLayerCompiler::PagedLayerCompiler()
{
    priority_offset = 1.0f;

    std::string s = "";
    for( int i=0; i<100; i++ ) {
        indent.push_back( s );
        s = s + " ";
    }        
}


void
PagedLayerCompiler::addScript( double min_range, double max_range, Script* script )
{
    ScriptRange slice;
    slice.min_range = min_range;
    slice.max_range = max_range;
    slice.script = script;

    script_ranges.push_back( slice );
}


void
PagedLayerCompiler::setTerrain(osg::Node*              _terrain,
                               const SpatialReference* _terrain_srs,
                               const GeoExtent&        _terrain_extent )
{
    terrain        = _terrain;
    terrain_srs    = (SpatialReference*)_terrain_srs;
    terrain_extent = _terrain_extent;
}


void
PagedLayerCompiler::setPriorityOffset( float value )
{
    priority_offset = value;
}


void
PagedLayerCompiler::setTerrain(osg::Node*              _terrain,
                               const SpatialReference* _terrain_srs )
{
    setTerrain( _terrain, _terrain_srs, GeoExtent::infinite() );
}


std::string
PagedLayerCompiler::compile(FeatureLayer*      layer,
                            const std::string& output_file )
{
    is_geocentric = false;

    // first find the CS Node...
    FindCSNodeVisitor cs_finder;
    terrain->accept( cs_finder );
    if ( !cs_finder.cs_node.valid() ) {
        osg::notify( osg::WARN ) << 
            "Reference terrain does not contain a CoordinateSystemNode." << std::endl;
    }
    else {
        is_geocentric = true;
    }

    // The folder into which to put the output:
    output_dir = osgDB::getFilePath( output_file );

    // The output name prefix and extension:
    output_prefix = osgDB::getStrippedName( output_file );
    output_extension = osgDB::getFileExtension( output_file );

    // if the user passed a non-real extent, assume whole-earth:
    GeoExtent top_extent = terrain_extent.getArea() > 0?
        terrain_extent : 
        GeoExtent( -180, -90, 180, 90, Registry::SRSFactory()->createWGS84() );

    // compile away.
    return compileAll( layer, top_extent );
}


//============================================================================


// Calculates a sub-extent of a larger extent, given the number of children and
// the child number. This currently assumes the subdivision ordering used by 
// VirtualPlanetBuilder.
static GeoExtent
getSubExtent(const GeoExtent& extent,
             int              num_children,
             int              child_no)
{
    GeoPoint centroid = extent.getCentroid();
    GeoExtent sub_extent;

    switch( num_children )
    {
    case 0:
    case 1:
        sub_extent = extent;
        break;

    case 2:
        if ( child_no == 0 )
        {
            sub_extent = GeoExtent(
                extent.getXMin(),
                extent.getYMin(),
                centroid.x(),
                extent.getYMax(),
                extent.getSRS() );
        }
        else
        {
            sub_extent = GeoExtent(
                centroid.x(),
                extent.getYMin(),
                extent.getXMax(),
                extent.getYMax(),
                extent.getSRS() );
        }
        break;

    case 4:
        if ( child_no == 2 )
        {
            sub_extent = GeoExtent(
                extent.getXMin(),
                centroid.y(),
                centroid.x(),
                extent.getYMax(),
                extent.getSRS() );
        }
        else if ( child_no == 3 )
        {
            sub_extent = GeoExtent(
                centroid.x(),
                centroid.y(),
                extent.getXMax(),
                extent.getYMax(),
                extent.getSRS() );
        }
        else if ( child_no == 0 )
        {
            sub_extent = GeoExtent(
                extent.getXMin(),
                extent.getYMin(),
                centroid.x(),
                centroid.y(),
                extent.getSRS() );
        }
        else if ( child_no == 1 )
        {
            sub_extent = GeoExtent(
                centroid.x(),
                extent.getYMin(),
                extent.getXMax(),
                centroid.y(),
                extent.getSRS() );
        }
    }

    return sub_extent;
}


// Calcuates all the extents of a parent's subtiles after subdivision.
static void
calculateSubExtents( const GeoExtent& extent, unsigned int num_children, std::vector<GeoExtent>& out )
{
    for( unsigned int i=0; i<num_children; i++ )
        out.push_back( getSubExtent( extent, num_children, i ) );
}


osg::Node*
PagedLayerCompiler::compileGeometry(
    FeatureLayer*    layer,
    int              level,
    osg::Node*       tile_terrain,
    const GeoExtent& tile_extent,
    double           geom_min_range,
    double           geom_max_range )
{
    // figure out which script to use:
    //TODO: if the geom range crosses >1 script range, make an LOD with 
    //      multiple geometries (possible paged).

    Script* script = NULL;
    double closest_to_max = DBL_MAX;
    for( ScriptRangeList::iterator s = script_ranges.begin(); s != script_ranges.end(); s++ )
    {
        double diff = fabs( geom_max_range - s->max_range );
        if ( script == NULL || 
             ( geom_max_range <= s->max_range && diff < closest_to_max ) )
        {
            closest_to_max = diff;
            script = s->script.get();
        }
    }

    // compile that script:
    osg::Node* out = NULL;
    if ( script )
    {
        osg::notify(osg::NOTICE) << indent[level+2]
            << "script = " << script->getName() << std::endl;

        osg::ref_ptr<FilterEnv> env = new FilterEnv();
        env->setTerrainNode( tile_terrain );
        env->setTerrainSRS( terrain_srs.get() );
        env->setExtent( tile_extent );
        Compiler compiler( layer, script );
        out = compiler.compile( env.get() );
    }
    else
    {
        out = new osg::Group();
        out->setName( "osgGIS: NO SCRIPT" );
        out->setDataVariance( osg::Object::DYNAMIC ); // no optimization
    }
    return out;
}


// Top-level function to compile layer into a paged lod model
std::string
PagedLayerCompiler::compileAll(
       FeatureLayer*      layer,
       const GeoExtent&   extent )
{
    // figure out the min and max ranges:
    layer_min_range = DBL_MAX;
    layer_max_range = DBL_MIN;
    
    for( ScriptRangeList::iterator i = script_ranges.begin(); i != script_ranges.end(); i++ )
    {
        if ( i->min_range < layer_min_range )
            layer_min_range = i->min_range;
        if ( i->max_range > layer_max_range )
            layer_max_range = i->max_range;
    }

    std::string top_filename = output_prefix + "." + output_extension;

    compileTile(
        layer,
        0,
        terrain.get(),
        extent,
        1e+10,
        top_filename );

    return output_dir + "/" + top_filename;
}


void
PagedLayerCompiler::compileTile(
    FeatureLayer*           layer,
    int                     level,
    osg::Node*              tile_terrain,
    const GeoExtent&        tile_extent,
    double                  tile_max_range,
    const std::string&      tile_filename )
{
    osg::notify(osg::NOTICE) << indent[level]
        << "L" << level << ": " << tile_filename
        << " (max = " << tile_max_range << ")"
        << std::endl;
        
    osg::ref_ptr<osg::Group> top = new osg::Group();
    std::vector<osg::ref_ptr<osg::PagedLOD> > subtile_plods;

    osg::Group* tile_terrain_group = tile_terrain->asGroup();
    if ( tile_terrain_group )
    {
        // precalculate the subdivisions that lie under this tile:
        std::vector<GeoExtent> sub_extents;
        unsigned int num_children = tile_terrain_group->getNumChildren();
        calculateSubExtents( tile_extent, num_children, sub_extents );

        for( unsigned int i=0; i<num_children; i++ )
        {
            const GeoExtent& sub_extent = sub_extents[i];

            if ( dynamic_cast<osg::PagedLOD*>( tile_terrain_group->getChild(i) ) )
            {
                osg::PagedLOD* plod = static_cast<osg::PagedLOD*>( 
                    tile_terrain_group->getChild( i ) );

                // save for later:
                subtile_plods.push_back( plod );

                // the range at which we need to page in the subtile:
                double tile_min_range = plod->getRangeList()[0].first;

                // first, compile against this PLOD's geometry (which is in child 0).
                osg::Node* geometry = compileGeometry(
                    layer,
                    level,
                    plod->getChild( 0 ),
                    sub_extent,
                    tile_min_range,
                    tile_max_range );
                
                // check whether we need to traverse down any further:
                bool at_min_range = layer_min_range >= tile_min_range;
                if ( at_min_range )
                {
                    //osg::notify(osg::NOTICE) << indent[level]
                    //    << "at min range (layer min = " << layer_min_range << ", tile min = "
                    //    << tile_min_range << std::endl;

                    osg::LOD* lod = new osg::LOD();
                    lod->addChild( geometry, layer_min_range, 1e+10 );
                    top->addChild( lod );
                }
                else // still need to delve deeper; make another PLOD:
                {
                    osg::PagedLOD* new_plod = static_cast<osg::PagedLOD*>(
                        plod->clone( osg::CopyOp::SHALLOW_COPY ) );

                    new_plod->setRangeList( plod->getRangeList() );
                    new_plod->setChild( 0, geometry );  
                    new_plod->setCullCallback( plod->getCullCallback() );
                    new_plod->setDatabasePath( "" );
                    new_plod->setPriorityOffset( 1, priority_offset );

                    std::string subtile_filename = 
                        output_prefix + "_" + 
                        osgDB::getStrippedName( plod->getFileName( 1 ) ) + "." +
                        output_extension;

                    new_plod->setFileName( 1, subtile_filename );

                    top->addChild( new_plod );
                }
            }
            else // anything besides a PLOD, include as straight geometry.
            {
                // Compile against the terrain geometry in this tile:
                osg::Node* geometry = compileGeometry( 
                    layer,
                    level,
                    tile_terrain_group->getChild( i ),
                    sub_extent,
                    layer_min_range, //tile_min_range,
                    tile_max_range );

                if ( layer_min_range > 0 )
                {
                    // turns the geomtry off at its min lod
                    osg::LOD* lod = new osg::LOD();
                    lod->addChild( geometry, layer_min_range, 1e+10 );
                    top->addChild( lod );
                }
                else
                {
                    top->addChild( geometry );
                }
            }
        }

        // Write the new file before create subtiles.
        std::string new_abs_path = osgDB::concatPaths( output_dir, tile_filename );
        top->setName( tile_filename );
        osgDB::writeNodeFile( *(top.get()), new_abs_path );

        // Finally, walk through any new PLODs we created and compile the subtiles.
        for( int i=0, j=0; i<top->getNumChildren(); i++ )
        {
            const GeoExtent& sub_extent = sub_extents[i];

            if ( dynamic_cast<osg::PagedLOD*>( top->getChild(i) ) )
            {
                osg::PagedLOD* new_plod = static_cast<osg::PagedLOD*>( top->getChild(i) );

                // grab the corresponding terrain subtile PLOD that we saved eariler:
                osg::PagedLOD* subtile_plod = subtile_plods[j++].get();
                
                // Read the next terrain subtile:
                osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options();
                options->setDatabasePath( subtile_plod->getDatabasePath() );
                osg::ref_ptr<osg::Node> subtile = osgDB::readNodeFile( 
                    subtile_plod->getFileName( 1 ), 
                    options.get() );

                // and compile a geometry tile for it.
                compileTile(
                    layer,
                    level + 1,
                    subtile.get(),
                    sub_extent,
                    subtile_plod->getRangeList()[1].second,
                    new_plod->getFileName( 1 ) );
            }
        }
    }
}
