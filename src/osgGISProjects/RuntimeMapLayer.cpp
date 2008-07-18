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

#include <osgGISProjects/RuntimeMapLayer>

using namespace osgGISProjects;
using namespace osgGIS;

RuntimeMapLayer::RuntimeMapLayer()
{
    searchable = false;
    visible = true;
}

void
RuntimeMapLayer::setBuildLayer( BuildLayer* value )
{
    build_layer = value;
}

BuildLayer*
RuntimeMapLayer::getBuildLayer()
{
    return build_layer.get();
}

void
RuntimeMapLayer::setSearchLayer( BuildLayer* value )
{
    search_layer = value;
}

BuildLayer*
RuntimeMapLayer::getSearchLayer()
{
    return search_layer.valid()? search_layer.get() : getBuildLayer();
}

void
RuntimeMapLayer::setSearchable( bool value )
{
    searchable = value;
}

bool
RuntimeMapLayer::getSearchable() const
{
    return searchable;
}

void
RuntimeMapLayer::setVisible( bool value )
{
    visible = value;
}

bool
RuntimeMapLayer::getVisible() const
{
    return visible;
}

