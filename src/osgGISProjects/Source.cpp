/**
 * osgGIS - GIS Library for OpenSceneGraph
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

#include <osgGISProjects/Source>
#include <osgGIS/Utils>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

using namespace osgGISProjects;
using namespace osgGIS;

Source::Source()
{
    //NOP
}

Source::Source( const std::string& _uri )
{
    setURI( _uri );
}

Source::~Source()
{
    //NOP
}

void
Source::setBaseURI( const std::string& value )
{
    base_uri = value;
}

void
Source::setName( const std::string& value )
{
    name = value;
}

const std::string&
Source::getName() const
{
    return name;
}

const Source::Type&
Source::getType() const
{
    return type;
}

void
Source::setType( const Source::Type& value )
{
    type = value;
}

void
Source::setURI( const std::string& value )
{
    uri = value;
}

const std::string&
Source::getURI() const
{
    return uri;
}

const std::string
Source::getAbsoluteURI() const
{
    return PathUtils::getAbsPath( base_uri, uri );
}

Properties& 
Source::getProperties()
{
    return props;
}

const Properties& 
Source::getProperties() const
{
    return props;
}

void
Source::setParentSource( Source* value )
{
    parent_source = value;
}

Source*
Source::getParentSource() const
{
    return (const_cast<const Source*>(this))->parent_source.get();
}

bool
Source::isIntermediate() const
{
    return getParentSource() != NULL;
}

void 
Source::setFilterGraph( FilterGraph* value )
{
    filter_graph = value;
}

FilterGraph*
Source::getFilterGraph() const
{
    return (const_cast<const Source*>(this))->filter_graph.get();
}

bool
Source::needsRefresh() const
{
    //TODO
    return true;
}

long
Source::getTimeLastModified() const
{
    return FileUtils::getFileTimeUTC( getAbsoluteURI() );
}

