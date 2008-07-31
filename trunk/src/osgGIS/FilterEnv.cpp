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

#include <osgGIS/FilterEnv>

using namespace osgGIS;

FilterEnv::FilterEnv( Session* _session )
{
    session = _session;
    extent = GeoExtent::infinite();  
    resource_cache = new ResourceCache();
}


FilterEnv::FilterEnv( const FilterEnv& rhs )
{
    session = rhs.session.get();
    extent = rhs.extent;
    in_srs = rhs.in_srs.get();
    out_srs = rhs.out_srs.get();
    terrain_node = rhs.terrain_node.get();
    terrain_srs = rhs.terrain_srs.get();
    terrain_read_cb = rhs.terrain_read_cb.get();
    script_engine = rhs.script_engine.get();
    properties = rhs.properties;
    optimizer_hints = rhs.optimizer_hints;
    resource_cache = rhs.resource_cache.get();
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

SpatialReference*
FilterEnv::getInputSRS() 
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


SpatialReference*
FilterEnv::getTerrainSRS() const
{
    return terrain_srs.get();
}


void 
FilterEnv::setTerrainSRS( const SpatialReference* srs )
{
    terrain_srs = (SpatialReference*)srs;
}


void
FilterEnv::setTerrainReadCallback( SmartReadCallback* value )
{
    terrain_read_cb = value;
}


SmartReadCallback*
FilterEnv::getTerrainReadCallback()
{
    return terrain_read_cb.get();
}

void
FilterEnv::setScriptEngine( ScriptEngine* _engine )
{
    script_engine = _engine;
}

ScriptEngine*
FilterEnv::getScriptEngine()
{
    if ( !script_engine.valid() && session.valid() )
        script_engine = session->createScriptEngine();

    return script_engine.get();
}

Session*
FilterEnv::getSession()
{
    return session.get();
}

void 
FilterEnv::setProperty( const Property& prop )
{
    properties.push_back( prop );
}

Properties&
FilterEnv::getProperties()
{
    return properties;
}

Property
FilterEnv::getProperty( const char* name ) const
{
    return properties.get( std::string( name ) );
}

Property
FilterEnv::getProperty( const std::string& name ) const
{
    return properties.get( name );
}

OptimizerHints&
FilterEnv::getOptimizerHints()
{
    return optimizer_hints;
}

ResourceCache*
FilterEnv::getResourceCache()
{
    return resource_cache.get();
}

