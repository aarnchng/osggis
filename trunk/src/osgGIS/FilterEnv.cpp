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

#include <osgGIS/FilterEnv>

using namespace osgGIS;

FilterEnv::FilterEnv()
{
    extent = GeoExtent::infinite();   
}


FilterEnv::FilterEnv( const FilterEnv& rhs )
{
    extent = rhs.extent;
    in_srs = rhs.in_srs.get();
    out_srs = rhs.out_srs.get();
    terrain_node = rhs.terrain_node.get();
}


FilterEnv*
FilterEnv::advance() const
{
    FilterEnv* a = clone();
    a->setInputSRS( getOutputSRS() );
    a->setOutputSRS( getOutputSRS() );
    return a;
}


FilterEnv*
FilterEnv::clone() const
{
    FilterEnv* a = new FilterEnv( *this );
    return a;
}


FilterEnv::~FilterEnv()
{
    //NOP
}


void
FilterEnv::setExtent( const GeoExtent& _extent )
{
    extent = _extent;
}


const GeoExtent&
FilterEnv::getExtent() const
{
    return extent;
}


void
FilterEnv::setInputSRS( const SpatialReference* _srs )
{
    in_srs = (SpatialReference*)_srs;
}


const SpatialReference*
FilterEnv::getInputSRS() const
{
    return in_srs.get();
}


void
FilterEnv::setOutputSRS( const SpatialReference* _srs )
{
    out_srs = (SpatialReference*)_srs;
}


const SpatialReference*
FilterEnv::getOutputSRS() const
{
    return out_srs.get();
}


void
FilterEnv::setTerrainNode( osg::Node* _terrain )
{
    terrain_node = _terrain;
}


osg::Node*
FilterEnv::getTerrainNode()
{
    return terrain_node.get();
}