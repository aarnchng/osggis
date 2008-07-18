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

#include <osgGIS/FragmentFilterState>
#include <osgGIS/CollectionFilterState>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>

using namespace osgGIS;

FragmentFilterState::FragmentFilterState( FragmentFilter* _filter )
{
    filter = _filter;
}

void
FragmentFilterState::push( Feature* input )
{
    if ( input && input->hasShapeData() )
        in_features.push_back( input );
}

void
FragmentFilterState::push( const FeatureList& input )
{
    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
        if ( i->get()->hasShapeData() )
            in_features.push_back( i->get() );
}

void
FragmentFilterState::push( Fragment* input )
{
    in_fragments.push_back( input );
}

void
FragmentFilterState::push( const FragmentList& input )
{
    in_fragments.insert( in_fragments.end(), input.begin(), input.end() );
}

FilterStateResult
FragmentFilterState::traverse( FilterEnv* in_env )
{
    FilterStateResult result;

    current_env = in_env->advance();

    FilterState* next = getNextState();
    if ( next )
    {
        FragmentList output =
            in_features.size() > 0? filter->process( in_features, current_env.get() ) :
            in_fragments.size() > 0? filter->process( in_fragments, current_env.get() ) :
            FragmentList();
        
        if ( output.size() > 0 )
        {
            if ( dynamic_cast<NodeFilterState*>( next ) )
            {
                NodeFilterState* state = static_cast<NodeFilterState*>( next );
                state->push( output );
            }
            else if ( dynamic_cast<FragmentFilterState*>( next ) )
            {
                FragmentFilterState* state = static_cast<FragmentFilterState*>( next );
                state->push( output );
            }
            else if ( dynamic_cast<CollectionFilterState*>( next ) )
            {
                CollectionFilterState* state = static_cast<CollectionFilterState*>( next );
                state->push( output );
            }

            result = next->traverse( current_env.get() );
        }
        else
        {
            result.set( FilterStateResult::STATUS_NODATA, filter.get() );
        }
    }

    in_features.clear();
    in_fragments.clear();

    return result;
}

