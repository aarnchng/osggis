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

#include <osgGIS/FeatureCursorImpl>

using namespace osgGIS;


FeatureCursorImpl::FeatureCursorImpl(const FeatureOIDList& _oids,
                                     FeatureStore*         _store )
{
    oids  = _oids;
    store = _store;
    iter  = 0;
}


FeatureCursorImpl::~FeatureCursorImpl()
{
    //NOP
}


void
FeatureCursorImpl::reset()
{
    iter = 0;
}


bool
FeatureCursorImpl::hasNext() const
{
    return store.valid() && iter < oids.size();
}


Feature*
FeatureCursorImpl::next()
{
    if ( hasNext() )
    {
        last_result = store->getFeature( oids[iter++] );
    }

    return last_result.get();
}
