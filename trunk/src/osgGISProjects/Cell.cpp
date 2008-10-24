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
#include <osgGISProjects/Cell>

using namespace osgGISProjects;
using namespace osgGIS;


/************************************************************************/

CellStatus::CellStatus( bool _is_compiled, time_t _compiled_time ) :
is_compiled( _is_compiled ),
compiled_time( _compiled_time )
{
    //NOP
}

CellStatus::CellStatus( const CellStatus& rhs )
{
    is_compiled = rhs.is_compiled;
    compiled_time = rhs.compiled_time;
}

bool 
CellStatus::isCompiled() const {
    return is_compiled;
}

time_t 
CellStatus::getCompiledTime() const {
    return compiled_time;
}

/************************************************************************/

Cell::Cell( const std::string& _id, const GeoExtent& _extent) : //, const CellStatus& _status ) :
id( _id ),
extent( _extent )
//status( _status )
{
    //NOP
}

const std::string&
Cell::getId() const { return id; }

const GeoExtent&
Cell::getExtent() const { return extent; }

//const CellStatus&
//Cell::getStatus() const { return status; }

