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

#include <osgGISProjects/QuadTreeMapLayerCompiler>
#include <osgGIS/FeatureLayerCompiler>
#include <osgGIS/ResourcePackager>
#include <osgGIS/Report>
#include <osgGIS/Session>
#include <osgGIS/Utils>
#include <osgGIS/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/NodeVisitor>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Geode>
#include <osg/Geometry>
#include <sstream>
#include <algorithm>

using namespace osgGIS;
using namespace osgGISProjects;
using namespace OpenThreads;

#define MY_PRIORITY_SCALE 1.0f //0.5f

//#define USE_PAGEDLODS_IN_INDEX 1


/*****************************************************************************/

QuadTreeMapLayerCompiler::QuadTreeProfile::QuadTreeProfile( const QuadMap& _qmap )
: Profile(), qmap(_qmap ) { }

const QuadMap&
QuadTreeMapLayerCompiler::QuadTreeProfile::getQuadMap() const { 
    return qmap;
}

/*****************************************************************************/

QuadTreeMapLayerCompiler::QuadTreeMapLayerCompiler( MapLayer* _layer, Session* _session )
: MapLayerCompiler( _layer, _session )
{
    //NOP
}

static unsigned int
getTopLod( const QuadMap& qmap, MapLayer* map_layer )
{
    int bottom_lod = qmap.getBestLodForCellsOfSize( map_layer->getCellWidth(), map_layer->getCellHeight() );
    int max_depth = map_layer->getMaxDepth();
    int top_lod = std::max( 0, bottom_lod - max_depth );
    return (unsigned int)top_lod;
}

static MapLayerLevelOfDetail*
getDefinition( const QuadKey& key, MapLayer* map_layer )
{
    unsigned top_lod = getTopLod( key.getMap(), map_layer );
    unsigned depth_to_find = key.getLOD() - top_lod;
    for( MapLayerLevelsOfDetail::const_iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
    {
        if ( i->get()->getDepth() == depth_to_find )
            return i->get();
    }
    return NULL;
}

Task*
QuadTreeMapLayerCompiler::createQuadKeyTask( const QuadKey& key )
{
    // construct a filter environment template to use for all tasks:
    osg::ref_ptr<FilterEnv> cell_env = getSession()->createFilterEnv();

    cell_env->setTerrainNode( getTerrainNode() );
    cell_env->setTerrainSRS( getTerrainSRS() );

    std::string abs_path = createAbsPathFromTemplate( "g" + key.toString() );

    Task* task = NULL;

    MapLayerLevelOfDetail* def = getDefinition( key.createParentKey(), map_layer.get() );
    if ( def )
    {
        cell_env->setInputSRS( def->getFeatureLayer()->getSRS() );
        cell_env->setExtent( map_layer->getAreaOfInterest().getSRS()->transform( key.getExtent() ) );
        cell_env->setProperty( Property( "compiler.cell_id", key.toString() ) );

        task = new CellCompiler(
            abs_path,
            def->getFeatureLayer(),
            def->getFilterGraph(),
            def->getMinRange(),
            def->getMaxRange(),
            cell_env.get(),
            def->getResourcePackager()? def->getResourcePackager() : resource_packager.get(),
            getArchive() );

        osgGIS::info()
            << "Task: Key = " << key.toString() << ", LOD = " << key.getLOD() << ", Extent = " << key.getExtent().toString() 
            << " (w=" << key.getExtent().getWidth() << ", h=" << key.getExtent().getHeight() << ")"
            << std::endl;
    }

    return task;
}

Profile*
QuadTreeMapLayerCompiler::createProfile()
{
    // determine the output SRS:
    osg::ref_ptr<SpatialReference> out_srs = map_layer->getOutputSRS( getSession(), terrain_srs.get() );
    if ( !out_srs.valid() )
    {
        osgGIS::warn() << "Unable to figure out the output SRS; aborting." << std::endl;
        return false;
    }

    // figure out the bounds of the compilation area and create a Q map. We want a sqaure AOI..maybe
    const GeoExtent& aoi = map_layer->getAreaOfInterest();
    if ( !aoi.isValid() )
    {
        osgGIS::warn() << "Invalid area of interest in the map layer - no data?" << std::endl;
        return false;
    }

    QuadMap qmap;
    if ( out_srs->isGeocentric() )
    {
        // for a geocentric map, use a modified GEO quadkey:
        // (yes, that MIN_LAT of -180 is correct...we want a square)
        qmap = QuadMap( GeoExtent( -180.0, -180.0, 180.0, 90.0, Registry::SRSFactory()->createWGS84() ) );
    }
    else
    {
        double max_span = std::max( aoi.getWidth(), aoi.getHeight() );
        GeoPoint sw = aoi.getSouthwest();
        GeoPoint ne( sw.x() + max_span, sw.y() + max_span, aoi.getSRS() );
        qmap = QuadMap( GeoExtent( sw, ne ) );
    }

#if 1
    qmap.setStringStyle( QuadMap::STYLE_LOD_QUADKEY );
#endif

    osgGIS::notice()
        << "QMAP: " << std::endl
        << "   Top LOD = " << getTopLod( qmap, map_layer.get() ) << std::endl
        << "   Depth   = " << map_layer->getMaxDepth() << std::endl
        << "   Extent = " << qmap.getBounds().toString() << ", w=" << qmap.getBounds().getWidth() << ", h=" << qmap.getBounds().getHeight() << std::endl
        << std::endl;

    return new QuadTreeProfile( qmap );
}

unsigned int
QuadTreeMapLayerCompiler::queueTasks( Profile* _profile, TaskManager* task_man )
{
    QuadTreeProfile* profile = dynamic_cast<QuadTreeProfile*>( _profile );
    if ( profile )
    {
        // Now, build the index and collect the list of keys for which to compile data.
        QuadKeyList keys;
        collectGeometryKeys( profile->getQuadMap(), keys );

        // make a build task for each quad cell we collected:
        //int total_tasks = keys.size();
        for( QuadKeyList::iterator i = keys.begin(); i != keys.end(); i++ )
        {
            task_man->queueTask( createQuadKeyTask( *i ) );
        }

        return keys.size();
    }
    else
    {
        return 0;
    }
}

// builds an index node (pointers to quadkey geometry nodes) that references 
// subtiles.
osg::Node*
QuadTreeMapLayerCompiler::createIntermediateIndexNode(const QuadKey& key,
                                                      float min_range, float max_range,
                                                      SmartReadCallback* reader )
{
    osg::Group* group = NULL;

    for( unsigned int quadrant = 0; quadrant < 4; quadrant++ )
    {
        QuadKey subkey = key.createSubKey( quadrant );

        if ( osgDB::fileExists( createAbsPathFromTemplate( "g"+subkey.toString() ) ) )
        {
            if ( !group )
            {
                group = new osg::Group();
                group->setName( key.toString() );
            }

            //osgGIS::notice() << "QK=" << subkey.toString() << ", Extent=" << subkey.getExtent().toString() << std::endl;

            // enter the subtile set as a paged index node reference:
            osg::PagedLOD* plod = new osg::PagedLOD();
            setCenterAndRadius( plod, subkey.getExtent(), reader );


#ifdef USE_PAGEDLODS_IN_INDEX
            osg::PagedLOD* pointer = new osg::PagedLOD();
            pointer->setFileName( 0, createRelPathFromTemplate( "g" + subkey.toString() ) );
            pointer->setRange( 0, 0, 1e10 );
            pointer->setPriorityScale( 0, 1000.0f ); // top priority, hopefully
            pointer->setPriorityOffset( 0, 1000.0f );
            pointer->setCenter( plod->getCenter() );
            pointer->setRadius( plod->getRadius() );
#else
            osg::ProxyNode* pointer = new osg::ProxyNode();
            pointer->setLoadingExternalReferenceMode( osg::ProxyNode::LOAD_IMMEDIATELY );
            pointer->setFileName( 0, createRelPathFromTemplate( "g" + subkey.toString() ) );
            //setCenterAndRadius( pointer, subkey.getExtent(), reader );
#endif
        
            plod->addChild( pointer, max_range, 1e10 );
            plod->setFileName( 1, createRelPathFromTemplate( "i" + subkey.toString() ) );
            plod->setRange( 1, 0, max_range ); // last one should always be min=0
            plod->setPriorityScale( 1, MY_PRIORITY_SCALE );

            group->addChild( plod );



            //osg::Geode* geode = new osg::Geode();

            //osg::Sphere* g1 = new osg::Sphere( osg::Vec3(0,0,0), plod->getRadius() );
            //osg::ShapeDrawable* sd1 = new osg::ShapeDrawable( g1 );
            //sd1->setColor( osg::Vec4f(1,0,0,.2) );
            //geode->addDrawable( sd1 );

            ////osg::Vec3d p = terrain_srs->transform( subkey.getExtent().getCentroid() );
            ////osg::Vec3d n = p; n.normalize();

            ////osg::Geometry* g3 = new osg::Geometry();
            ////osg::Vec3Array* v3 = new osg::Vec3Array(2);
            ////(*v3)[0] = osg::Vec3d(0,0,0);
            ////(*v3)[1] = n * 3000.0;
            ////g3->setVertexArray( v3 );
            ////osg::Vec4Array* c3 = new osg::Vec4Array(1);
            ////(*c3)[0].set(1,1,0,1);
            ////g3->setColorArray( c3 );
            ////g3->setColorBinding(osg::Geometry::BIND_OVERALL);
            ////g3->addPrimitiveSet( new osg::DrawArrays(GL_LINES, 0, 2) );
            ////g3->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
            ////geode->addDrawable( g3 );

            ////osg::Sphere* g2 = new osg::Sphere( osg::Vec3(0,0,0), 25 );
            ////osg::ShapeDrawable* sd2 = new osg::ShapeDrawable( g2 );
            ////sd2->setColor( osg::Vec4f(1,1,0,1) );
            ////geode->addDrawable( sd2 );

            ////osg::Matrixd mx = osg::Matrixd::translate( p );
            //osg::Matrixd mx = osg::Matrixd::translate( plod->getCenter() );
            //osg::MatrixTransform* mt = new osg::MatrixTransform( mx );

            //mt->addChild( geode );
            //mt->getOrCreateStateSet()->setMode( GL_BLEND, osg::StateAttribute::ON );
            //mt->getOrCreateStateSet()->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
            //group->addChild( mt );
        }
    }

    return group;
}

// creates an index node (pointer to quadkey geometry nodes) that has no
// children.
osg::Node*
QuadTreeMapLayerCompiler::createLeafIndexNode( const QuadKey& key, SmartReadCallback* reader )
{
    osg::Group* group = NULL;

    for( unsigned int i=0; i<4; i++ )
    {
        QuadKey quadrant_key = key.createSubKey( i );

        if ( osgDB::fileExists( createAbsPathFromTemplate( "g"+quadrant_key.toString() ) ) )
        {
            if ( !group )
            {
                group = new osg::Group();
                group->setName( key.toString() );
            }
            
#ifdef USE_PAGEDLODS_IN_INDEX
            osg::PagedLOD* pointer = new osg::PagedLOD();
            pointer->setFileName( 0, createRelPathFromTemplate( "g" + quadrant_key.toString() ) );
            pointer->setRange( 0, 0, 1e10 );
            pointer->setPriorityScale( 0, 1000.0f ); // top priority!
            pointer->setPriorityOffset( 0, 1000.0f );
            setCenterAndRadius( pointer, quadrant_key.getExtent(), reader );
#else
            osg::ProxyNode* pointer = new osg::ProxyNode();
            pointer->setLoadingExternalReferenceMode( osg::ProxyNode::LOAD_IMMEDIATELY );
            pointer->setFileName( 0, createRelPathFromTemplate( "g" + quadrant_key.toString() ) );
            //setCenterAndRadius( pointer, quadrant_key.getExtent(), reader );
#endif

            group->addChild( pointer );
        }
    }

    return group;
}

// assembles the list of quadkeys for which to build geometry cells. We can derive the set
// of index cells from this set of geometry cells if necessary.
void
QuadTreeMapLayerCompiler::collectGeometryKeys( const QuadMap& qmap, QuadKeyList& geom_keys )
{
    // the starting LOD is the best fit the the cell size:
    unsigned int top_lod = getTopLod( qmap, map_layer.get() );

    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
    {
        MapLayerLevelOfDetail* level_def = i->get();
        unsigned int lod = top_lod + level_def->getDepth();
        
        // get the extent of tiles that we will build based on the AOI:
        unsigned int cell_xmin, cell_ymin, cell_xmax, cell_ymax;
        qmap.getCells(
            map_layer->getAreaOfInterest(), lod,
            cell_xmin, cell_ymin, cell_xmax, cell_ymax );

        for( unsigned int y = cell_ymin; y <= cell_ymax; y++ )
        {
            for( unsigned int x = cell_xmin; x <= cell_xmax; x++ )
            {
                QuadKey key( x, y, lod, qmap ); 
                for( unsigned int k=0; k<4; k++ )
                {
                    geom_keys.push_back( key.createSubKey( k ) );
                }
            }
        }
    }
}

// builds and writes all the index nodes.
void
QuadTreeMapLayerCompiler::buildIndex( Profile* _profile, osg::Group* scene_graph )
{
    QuadTreeProfile* profile = dynamic_cast<QuadTreeProfile*>( _profile );
    if ( !profile ) return;

    osgGIS::notice() << "Rebuilding index..." << std::endl;

    // first, determine the SRS of the output scene graph so that we can
    // make pagedlod/lod centroids.
    SpatialReference* output_srs = map_layer->getOutputSRS( getSession(), getTerrainSRS() );

    // first build the very top level.
    //scene_graph = new osg::Group();

    // the starting LOD is the best fit the the cell size:
    unsigned int top_lod = getTopLod( profile->getQuadMap(), map_layer.get() );

    osg::ref_ptr<SmartReadCallback> reader = new SmartReadCallback();

    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
    {
        MapLayerLevelOfDetail* level_def = i->get();
        unsigned int lod = top_lod + level_def->getDepth();

        MapLayerLevelOfDetail* sub_level_def = i+1 != map_layer->getLevels().end()? (i+1)->get() : NULL;
        float min_range, max_range;
        if ( sub_level_def )
        {
            min_range = sub_level_def->getMinRange();
            max_range = sub_level_def->getMaxRange();
        }
        
        // get the extent of tiles that we will build based on the AOI:
        unsigned int cell_xmin, cell_ymin, cell_xmax, cell_ymax;
        profile->getQuadMap().getCells(
            map_layer->getAreaOfInterest(), lod,
            cell_xmin, cell_ymin, cell_xmax, cell_ymax );

        for( unsigned int y = cell_ymin; y <= cell_ymax; y++ )
        {
            for( unsigned int x = cell_xmin; x <= cell_xmax; x++ )
            {
                osg::ref_ptr<osg::Node> node;

                QuadKey key( x, y, lod, profile->getQuadMap() );  

                //osgGIS::notify( osg::NOTICE )
                //    << "Cell: " << std::endl
                //    << "   Quadkey = " << key.toString() << std::endl
                //    << "   LOD = " << key.getLOD() << std::endl
                //    << "   Extent = " << key.getExtent().toString() << " (w=" << key.getExtent().getWidth() << ", h=" << key.getExtent().getHeight() << ")" << std::endl
                //    << std::endl;

                node = sub_level_def ?
                    createIntermediateIndexNode( key, min_range, max_range, reader.get() ) :
                    createLeafIndexNode( key, reader.get() );
                
                if ( node.valid() )
                {
                    std::string out_file = createAbsPathFromTemplate( "i" + key.toString() );

                    if ( !osgDB::writeNodeFile( *(node.get()), out_file ) )
                    {
                        osgGIS::warn() << "FAILED to write index file " << out_file << std::endl;
                    }
                    
                    // at the top level, assemble the root node
                    if ( i == map_layer->getLevels().begin() )
                    {
                        double top_min_range = sub_level_def? 0 : level_def->getMinRange();

                        osg::PagedLOD* plod = new osg::PagedLOD();
                        plod->setName( key.toString() );
                        plod->setFileName( 0, createRelPathFromTemplate( "i" + key.toString() ) );
                        plod->setRange( 0, top_min_range, level_def->getMaxRange() );
                        plod->setPriorityScale( 0, MY_PRIORITY_SCALE );
                        setCenterAndRadius( plod, key.getExtent(), reader.get() );

                        //osgGIS::notice() << "QK=" << key.toString() << ", Ex=" << key.getExtent().toString() << ", Cen=" << key.getExtent().getCentroid().toString() << std::endl;


                        scene_graph->addChild( plod );
                    }
                }
            }
        }
    }
}
