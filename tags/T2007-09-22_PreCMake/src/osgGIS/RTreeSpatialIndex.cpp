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

#include <osgGIS/RTreeSpatialIndex>
#include <osgGIS/FeatureCursorImpl>
#include <osg/notify>
#include <algorithm>

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


FeatureCursor*
RTreeSpatialIndex::createCursor( const GeoExtent& extent )
{
    //TODO: replace this with an RTree iterator.
    std::list<FeatureOID> oids = rtree->find( extent );

    FeatureOIDList vec( oids.size() );
    int k = 0;
    for( std::list<FeatureOID>::iterator i = oids.begin(); i != oids.end(); i++ )
        vec[k++] = *i;

    return new FeatureCursorImpl( vec, store.get() );
}


bool
RTreeSpatialIndex::buildIndex()
{
    rtree = new RTree<FeatureOID>();
    osg::ref_ptr<FeatureCursor> cursor = store->createCursor();
    while( cursor->hasNext() )
    {
        Feature* f = cursor->next();
        const GeoExtent& extent = f->getExtent();
        if ( extent.isValid() && extent.getArea() > 0 )
        {
            rtree->insert( f->getExtent(), f->getOID() );
        }
    }
    return true;
}