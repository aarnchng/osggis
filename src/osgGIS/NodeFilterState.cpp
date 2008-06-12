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

#include <osgGIS/NodeFilterState>
#include <osgGIS/CollectionFilterState>
#include <osg/Notify>
#include <osg/Group>
#include <osg/Geode>

using namespace osgGIS;


NodeFilterState::NodeFilterState( NodeFilter* _filter )
{
    filter = _filter;
}

void
NodeFilterState::push( Feature* input )
{
    if ( input && input->hasShapeData() )
        in_features.push_back( input );
}

void
NodeFilterState::push( FeatureList& input )
{
    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
        if ( i->get()->hasShapeData() )
            in_features.push_back( i->get() );
}

void
NodeFilterState::push( Fragment* input )
{
    in_fragments.push_back( input );
}

void
NodeFilterState::push( FragmentList& input )
{
    in_fragments.insert( in_fragments.end(), input.begin(), input.end() );
}

void
NodeFilterState::push( osg::Node* input )
{
    in_nodes.push_back( input );
}

void
NodeFilterState::push( osg::NodeList& input )
{
    in_nodes.insert( in_nodes.end(), input.begin(), input.end() );
}

bool
NodeFilterState::traverse( FilterEnv* in_env )
{
    bool ok = true;

    osg::ref_ptr<FilterEnv> env = in_env->advance();

    if ( in_features.size() > 0 )
    {
        out_nodes = filter->process( in_features, env.get() );
    }
    else if ( in_fragments.size() > 0 )
    {
        out_nodes = filter->process( in_fragments, env.get() );
    }
    else if ( in_nodes.size() > 0 )
    {
        out_nodes = filter->process( in_nodes, env.get() );
    }
    
    FilterState* next = getNextState();
    if ( next && out_nodes.size() > 0 )
    {
        if ( dynamic_cast<NodeFilterState*>( next ) )
        {
            NodeFilterState* state = static_cast<NodeFilterState*>( next );
            state->push( out_nodes );
        }
        else if ( dynamic_cast<CollectionFilterState*>( next ) )
        {
            CollectionFilterState* state = static_cast<CollectionFilterState*>( next );
            state->push( out_nodes );
        }
        //else if ( dynamic_cast<DisperseFilterState*>( next ) )
        //{
        //    DisperseFilterState* state = static_cast<DisperseFilterState*>( next );
        //    state->push( out_nodes );
        //}

        out_nodes.clear();
        ok = next->traverse( env.get() );
    }

    in_features.clear();
    in_fragments.clear();
    in_nodes.clear();

    return ok;
}

osg::NodeList&
NodeFilterState::getOutput()
{
    return out_nodes;
}

