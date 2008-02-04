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
    in_features.push_back( input );
}

void
NodeFilterState::push( FeatureList& input )
{
    in_features.insert( in_features.end(), input.begin(), input.end() );
}

void
NodeFilterState::push( osg::Drawable* input )
{
    in_drawables.push_back( input );
}

void
NodeFilterState::push( DrawableList& input )
{
    in_drawables.insert( in_drawables.end(), input.begin(), input.end() );
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
    else if ( in_drawables.size() > 0 )
    {
        out_nodes = filter->process( in_drawables, env.get() );
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

        out_nodes.clear();
        ok = next->traverse( env.get() );
    }

    in_features.clear();
    in_drawables.clear();
    in_nodes.clear();

    return ok;
}

osg::NodeList&
NodeFilterState::getOutput()
{
    return out_nodes;
}

