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

#include <osgGIS/FeatureLayerResource>
#include <OpenThreads/ScopedLock>

using namespace osgGIS;
using namespace OpenThreads;

#include <osgGIS/Registry>
OSGGIS_DEFINE_RESOURCE(FeatureLayerResource);


FeatureLayerResource::FeatureLayerResource()
{
    init();
}

FeatureLayerResource::FeatureLayerResource( const std::string& _name )
: Resource( _name )
{
    init();
}

void
FeatureLayerResource::init()
{
    //NOP
}

FeatureLayerResource::~FeatureLayerResource()
{
    //NOP
}

void
FeatureLayerResource::setProperty( const Property& prop )
{
    Resource::setProperty( prop );
}

Properties
FeatureLayerResource::getProperties() const
{
    Properties props = Resource::getProperties();
    return props;
}

FeatureLayer*
FeatureLayerResource::getFeatureLayer()
{
    ScopedLock<ReentrantMutex> sl( getMutex() );

    if ( !feature_layer.valid() )
    {
        feature_layer = Registry::instance()->createFeatureLayer( getAbsoluteURI() );
    }

    return feature_layer.get();
}