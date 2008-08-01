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

#include <osgGIS/DefaultRasterStoreFactory>
#include <osgGIS/GDAL_RasterStore>
#include <osgGIS/OGR_Utils>
#include <osgGIS/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osg/Notify>

using namespace osgGIS;

DefaultRasterStoreFactory::DefaultRasterStoreFactory()
{
	OGR_Utils::registerAll();
}

DefaultRasterStoreFactory::~DefaultRasterStoreFactory()
{
	//NOP
}


RasterStore*
DefaultRasterStoreFactory::connectToRasterStore( const std::string& uri )
{
	RasterStore* result = NULL;

    result = new GDAL_RasterStore( uri );

	if ( !result )
	{
		osgGIS::notify( osg::WARN ) << "Cannot find an appropriate raster store to handle URI: " << uri << std::endl;
	}
	else if ( !result->isReady() )
	{
		osgGIS::notify( osg::WARN ) << "Unable to initialize raster store for URI: " << uri << std::endl;
		result->unref();
		result = NULL;
	}

	return result;
}

