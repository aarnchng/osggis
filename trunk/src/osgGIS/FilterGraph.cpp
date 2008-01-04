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

#include <osgGIS/FilterGraph>
#include <osgGIS/CollectionFilterState>
#include <osgGIS/DrawableFilterState>
#include <osgGIS/FeatureFilterState>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>
#include <osg/Timer>

using namespace osgGIS;

FilterGraph::FilterGraph()
{
}


FilterGraph::~FilterGraph()
{
}


const std::string&
FilterGraph::getName() const
{
    return name;
}


void
FilterGraph::setName( const std::string& value )
{
    name = value;
}

const FilterList&
FilterGraph::getFilters() const
{
    return filters;
}

FilterList&
FilterGraph::getFilters()
{
    return filters;
}


bool 
FilterGraph::appendFilter( Filter* filter )
{
    filters.push_back( filter );
    return true;
}


Filter*
FilterGraph::getFilter( const std::string& name )
{
    for( FilterList::iterator i = filters.begin(); i != filters.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get();
    }
    return NULL;
}

bool
FilterGraph::run( FeatureCursor* cursor, FilterEnv* env, osg::NodeList& output )
{
    bool ok = false;

    osg::ref_ptr<NodeFilterState> output_state;

    // first build a new state chain corresponding to our filter chain.
    osg::ref_ptr<FilterState> first = NULL;
    for( FilterList::iterator i = filters.begin(); i != filters.end(); i++ )
    {
        FilterState* next_state = i->get()->newState();
        if ( !first.valid() )
        {
            first = next_state;
        }
        else
        {
            first->appendState( next_state );
        }

        if ( dynamic_cast<NodeFilterState*>( next_state ) )
        {
            output_state = static_cast<NodeFilterState*>( next_state );
        }
    }

    // now traverse the states.
    if ( first.valid() )
    {
        ok = true;
        int count = 0;
        osg::Timer_t start = osg::Timer::instance()->tick();
        
        env->setOutputSRS( env->getInputSRS() );

        if ( dynamic_cast<FeatureFilterState*>( first.get() ) )
        {
            FeatureFilterState* state = static_cast<FeatureFilterState*>( first.get() );
            while( ok && cursor->hasNext() )
            {
                state->push( cursor->next() );
                ok = state->traverse( env );
                count++;
            }
            if ( ok )
            {
                ok = state->signalCheckpoint();
            }
        }
        else if ( dynamic_cast<DrawableFilterState*>( first.get() ) )
        {
            DrawableFilterState* state = static_cast<DrawableFilterState*>( first.get() );
            while( ok && cursor->hasNext() )
            {
                state->push( cursor->next() );
                ok = state->traverse( env );
                count++;           
            }
            if ( ok )
            {
                ok = state->signalCheckpoint();
            }
        }
        else if ( dynamic_cast<CollectionFilterState*>( first.get() ) )
        {
            CollectionFilterState* state = static_cast<CollectionFilterState*>( first.get() );
            while( ok && cursor->hasNext() )
            {
                state->push( cursor->next() );
                ok = state->traverse( env );
                count++;           
            }
            if ( ok )
            {
                ok = state->signalCheckpoint();
            }
        }

        osg::Timer_t end = osg::Timer::instance()->tick();

        double dur = osg::Timer::instance()->delta_s( start, end );
        //osg::notify( osg::ALWAYS ) << std::endl << "Time = " << dur << " s; Per Feature Avg = " << (dur/(double)count) << " s" << std::endl;
    }

    if ( output_state.valid() )
    {
        osg::NodeList& result = output_state->getOutput();
        output.insert( output.end(), result.begin(), result.end() );
    }

    return ok;
}
