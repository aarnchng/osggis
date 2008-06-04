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

#include <osgGIS/Report>

using namespace osgGIS;

Report::Report()
{
    //NOP
}

Report::Report( const Report& rhs )
: name( rhs.name )
{
    //NOP
}

Report::~Report()
{
    //NOP
}

void
Report::setName( const std::string& value )
{
    name = value;
}

const std::string&
Report::getName() const
{
    return name;
}

void
Report::setProperty( const Property& p )
{
    //NOP
}

Properties
Report::getProperties() const
{
    return Properties();
}

