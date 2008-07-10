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

#include <osgGIS/RTreeSpatialIndex>
#include <osgGIS/Registry>
#include <osgGIS/Utils>
#include <osg/Notify>
#include <osgDB/FileNameUtils>
#include <algorithm>
#include <fstream>

using namespace osgGIS;



RTreeSpatialIndex::RTreeSpatialIndex( FeatureStore* _store )
{
    store = _store;
    buildIndex();
}


RTreeSpatialIndex::~RTreeSpatialIndex()
{
    //NOP
}


FeatureCursor
RTreeSpatialIndex::getCursor( const GeoExtent& query_extent, bool match_exactly )
{
    GeoExtent ex(
        store->getSRS()->transform( query_extent.getSouthwest() ),
        store->getSRS()->transform( query_extent.getNortheast() ) );
        
    //TODO: replace this with an RTree iterator.
    std::list<FeatureOID> oids = rtree->find( ex );

    FeatureOIDList vec( oids.size() );
    int k = 0;
    for( std::list<FeatureOID>::iterator i = oids.begin(); i != oids.end(); i++ )
        vec[k++] = *i;

    return FeatureCursor( vec, store.get(), ex, match_exactly );
}


bool
RTreeSpatialIndex::buildIndex()
{
    bool loaded = false;

    bool cache_index = Registry::instance()->hasWorkDirectory();
    std::string index_name = osgDB::getSimpleFileName( store->getName() ) + "_spatialindex";

    if ( cache_index )
    {
        std::string cache_path = PathUtils::combinePaths(
            Registry::instance()->getWorkDirectory(),
            index_name );

        std::ifstream input( cache_path.c_str() );
        if ( input.is_open() )
        {
            rtree = new RTree<FeatureOID>();
            loaded = rtree->readFrom( input, store->getSRS(), extent );
            input.close();
            if ( loaded )
                osg::notify(osg::NOTICE) << "Loaded cached spatial index OK..";
        }    
    }
    
    if ( !loaded )
    {
        rtree = new RTree<FeatureOID>();

        for( FeatureCursor cursor = store->getCursor(); cursor.hasNext(); )
        {
            Feature* f = cursor.next();
            const GeoExtent& f_extent = f->getExtent();
            if ( f_extent.isValid() && !f_extent.isInfinite() ) //extent.getArea() > 0 )
            {
                rtree->insert( f_extent, f->getOID() );
                extent.expandToInclude( f_extent );
            }
        }

        if ( cache_index )
        {
            std::string cache_path = PathUtils::combinePaths(
                Registry::instance()->getWorkDirectory(),
                index_name );

            std::ofstream output( cache_path.c_str() );
            rtree->writeTo( output, extent );
            output.close();
        }

        loaded = true;
    }

    return loaded;
}

const GeoExtent&
RTreeSpatialIndex::getExtent() const
{
    return extent;
}

