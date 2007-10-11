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

#include <osgGIS/PagedLayerCompiler2>
#include <osgGIS/Registry>
#include <osgGIS/Compiler>
#include <osgGIS/Registry>
#include <osg/CoordinateSystemNode>
#include <osg/Notify>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <OpenThreads/Thread>

using namespace osgGIS;



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


PagedLayerCompiler2::PagedLayerCompiler2()
{
    priority_offset = 0.0f;
}


void
PagedLayerCompiler2::addScript( double min_range, double max_range, Script* script )
{
    ScriptRange slice;
    slice.min_range = min_range;
    slice.max_range = max_range;
    slice.script = script;

    script_ranges.push_back( slice );
}


void
PagedLayerCompiler2::setTerrain(osg::Node*              _terrain,
                                const SpatialReference* _terrain_srs,
                                const GeoExtent&        _terrain_extent )
{
    terrain        = _terrain;
    terrain_srs    = (SpatialReference*)_terrain_srs;
    terrain_extent = _terrain_extent;
}


void
PagedLayerCompiler2::setTerrain(osg::Node*              _terrain,
                                const SpatialReference* _terrain_srs )
{
    setTerrain( _terrain, _terrain_srs, GeoExtent::infinite() );
}


osg::Node*
PagedLayerCompiler2::compile(FeatureLayer*      layer,
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
    osg::Node* result = compileAll(
        layer,
        top_extent );

    return result;
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
PagedLayerCompiler2::compileGeometry(
    FeatureLayer*    layer,
    int              level,
    osg::Node*       tile_terrain,
    const GeoExtent& tile_extent,
    double           min_range,
    double           max_range )
{
    osg::notify( osg::NOTICE ) 
        << " Compiling: L" << level << " " << tile_extent.toString() << std::endl;

    // figure out which script to use:
    Script* script = NULL;
    for( ScriptRangeList::iterator i = script_ranges.begin(); i != script_ranges.end(); i++ )
    {
        if ( script == NULL || ( i->min_range <= min_range && min_range <= i->max_range ) )
        {
            script = i->script.get();
        }
    }

    // compile that script:
    osg::Node* out = NULL;
    if ( script )
    {
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
    }
    return out;
}


// Top-level function to compile layer into a paged lod model
osg::Node*
PagedLayerCompiler2::compileAll(
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

    osg::Node* out = compileTile(
        layer,
        0,
        terrain.get(),
        extent,
        0,
        DBL_MAX );

    std::string top_filename = output_dir + "/" + output_prefix + "." + output_extension;
    osgDB::writeNodeFile( *out, top_filename );

    // after writing, set the path at the top-level plod(s):
    SetDatabasePathVisitor spv( output_dir );
    out->accept( spv );

    return out;
}


// Traverses a terrain tile and compiles vector data against that tile; then
// recurses down into a PagedLOD graph and repeats.
osg::Group*
PagedLayerCompiler2::compileTile(
    FeatureLayer*           layer,
    int                     level,
    osg::Node*              tile_terrain,
    const GeoExtent&        tile_extent,
    double                  tile_min_range,
    double                  tile_max_range )
{
    //osg::notify( osg::NOTICE ) 
    //    << "L" << level << ": Compiling tile: " << tile_extent.toString() << std::endl;

    osg::Group* top = new osg::Group();
    
    if ( layer_min_range > tile_max_range )
        return top;
    //if ( level > max_level+1 && max_level >= 0 )
    //    return top;

    osg::Group* source_group = tile_terrain->asGroup();

    if ( source_group )
    {
        // precalculate the subdivisions that lie under this tile:
        std::vector<GeoExtent> sub_extents;
        unsigned int num_children = source_group->getNumChildren();
        calculateSubExtents( tile_extent, num_children, sub_extents );
        std::vector<GeoExtent>::iterator exi = sub_extents.begin();

        for( unsigned int i=0; i<num_children; i++ )
        {
            GeoExtent sub_extent = *exi++;

            if ( dynamic_cast<osg::PagedLOD*>( source_group->getChild(i) ) )
            {
                osg::PagedLOD* plod = static_cast<osg::PagedLOD*>( source_group->getChild( i ) );
                osg::PagedLOD* new_plod = static_cast<osg::PagedLOD*>( plod->clone( osg::CopyOp::SHALLOW_COPY ) );

                // compile the geometry under child 0: //TODO: check for more
                osg::Node* geometry;
                osg::Object::DataVariance dv;

                if ( layer_max_range >= tile_max_range )
                {
                    osg::Node* sub_terrain = plod->getChild( 0 );
                    geometry = compileGeometry(
                        layer,
                        level,
                        sub_terrain,
                        sub_extent,
                        tile_min_range,
                        tile_max_range );
                    dv = osg::Object::STATIC;
                }
                else // make an empty placeholder
                {
                    geometry = new osg::Group();
                    dv = osg::Object::DYNAMIC;
                }

                new_plod->setChild( 0, geometry );
                new_plod->setRangeList( plod->getRangeList() );

                osg::notify( osg::NOTICE ) <<
                    "  GR = " << 
                    plod->getRangeList()[0].first << "=>" <<
                    plod->getRangeList()[0].second <<
                    ", TR= " <<
                    tile_min_range << "=>" <<
                    tile_max_range <<
                    ", LR= " <<
                    layer_min_range << "=>" <<
                    layer_max_range <<
                    std::endl;

                double inf = plod->getRangeList()[0].first;

                new_plod->setRange(
                    0,
                    std::max( layer_min_range, inf ),
                    plod->getRangeList()[0].second );

                // finish building the new subtile
                std::string new_filename = 
                    output_prefix + "_" + 
                    osgDB::getStrippedName( plod->getFileName( 1 ) ) + "." +
                    output_extension;

                new_plod->setFileName( 1, new_filename );
                new_plod->setCullCallback( plod->getCullCallback() );

                new_plod->setDatabasePath( "" );

                // bump the priority so that vectors page in before terrain tiles, to
                // avoid a flashing effect
                new_plod->setPriorityOffset( 1, priority_offset );

                // this will prevent the optimizer from blowing away the PLOD when we have a
                // stub geometry
                new_plod->setDataVariance( dv );           

                top->addChild( new_plod );
                
                // Read the next subtile:
                osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options();
                options->setDatabasePath( plod->getDatabasePath() );
                osg::ref_ptr<osg::Node> subtile = osgDB::readNodeFile( 
                    plod->getFileName( 1 ), 
                    options.get() );

                // Build the geometry associated with this subtile:
                osg::ref_ptr<osg::Group> new_subtile = compileTile( 
                    layer,
                    level+1,
                    subtile.get(),
                    sub_extent,
                    plod->getRangeList()[1].first,
                    plod->getRangeList()[1].second );

                // Write the new subtile:
                std::string new_abs_path = osgDB::concatPaths( output_dir, new_filename );
                osg::notify( osg::NOTICE ) << "Writing " << new_filename << std::endl;
                osgDB::writeNodeFile( *(new_subtile.get()), new_abs_path );
            }
            else if ( layer_max_range >= tile_max_range ) // ( level >= min_level )
            {
                // Compile against the terrain geometry in this tile:
                osg::Node* sub_terrain = source_group->getChild( i );
                osg::Node* geometry = compileGeometry( 
                    layer,
                    level,
                    sub_terrain,
                    sub_extent,
                    tile_min_range,
                    tile_max_range );

                top->addChild( geometry );
            }
        }
    }
    else if ( layer_max_range >= tile_max_range ) // level >= min_level )
    {
        osg::Node* geometry = compileGeometry( 
            layer,
            level,
            tile_terrain,
            tile_extent,
            tile_min_range,
            tile_max_range );

        top->addChild( geometry );
    }

    return top;
}
