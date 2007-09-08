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

#include <osgGIS/Registry>
#include <osgGIS/DefaultFeatureStoreFactory>
#include <osgGIS/OGR_SpatialReferenceFactory>

using namespace osgGIS;

osgGIS::Registry* osgGIS::Registry::singleton = NULL;


Registry::Registry()
{
	setSRSFactory( new OGR_SpatialReferenceFactory() );
	setFeatureStoreFactory( new DefaultFeatureStoreFactory() );
}


Registry::~Registry()
{
	//NOP
}


osgGIS::Registry*
Registry::instance()
{
	if ( singleton == NULL )
	{
		singleton = new Registry();
	}
	return singleton;
}


FeatureLayer*
Registry::createFeatureLayer( const std::string& uri )
{
    FeatureLayer* result = NULL;
    if ( getFeatureStoreFactory() )
    {
        osg::ref_ptr<FeatureStore> store = 
            getFeatureStoreFactory()->connectToFeatureStore( uri );

        if ( store.valid() && store->isReady() )
        {
            result = new FeatureLayer( store.get() );
        }
    }
    return result;
}


SpatialReferenceFactory*
Registry::getSRSFactory()
{
	return spatial_ref_factory.get();
}


void
Registry::setSRSFactory( SpatialReferenceFactory* _factory )
{
	spatial_ref_factory = _factory;
}


FeatureStoreFactory*
Registry::getFeatureStoreFactory()
{
	return feature_store_factory.get();
}


void
Registry::setFeatureStoreFactory( FeatureStoreFactory* _factory )
{
	feature_store_factory = _factory;
}