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

#include <osgGIS/Resource>
#include <osgGIS/Utils>
#include <OpenThreads/ScopedLock>

using namespace osgGIS;
using namespace OpenThreads;

Resource::Resource()
{
    mutex = NULL;
    owns_mutex = false;
    setSingleUse( false );
}

Resource::Resource( const std::string& _name )
{
    mutex = NULL;
    owns_mutex = false;
    setName( _name );
    setSingleUse( false );
}

Resource::~Resource()
{
    //NOP
}

bool
Resource::isSingleUse() const
{
    return single_use;
}

void
Resource::setSingleUse( bool value )
{
    single_use = value;
}

void
Resource::setName( const std::string& value )
{
    name = value;
}

const char*
Resource::getName() const
{
    return name.c_str();
}

const std::string&
Resource::getURI() const
{
    return uri;
}

void
Resource::setURI( const std::string& value )
{
    uri = value;
}

const std::string
Resource::getAbsoluteURI() const
{
    return PathUtils::getAbsPath( getBaseURI(), getURI() );
}

void
Resource::setBaseURI( const std::string& value )
{
    base_uri = value;
}

const std::string&
Resource::getBaseURI() const
{
    return base_uri;
}

void
Resource::setProperty( const Property& prop )
{
    //NOP
}

Properties
Resource::getProperties() const
{
    return Properties();
}

ReentrantMutex&
Resource::getMutex()
{
    if ( !mutex )
    {
        mutex = new ReentrantMutex();
        owns_mutex = true;
    }
    return *mutex;
}

void
Resource::setMutex( ReentrantMutex& value )
{
    if ( mutex && owns_mutex )
        delete mutex;

    mutex = &value;
    owns_mutex = false;
}
