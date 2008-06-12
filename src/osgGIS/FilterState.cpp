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

#include <osgGIS/FilterState>
#include <osgGIS/Filter>

using namespace osgGIS;

FilterState::FilterState()
{
    //NOP
}

FilterState*
FilterState::setNextState( FilterState* _next_state )
{
    //TODO: validate the the input filter is valid here
    next_state = _next_state;
    return next_state.get();
}

FilterState*
FilterState::getNextState()
{
    return next_state.get();
}

FilterState*
FilterState::appendState( FilterState* _state )
{
    if ( next_state.valid() )
    {
        next_state->appendState( _state );
    }
    else
    {
        next_state = _state;
    }

    return next_state.get();
}

bool
FilterState::signalCheckpoint()
{
    FilterState* next = getNextState();
    return next? next->signalCheckpoint() : true;
}


