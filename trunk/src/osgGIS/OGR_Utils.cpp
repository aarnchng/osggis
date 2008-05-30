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

#include <osgGIS/OGR_Utils>
#include <ogr_api.h>
#include <gdal_priv.h>

using namespace osgGIS;

bool OGR_Utils::is_registered = false;
OpenThreads::ReentrantMutex* OGR_Utils::ogr_mutex = NULL;

void
OGR_Utils::registerAll()
{
	if ( !is_registered )
	{
        OGR_SCOPE_LOCK();
		OGRRegisterAll();
        GDALAllRegister();
		is_registered = true;
	}
}

OpenThreads::ReentrantMutex&
OGR_Utils::getMutex()
{
    if ( !ogr_mutex )
        ogr_mutex = new OpenThreads::ReentrantMutex();
    return *ogr_mutex;
}
