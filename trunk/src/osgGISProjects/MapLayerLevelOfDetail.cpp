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

#include <osgGISProjects/MapLayerLevelOfDetail>

using namespace osgGIS;
using namespace osgGISProjects;

MapLayerLevelOfDetail::MapLayerLevelOfDetail(FeatureLayer*     _layer,
                                             FilterGraph*      _graph,
                                             const Properties& _env_props,
                                             ResourcePackager* _packager,
                                             float             _min_range,
                                             float             _max_range,
                                             bool              _replace_previous,
                                             unsigned int      _depth,
                                             osg::Referenced*  _user_data )
{
    layer            = _layer;
    graph            = _graph;
    env_props        = _env_props;
    packager         = _packager;
    min_range        = _min_range;
    max_range        = _max_range;
    replace_previous = _replace_previous;
    depth            = _depth;
    user_data        = _user_data;
}

FeatureLayer*
MapLayerLevelOfDetail::getFeatureLayer() const {
    return layer.get();
}

FilterGraph*
MapLayerLevelOfDetail::getFilterGraph() const {
    return graph.get();
}

ResourcePackager*
MapLayerLevelOfDetail::getResourcePackager() const {
    return packager.get();
}

float
MapLayerLevelOfDetail::getMinRange() const {
    return min_range;
}

float
MapLayerLevelOfDetail::getMaxRange() const {
    return max_range;
}

bool
MapLayerLevelOfDetail::getReplacePrevious() const {
    return replace_previous;
}

unsigned int
MapLayerLevelOfDetail::getDepth() const {
    return depth;
}

osg::Referenced*
MapLayerLevelOfDetail::getUserData() const {
    return user_data.get();
}

const Properties&
MapLayerLevelOfDetail::getEnvProperties() const {
    return env_props;
}
