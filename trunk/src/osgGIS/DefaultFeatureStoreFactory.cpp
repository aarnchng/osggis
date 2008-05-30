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

#include <osgGIS/DefaultFeatureStoreFactory>
#include <osgGIS/OGR_FeatureStore>
#include <osgGIS/OGR_Utils>
#include <osgGIS/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Notify>

using namespace osgGIS;

DefaultFeatureStoreFactory::DefaultFeatureStoreFactory()
{
	OGR_Utils::registerAll();
}

DefaultFeatureStoreFactory::~DefaultFeatureStoreFactory()
{
	//NOP
}


FeatureStore*
DefaultFeatureStoreFactory::connectToFeatureStore( const std::string& uri )
{
	FeatureStore* result = NULL;

    result = new OGR_FeatureStore( uri );

	if ( !result )
	{
		osg::notify( osg::WARN ) << "Cannot find an appropriate feature store to handle URI: " << uri << std::endl;
	}
	else if ( !result->isReady() )
	{
		osg::notify( osg::WARN ) << "Unable to initialize feature store for URI: " << uri << std::endl;
		result->unref();
		result = NULL;
	}

	return result;
}

FeatureStore*
DefaultFeatureStoreFactory::createFeatureStore(const std::string& uri, 
                                               const GeoShape::ShapeType& type, 
                                               const AttributeSchemaList& schemas,
                                               int   dimensionality,
                                               const SpatialReference* srs,
                                               const Properties& props )
{
    FeatureStore* result = NULL;

    result = new OGR_FeatureStore( uri, type, schemas, dimensionality, srs, props );

    return result;
}
