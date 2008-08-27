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

#include <osgGIS/FeatureLayer>
#include <osgGIS/SimpleSpatialIndex>
#include <osgGIS/RTreeSpatialIndex>
#include <osgGIS/Registry>
#include <osg/Notify>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/ReentrantMutex>

using namespace osgGIS;
using namespace OpenThreads;

FeatureLayer::FeatureLayer( FeatureStore* _store )
{
    store = _store;
}


FeatureLayer::~FeatureLayer()
{
    //NOP
}

void
FeatureLayer::assertSpatialIndex()
{
    if ( store.valid() && !index.valid() )
    {
        ScopedLock<ReentrantMutex> lock( osgGIS::Registry::instance()->getGlobalMutex() );

        osgGIS::notice() << "Initializing spatial index..." << std::flush;

        index = new RTreeSpatialIndex( store.get() );

        osgGIS::notice() << "done." << std::endl;
    }
}

const std::string&
FeatureLayer::getName() const
{
    return store->getName();
}


const GeoExtent
FeatureLayer::getExtent() const
{
    GeoExtent e = store->getExtent();
    if ( e.isValid() )
        return e;

    osgGIS::warn() << "Store " << store->getName() << " did not report an extent; calculating..." << std::endl;

    const_cast<FeatureLayer*>(this)->assertSpatialIndex();
    return index.valid()? index->getExtent() : store.valid()? store->getExtent() : GeoExtent::invalid();
}


Feature*
FeatureLayer::getFeature( const FeatureOID& oid )
{
    return store.valid()? store->getFeature( oid ) : NULL;
}



SpatialIndex*
FeatureLayer::getSpatialIndex()
{
    assertSpatialIndex();
    return index.get();
}


void
FeatureLayer::setSpatialIndex( SpatialIndex* _index )
{
    index = _index;
    if ( !index.valid() && store.valid() )
    {
        index = new SimpleSpatialIndex( store.get() );
    }
}


SpatialReference* 
FeatureLayer::getSRS() const
{
    return 
        assigned_srs.valid()? assigned_srs.get() :
        store.valid()? store->getSRS() :
        NULL;
}


void
FeatureLayer::setSRS( const SpatialReference* _srs )
{
    assigned_srs = (SpatialReference*)_srs;
}


FeatureStore*
FeatureLayer::getFeatureStore()
{
    return store.get();
}


FeatureCursor
FeatureLayer::getCursor()
{
    return store.valid()? store->getCursor() : FeatureCursor();
}


FeatureCursor
FeatureLayer::getCursor( const GeoExtent& extent )
{
    if ( extent.isInfinite() )
    {
        return getCursor();
    }
    else
    {
        assertSpatialIndex();
        if ( index.valid() )
        {
            return index->getCursor( extent );
        }
    }

    osgGIS::notify( osg::WARN )
        << "osgGIS::FeatureLayer::createCursor, no spatial index available" << std::endl;
    return FeatureCursor();
}


FeatureCursor
FeatureLayer::getCursor( const GeoPoint& point )
{
    if ( point.isValid() )
    {        
        assertSpatialIndex();
        return index->getCursor( GeoExtent( point, point ), true );
    }
    else
    {
        return FeatureCursor(); // empty
    }
}

