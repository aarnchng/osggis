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

#include <osgGIS/FeatureCursor>
#include <osgGIS/FeatureStore>
#include <osgGIS/OGR_Utils>
#include <algorithm>

using namespace osgGIS;

#define DEFAULT_PREFETCH_SIZE 64

FeatureCursor::FeatureCursor(const FeatureOIDList& _oids,
                             FeatureStore*         _store,
                             const GeoExtent&      _search_extent,
                             bool                  _match_exactly)
{
    oids  = _oids;
    store = _store;
    search_extent = _search_extent;
    match_exactly = _match_exactly;
    prefetch_size = DEFAULT_PREFETCH_SIZE;
    at_bof = false;
    reset();
}

FeatureCursor::FeatureCursor()
{
    iter = 0;
    at_bof = false;
}

FeatureCursor::FeatureCursor( const FeatureCursor& rhs )
: oids( rhs.oids ),
  store( rhs.store.get() ),
  iter( rhs.iter ),
  search_extent( rhs.search_extent ),
  match_exactly( rhs.match_exactly ),
  prefetch_size( rhs.prefetch_size ),
  prefetched_results( rhs.prefetched_results ),
  last_result( rhs.last_result.get() ),
  at_bof( rhs.at_bof )
{
    //NOP
}

FeatureCursor::~FeatureCursor()
{
    //NOP
}

void
FeatureCursor::setPrefetchSize( int value )
{
    prefetch_size = std::max( value, 1 );
}

void
FeatureCursor::reset()
{
    if ( !at_bof )
    {
        iter = 0;
        while( !prefetched_results.empty() )
            prefetched_results.pop();
        last_result = NULL;
        prefetch();
        at_bof = true;
    }
}

bool
FeatureCursor::hasNext() const
{
    return store.valid() && prefetched_results.size() > 0; //next_result.valid();
}

Feature*
FeatureCursor::next()
{
    at_bof = false;

    if ( prefetched_results.size() == 0 )
    {
        last_result = NULL;
    }
    else
    {
        last_result = prefetched_results.front().get();
        prefetched_results.pop();
    }

    prefetch();

    return last_result.get();
}

void
FeatureCursor::prefetch()
{
    if ( store.valid() && prefetched_results.size() <= 1 )
    {
        //TODO: make this implementation-independent:
        OGR_SCOPE_LOCK();

        while( prefetched_results.size() < prefetch_size && iter < oids.size() )
        {
            Feature* f = store->getFeature( oids[iter++] );
            if ( f )
            {
                bool match = true;
                if ( match_exactly )
                {
                    match = f->getShapes().intersects( search_extent );
                }

                if ( match )
                {
                    prefetched_results.push( f );
                }
            }
        }
    }
}

