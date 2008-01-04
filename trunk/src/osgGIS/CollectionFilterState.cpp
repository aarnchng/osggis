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

#include <osgGIS/CollectionFilterState>
#include <osgGIS/DrawableFilterState>
#include <osgGIS/FeatureFilterState>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>

using namespace osgGIS;

CollectionFilterState::CollectionFilterState( CollectionFilter* _filter )
{
    filter = _filter;
}

void 
CollectionFilterState::push( const FeatureList& input )
{
    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
        push( (Feature*)( i->get() ) );
}


void
CollectionFilterState::push( Feature* input )
{
    feature_groups[ filter->assign( input ) ].push_back( input );
}


void 
CollectionFilterState::push( const DrawableList& input )
{
    for( DrawableList::const_iterator i = input.begin(); i != input.end(); i++ )
        push( (osg::Drawable*)( i->get() ) );
}


void
CollectionFilterState::push( osg::Drawable* input )
{
    drawable_groups[ filter->assign( input ) ].push_back( input );
}


void 
CollectionFilterState::push( const osg::NodeList& input )
{
    for( osg::NodeList::const_iterator i = input.begin(); i != input.end(); i++ )
        push( i->get() );
}


void
CollectionFilterState::push( osg::Node* input )
{
    node_groups[ filter->assign( input ) ].push_back( input );
}


bool 
CollectionFilterState::traverse( FilterEnv* env )
{
    // just save a copy of the env for checkpoint time.
    saved_env = env->advance();
    return true;
}

template<typename A, typename B>
static bool
meterData( A source, B state, unsigned int metering, FilterEnv* env )
{
    bool ok = true;

    //osg::notify( osg::ALWAYS ) << "Metering " << source.size() << " units." << std::endl;

    if ( metering == 0 )
    {
        state->push( source );
        ok = state->traverse( env );
    }
    else
    {
        unsigned int batch_size = 0;
        for( typename A::iterator i = source.begin(); i < source.end() && ok; i += batch_size )
        {
            unsigned int remaining = source.end()-i;
            batch_size = std::min( remaining, metering );
            A partial;
            partial.insert( partial.end(), i, i + batch_size );
            state->push( partial );
            ok = state->traverse( env );

            //osg::notify( osg::ALWAYS )
            //    << "Metered: " << i-source.begin() << "/" << source.end()-source.begin()
            //    << std::endl;
        }
    }
    return ok;
}

template<typename A, typename B>
static bool
meterGroups( A source, B state, unsigned int metering, FilterEnv* env )
{
    bool ok = true;
    for( typename A::iterator i = source.begin(); i != source.end() && ok; i++ )
    {
        ok = meterData( i->second, state, metering, env );
    }
    return ok;
}


bool 
CollectionFilterState::signalCheckpoint()
{
    bool ok = true;

    FilterState* next = getNextState();
    if ( next )
    {
        int metering = filter->getMetering();

        if ( dynamic_cast<FeatureFilterState*>( next ) )
        {
            FeatureFilterState* state = static_cast<FeatureFilterState*>( next );
            ok = meterGroups( feature_groups, state, metering, saved_env.get() );
        }
        else if ( dynamic_cast<DrawableFilterState*>( next ) )
        {
            DrawableFilterState* state = static_cast<DrawableFilterState*>( next );
            if ( feature_groups.size() > 0 )
                ok = meterGroups( feature_groups, state, metering, saved_env.get() );
            else if ( drawable_groups.size() > 0 )
                ok = meterGroups( drawable_groups, state, metering, saved_env.get() );
            else
                ok = false;
        }
        else if ( dynamic_cast<NodeFilterState*>( next ) )
        {
            NodeFilterState* state = static_cast<NodeFilterState*>( next );
            if ( drawable_groups.size() > 0 )
                ok = meterGroups( drawable_groups, state, metering, saved_env.get() );
            else if ( node_groups.size() > 0 )
                ok = meterGroups( node_groups, state, metering, saved_env.get() );
            else
                ok = false;
        }
        else if ( dynamic_cast<CollectionFilterState*>( next ) )
        {
            CollectionFilterState* state = static_cast<CollectionFilterState*>( next );   
            if ( feature_groups.size() > 0 )
                ok = meterGroups( feature_groups, state, metering, saved_env.get() );
            else if ( drawable_groups.size() > 0 )
                ok = meterGroups( drawable_groups, state, metering, saved_env.get() );
            else if ( node_groups.size() > 0 )
                ok = meterGroups( node_groups, state, metering, saved_env.get() );
            else
                ok = false;         
        }

        if ( ok )
        {
            ok = next->signalCheckpoint();
        }
    }

    // clean up the input:
    feature_groups.clear();
    drawable_groups.clear();
    node_groups.clear();
    saved_env = NULL;

    return ok;
}