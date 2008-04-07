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
#include <osgGIS/FeatureFilterState>
#include <osgGIS/DrawableFilterState>
#include <osgGIS/NodeFilterState>
#include <osgGIS/CollectionFilterState>
#include <osgGIS/WriteFeaturesFilter>
#include <osgGIS/Registry>
#include <osg/Notify>
#include <osg/Timer>

using namespace osgGIS;

FilterGraphResult
FilterGraphResult::ok()
{
    return FilterGraphResult( true );
}

FilterGraphResult
FilterGraphResult::error()
{
    return FilterGraphResult( false );
}

FilterGraphResult::FilterGraphResult( bool _is_ok )
{
    is_ok = _is_ok;
}

bool
FilterGraphResult::isOK() const
{
    return is_ok;
}


/*****************************************************************************/

FilterGraph::FilterGraph()
{
    //NOP
}

FilterGraph::~FilterGraph()
{
    //NOP
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


FilterGraphResult
FilterGraph::computeFeatureStore(FeatureCursor*     cursor,
                                 FilterEnv*         env,
                                 const std::string& output_uri )
{
    bool ok = false;

    // first build the filter state chain, validating that there are ONLY feature filters
    // present. No other filter type is permitted when generating a feature store.
    osg::ref_ptr<FilterState> first = NULL;
    for( FilterList::iterator i = filters.begin(); i != filters.end(); i++ )
    {
        Filter* filter = i->get();

        if ( !dynamic_cast<FeatureFilter*>( filter ) )
        {
            osg::notify(osg::WARN) << "Error: illegal filter of type \"" << filter->getFilterType() << "\" in graph. Only feature features are allowed." << std::endl;
            return FilterGraphResult::error();
        }

        FilterState* next_state = filter->newState();
        if ( !first.valid() )
        {
            first = next_state;
        }
        else
        {
            first->appendState( next_state );
        }
    }

    if ( !first.valid() )
    {
        osg::notify(osg::WARN) << "Error: filter graph \"" << getName() << "\" is empty." << std::endl;
        return FilterGraphResult::error();
    }

    // next, append a WriteFeatures filter that will generate the output
    // feature store.
    WriteFeaturesFilter* writer = new WriteFeaturesFilter();
    writer->setOutputURI( output_uri );
    //writer->setAppendMode( WriteFeaturesFilter::OVERWRITE );

    first->appendState( writer->newState() );

    // now run the graph.
    ok = true;
    int count = 0;
    osg::Timer_t start = osg::Timer::instance()->tick();
    
    env->setOutputSRS( env->getInputSRS() );

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

    osg::Timer_t end = osg::Timer::instance()->tick();
    double dur = osg::Timer::instance()->delta_s( start, end );

    //writer->finalize();

    return ok? FilterGraphResult::ok() : FilterGraphResult::error();
}

FilterGraphResult
FilterGraph::computeNodes( FeatureCursor* cursor, FilterEnv* env, osg::NodeList& output )
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

    return ok? FilterGraphResult::ok() : FilterGraphResult::error();
}
