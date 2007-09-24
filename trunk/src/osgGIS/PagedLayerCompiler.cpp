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
#include <osgGIS/Compiler>
#include <osgGIS/Registry>
#include <osg/CoordinateSystemNode>
#include <osg/Notify>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <OpenThreads/Thread>

using namespace osgGIS;


PagedLayerCompiler::PagedLayerCompiler()
{
    //NOP
}

PagedLayerCompiler::~PagedLayerCompiler()
{
    //NOP
}


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
class SetPathVisitor : public osg::NodeVisitor
{
public:
    SetPathVisitor( const std::string& _db_path ) : 
      osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ),
      db_path( _db_path ) { }

    void apply( osg::PagedLOD& node ) {
        node.setDatabasePath( db_path );
        traverse( node );
    }
    std::string db_path;
};


// Calculates a sub-extent of a larger extent, given the number of children and
// the child number. This currently assumes the subdivision ordering used by 
// VirtualPlanetBuilder.
GeoExtent
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
void
calculateSubExtents( const GeoExtent& extent, unsigned int num_children, std::vector<GeoExtent>& out )
{
    for( unsigned int i=0; i<num_children; i++ )
        out.push_back( getSubExtent( extent, num_children, i ) );
}


// Runs an osgGIS script to compile vector data against a terrain tile.
osg::Node*
compileGeometry(const GeoExtent& extent,
                osg::Node*       terrain,
                FeatureLayer*    layer,
                Script*          script )
{
    osg::notify( osg::NOTICE ) 
        << " ...compiling geom: " << extent.toString() << std::endl;

    osg::ref_ptr<FilterEnv> env = new FilterEnv();
    env->setTerrainNode( terrain );
    env->setExtent( extent );
    Compiler compiler( layer, script );
    osg::Node* out = compiler.compile( env.get() );
    return out;
}


// Traverses a terrain tile and compiles vector data against that tile; then
// recurses down into a PagedLOD graph and repeats.
osg::Group*
compileTileFile(int                level,
                osg::Node*         source_tile,
                bool               is_geocentric,
                const std::string& output_prefix,
                const std::string& output_extension,
                const GeoExtent&   extent,
                int                min_level,
                int                max_level,
                float              priority_offset,
                FeatureLayer*      layer,
                Script*            script,
                const std::string& output_dir )
{
    osg::notify( osg::NOTICE ) 
        << "L" << level << ": Compiling tile: " << extent.toString() << std::endl;

    osg::Group* top = new osg::Group();
    
    if ( level > max_level+1 && max_level >= 0 )
        return top;

    osg::Group* source_group = source_tile->asGroup();

    if ( source_group )
    {
        // precalculate the subdivisions that lie under this tile:
        std::vector<GeoExtent> sub_extents;
        unsigned int num_children = source_group->getNumChildren();
        calculateSubExtents( extent, num_children, sub_extents );
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

                if ( level >= min_level )
                {
                    osg::Node* terrain = plod->getChild( 0 );
                    geometry = compileGeometry(
                        sub_extent,
                        terrain,
                        layer,
                        script );
                    dv = osg::Object::STATIC;
                }
                else
                {
                    geometry = new osg::Group();
                    dv = osg::Object::DYNAMIC;
                }
                new_plod->setRangeList( plod->getRangeList() );
                new_plod->setChild( 0, geometry );

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
                new_plod->setDataVariance( dv ); //osg::Object::DYNAMIC );            

                top->addChild( new_plod );
                
                // Read the next subtile:
                osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options();
                options->setDatabasePath( plod->getDatabasePath() );
                osg::ref_ptr<osg::Node> subtile = osgDB::readNodeFile( 
                    plod->getFileName( 1 ), 
                    options.get() );

                // Build the geometry associated with this subtile:
                osg::ref_ptr<osg::Group> new_subtile = compileTileFile( 
                    level+1,
                    subtile.get(),
                    is_geocentric,
                    output_prefix,
                    output_extension,
                    sub_extent,
                    min_level,
                    max_level,
                    priority_offset,
                    layer,
                    script,
                    output_dir );     

                // Write the new subtile:
                std::string new_abs_path = osgDB::concatPaths( output_dir, new_filename );
                osg::notify( osg::NOTICE ) << "Writing " << new_filename << std::endl;
                osgDB::writeNodeFile( *(new_subtile.get()), new_abs_path );
            }
            else if ( level >= min_level )
            {
                // Compile against the terrain geometry in this tile:
                osg::Node* terrain = source_group->getChild( i );
                osg::Node* geometry = compileGeometry( 
                    sub_extent,
                    terrain,
                    layer,
                    script );

                top->addChild( geometry );
            }
        }
    }
    else if ( level >= min_level )
    {
        osg::Node* geometry = compileGeometry( 
            extent,
            source_tile,
            layer,
            script );

        top->addChild( geometry );
    }

    return top;
}


osg::Node*
compileAll(osg::Node*                 terrain_root,
           bool                       is_geocentric,
           const std::string&         output_prefix,
           const std::string&         output_extension,
           const GeoExtent&           extent,
           int                        min_level,
           int                        max_level,
           float                      priority_offset,
           FeatureLayer*              layer,
           Script*                    script,
           const std::string&         output_dir )
{
    //osg::CoordinateSystemNode* root = static_cast<osg::CoordinateSystemNode*>(
    //    cs_node->clone( osg::CopyOp::SHALLOW_COPY ) );

    osg::Node* out = compileTileFile(
        0,
        terrain_root,
        is_geocentric,
        output_prefix,
        output_extension,
        extent,
        min_level,
        max_level,
        priority_offset,
        layer,
        script,
        output_dir );

    std::string top_filename = output_dir + "/" + output_prefix + "." + output_extension;
    osgDB::writeNodeFile( *out, top_filename );

    // after writing, set the path at the top-level plod(s):
    SetPathVisitor spv( output_dir );
    out->accept( spv );

    return out;
}


osg::Node*
PagedLayerCompiler::compile(FeatureLayer*      layer,
                            Script*            script,
                            osg::Node*         ref_terrain,
                            const GeoExtent&   ref_terrain_extent,
                            int                min_level,
                            int                max_level,
                            float              priority_offset,
                            const std::string& output_file )
{
    bool is_geocentric = false;

    // first find the CS Node...
    FindCSNodeVisitor cs_finder;
    ref_terrain->accept( cs_finder );
    if ( !cs_finder.cs_node.valid() ) {
        osg::notify( osg::WARN ) << 
            "Reference terrain does not contain a CoordinateSystemNode." << std::endl;
    }
    else {
        is_geocentric = true;
    }

    // The folder into which to put the output:
    std::string output_dir = osgDB::getFilePath( output_file );

    // The output name prefix and extension:
    std::string output_prefix = osgDB::getStrippedName( output_file );
    std::string output_extension = osgDB::getFileExtension( output_file );

    osg::notify(osg::ALWAYS) << "PREFIX = " << output_prefix << std::endl;
    osg::notify(osg::ALWAYS) << "EXTENSION = " << output_extension << std::endl;
    osg::notify(osg::ALWAYS) << "DIR = " << output_dir << std::endl;

    // If the user passed in an empty/invalid/default extent, assume the terrain
    // is whole-earth:
    GeoExtent full_extent =
        ref_terrain_extent.getArea() > 0? ref_terrain_extent :
        GeoExtent( -180, -90, 180, 90, layer->getSRS()->getBasisSRS() );

    osg::Node* result = compileAll(
        ref_terrain,
        is_geocentric,
        output_prefix,
        output_extension,
        full_extent,
        min_level,
        max_level,
        priority_offset,
        layer,
        script,
        output_dir );

    return result;
}