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

#include <osgGIS/SimpleSpatialIndex>

using namespace osgGIS;

SimpleSpatialIndex::SimpleSpatialIndex( FeatureStore* _store )
{
	store = _store;
    if ( store.valid() )
        buildIndex();
}


SimpleSpatialIndex::~SimpleSpatialIndex()
{
	//NOP
}


FeatureCursor
SimpleSpatialIndex::getCursor( const GeoExtent& query_extent, bool match_exactly )
{
    FeatureOIDList oids;
    for( FeatureCursor cursor = store->getCursor(); cursor.hasNext(); )
	{
		Feature* feature = cursor.next();
        const GeoExtent f_extent = feature->getExtent();
        if ( f_extent.intersects( query_extent ) )
        {
            oids.push_back( feature->getOID() );
        }
	}

    return FeatureCursor( oids, store.get(), query_extent, match_exactly );
}


const GeoExtent& 
SimpleSpatialIndex::getExtent() const
{
    return extent;
}

void
SimpleSpatialIndex::buildIndex()
{
    FeatureOIDList oids;
    for( FeatureCursor cursor = store->getCursor(); cursor.hasNext(); )
	{
		Feature* feature = cursor.next();
        extent.expandToInclude( feature->getExtent() );
    }
}


