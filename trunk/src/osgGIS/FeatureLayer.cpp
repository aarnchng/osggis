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

#include <osgGIS/FeatureLayer>
#include <osgGIS/SimpleSpatialIndex>
#include <osgGIS/RTreeSpatialIndex>
#include <osg/Notify>

using namespace osgGIS;

FeatureLayer::FeatureLayer( FeatureStore* _store )
{
    store = _store;

    if ( store.valid() )
    {
        osg::notify(osg::NOTICE) << "Building spatial index..." << std::flush;

        index = new RTreeSpatialIndex( store.get() );

        osg::notify(osg::NOTICE) << "done." << std::endl;
    }
}


FeatureLayer::~FeatureLayer()
{
    //NOP
    osg::notify(osg::NOTICE) << "dtor" << std::endl;
}


const std::string&
FeatureLayer::getName() const
{
    return store->getName();
}


const GeoExtent
FeatureLayer::getExtent() const
{
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
    else if ( index.valid() )
    {
        return index->getCursor( extent );
    }
    else 
    {
        osg::notify( osg::WARN )
            << "osgGIS::FeatureLayer::createCursor, no spatial index available" << std::endl;
        return FeatureCursor();
    }
}


FeatureCursor
FeatureLayer::getCursor( const GeoPoint& point )
{
    if ( point.isValid() )
    {
        return index->getCursor( GeoExtent( point, point ), true );
    }
    else
    {
        return FeatureCursor(); // empty
    }
}

