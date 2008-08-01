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
#include <sstream>

using namespace osgGIS;
using namespace osgGISProjects;
using namespace OpenThreads;

#define MY_PRIORITY_SCALE 1.0f //0.5f


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


Task*
QuadTreeMapLayerCompiler::createQuadKeyTask( const QuadKey& key )
{
    // construct a filter environment template to use for all tasks:
    osg::ref_ptr<FilterEnv> cell_env = getSession()->createFilterEnv();

    cell_env->setTerrainNode( getTerrainNode() );
    cell_env->setTerrainSRS( getTerrainSRS() );

    std::string abs_path = createAbsPathFromTemplate( "g" + key.toString() );

    Task* task = NULL;

    MapLayerLevelOfDetail* def = map_layer->getDefinition( key.createParentKey() );
    if ( def )
    {
        cell_env->setInputSRS( def->getFeatureLayer()->getSRS() );
        cell_env->setExtent( map_layer->getAreaOfInterest().getSRS()->transform( key.getExtent() ) );
        cell_env->setProperty( Property( "compiler.cell_id", key.toString() ) );

        task = new CellCompiler(
            abs_path,
            def->getFeatureLayer(),
            def->getFilterGraph(),
            cell_env.get(),
            def->getResourcePackager()? def->getResourcePackager() : resource_packager.get(),
            getArchive() );

        osgGIS::notify( osg::INFO )
            << "Task: Key = " << key.toString() << ", LOD = " << key.getLOD() << ", Extent = " << key.getExtent().toString() 
            << " (w=" << key.getExtent().getWidth() << ", h=" << key.getExtent().getHeight() << ")"
            << std::endl;
    }

    return task;
}

//void
//QuadTreeMapLayerCompiler::setCenterAndRadius( osg::PagedLOD* plod, const QuadKey& key, SmartReadCallback* reader )
//{
//    SpatialReference* srs = map_layer->getOutputSRS( getSession(), terrain_srs.get() );
//    // first get the output srs centroid:
//    const GeoExtent& cell_extent = key.getExtent();    
//    GeoPoint centroid = srs->transform( cell_extent.getCentroid() );
//    GeoPoint sw = srs->transform( cell_extent.getSouthwest() );
//
//    double radius = (centroid-sw).length();
//    
//    if ( terrain_node.valid() && terrain_srs.valid() )
//    {
//        centroid = GeomUtils::clampToTerrain( centroid, terrain_node.get(), terrain_srs.get(), reader );
//    }
//
//    plod->setCenter( centroid );
//    plod->setRadius( radius );
//}

MapLayerCompiler::Profile*
QuadTreeMapLayerCompiler::createProfile()
{
    // determine the output SRS:
    osg::ref_ptr<SpatialReference> out_srs = map_layer->getOutputSRS( getSession(), terrain_srs.get() );
    if ( !out_srs.valid() )
    {
        osgGIS::notify( osg::WARN ) << "Unable to figure out the output SRS; aborting." << std::endl;
        return false;
    }

    // figure out the bounds of the compilation area and create a Q map. We want a sqaure AOI..maybe
    const GeoExtent& aoi = map_layer->getAreaOfInterest();
    if ( !aoi.isValid() )
    {
        osgGIS::notify( osg::WARN ) << "Invalid area of interest in the map layer - no data?" << std::endl;
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

    osgGIS::notify( osg::NOTICE )
        << "QMAP: " << std::endl
        << "   Base LOD = " << qmap.getStartingLodForCellsOfSize( map_layer->getCellWidth(), map_layer->getCellHeight() ) << std::endl
        << "   Extent = " << qmap.getBounds().toString() << ", w=" << qmap.getBounds().getWidth() << ", h=" << qmap.getBounds().getHeight() << std::endl
        << std::endl;

    return new QuadTreeProfile( qmap );
}

unsigned int
QuadTreeMapLayerCompiler::queueTasks( MapLayerCompiler::Profile* _profile, TaskManager* task_man )
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

            // enter the geometry as a proxy node (so we can rebuild it without modifying
            // the index)
            //osg::PagedLOD* proxy = new osg::PagedLOD();
            osg::ProxyNode* proxy = new osg::ProxyNode();
            proxy->setLoadingExternalReferenceMode( osg::ProxyNode::LOAD_IMMEDIATELY );
            proxy->setFileName( 0, createRelPathFromTemplate( "g" + subkey.toString() ) );
            //proxy->setRange( 0, 0, 1e10 );
            //proxy->setCenter( calculateCentroid( key ) );
        
            // enter the subtile set as a paged index node reference:
            osg::PagedLOD* plod = new osg::PagedLOD();
            plod->addChild( proxy, max_range, 1e10 );
            plod->setFileName( 1, createRelPathFromTemplate( "i" + subkey.toString() ) );
            //plod->setRange( 1, min_range, max_range ); // last one should always be min=0
            plod->setRange( 1, 0, max_range ); // last one should always be min=0
            plod->setPriorityScale( 1, MY_PRIORITY_SCALE );
            //plod->setCenter( calculateCentroid( key ) );
            setCenterAndRadius( plod, key.getExtent(), reader );

            group->addChild( plod );
        }
    }

    return group;
}

// creates an index node (pointer to quadkey geometry nodes) that has no
// children.
osg::Node*
QuadTreeMapLayerCompiler::createLeafIndexNode( const QuadKey& key )
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

            //osg::PagedLOD* proxy = new osg::PagedLOD();
            osg::ProxyNode* proxy = new osg::ProxyNode();
            proxy->setLoadingExternalReferenceMode( osg::ProxyNode::LOAD_IMMEDIATELY );
            proxy->setFileName( 0, createRelPathFromTemplate( "g" + quadrant_key.toString() ) );
            //proxy->setCenter( calculateCentroid( quadrant_key ) );
            //proxy->setRange( 0, 0, 1e10 );

            group->addChild( proxy );
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
    unsigned int starting_lod = qmap.getStartingLodForCellsOfSize( map_layer->getCellWidth(), map_layer->getCellHeight() );

    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
    {
        MapLayerLevelOfDetail* level_def = i->get();
        unsigned int lod = starting_lod + level_def->getDepth();
        
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
QuadTreeMapLayerCompiler::buildIndex( Profile* _profile )
{
    QuadTreeProfile* profile = dynamic_cast<QuadTreeProfile*>( _profile );
    if ( !profile ) return;

    // first, determine the SRS of the output scene graph so that we can
    // make pagedlod/lod centroids.
    SpatialReference* output_srs = map_layer->getOutputSRS( getSession(), getTerrainSRS() );

    // first build the very top level.
    scene_graph = new osg::Group();

    // the starting LOD is the best fit the the cell size:
    unsigned int starting_lod = profile->getQuadMap().getStartingLodForCellsOfSize( 
        map_layer->getCellWidth(), 
        map_layer->getCellHeight() );

    osg::ref_ptr<SmartReadCallback> reader = new SmartReadCallback();

    for( MapLayerLevelsOfDetail::iterator i = map_layer->getLevels().begin(); i != map_layer->getLevels().end(); i++ )
    {
        MapLayerLevelOfDetail* level_def = i->get();
        unsigned int lod = starting_lod + level_def->getDepth();

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
                    createLeafIndexNode( key );
                
                if ( node.valid() )
                {
                    std::string out_file = createAbsPathFromTemplate( "i" + key.toString() );

                    if ( !osgDB::writeNodeFile( *(node.get()), out_file ) )
                    {
                        osgGIS::notify(osg::WARN) << "FAILED to write index file " << out_file << std::endl;
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

                        scene_graph->addChild( plod );
                    }
                }
            }
        }
    }
}
