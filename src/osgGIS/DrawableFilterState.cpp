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

#include <osgGIS/DrawableFilterState>
#include <osgGIS/CollectionFilterState>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>

using namespace osgGIS;

DrawableFilterState::DrawableFilterState( DrawableFilter* _filter )
{
    filter = _filter;
}

void
DrawableFilterState::push( Feature* input )
{
    in_features.push_back( input );
}

void
DrawableFilterState::push( const FeatureList& input )
{
    in_features.insert( in_features.end(), input.begin(), input.end() );
}

void
DrawableFilterState::push( osg::Drawable* input )
{
    in_drawables.push_back( input );
}

void
DrawableFilterState::push( const DrawableList& input )
{
    in_drawables.insert( in_drawables.end(), input.begin(), input.end() );
}

void
DrawableFilterState::reset( ScriptContext* context )
{
    in_features.clear();
    in_drawables.clear();
    FilterState::reset( context );
}

bool
DrawableFilterState::traverse( FilterEnv* in_env )
{
    bool ok = true;

    osg::ref_ptr<FilterEnv> env = in_env->advance();

    FilterState* next = getNextState();
    if ( next )
    {
        DrawableList output =
            in_features.size() > 0? filter->process( in_features, env.get() ) :
            in_drawables.size() > 0? filter->process( in_drawables, env.get() ) :
            DrawableList();
        
        if ( dynamic_cast<NodeFilterState*>( next ) )
        {
            NodeFilterState* state = static_cast<NodeFilterState*>( next );
            state->push( output );
        }
        else if ( dynamic_cast<DrawableFilterState*>( next ) )
        {
            DrawableFilterState* state = static_cast<DrawableFilterState*>( next );
            state->push( output );
        }
        else if ( dynamic_cast<CollectionFilterState*>( next ) )
        {
            CollectionFilterState* state = static_cast<CollectionFilterState*>( next );
            state->push( output );
        }

        ok = next->traverse( env.get() );
    }

    in_features.clear();
    in_drawables.clear();

    return ok;
}
