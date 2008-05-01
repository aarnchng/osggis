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

#include <osgGIS/FeatureCursor>
#include <osgGIS/FeatureStore>

using namespace osgGIS;


FeatureCursor::FeatureCursor(const FeatureOIDList& _oids,
                             FeatureStore*         _store,
                             const GeoExtent&      _search_extent,
                             bool                  _match_exactly)
{
    oids  = _oids;
    store = _store;
    search_extent = _search_extent;
    match_exactly = _match_exactly;
    reset();
}

FeatureCursor::FeatureCursor()
{
    iter = 0;
}

FeatureCursor::~FeatureCursor()
{
    //NOP
}

void
FeatureCursor::reset()
{
    iter = 0;
    prefetch();
}

bool
FeatureCursor::hasNext() const
{
    return store.valid() && next_result.valid();
}

Feature*
FeatureCursor::next()
{
    last_result = next_result.get();
    prefetch();
    return last_result.get();
}

void
FeatureCursor::prefetch()
{
    bool match = false;
    while( store.valid() && !match && iter < oids.size() )
    {
        next_result = store->getFeature( oids[iter++] );
        if ( !match_exactly )
            return;

        match = next_result->getShapes().intersects( search_extent );
    }
    if ( !match )
        next_result = NULL;
}