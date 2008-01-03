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

using namespace osgGIS;

Resource::Resource()
{
    //NOP
}

Resource::Resource( const std::string& _name )
{
    setName( _name );
}

Resource::~Resource()
{
    //NOP
}

void
Resource::setName( const std::string& value )
{
    name = value;
}

const std::string&
Resource::getName() const
{
    return name;
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

