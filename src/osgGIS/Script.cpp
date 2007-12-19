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

#include <osgGIS/Script>
#include <osgGIS/CollectionFilterState>
#include <osgGIS/DrawableFilterState>
#include <osgGIS/FeatureFilterState>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>
#include <osg/Timer>

using namespace osgGIS;

Script::Script()
{
}


Script::~Script()
{
}


const std::string&
Script::getName() const
{
    return name;
}


void
Script::setName( const std::string& value )
{
    name = value;
}

const FilterList&
Script::getFilters() const
{
    return filters;
}

FilterList&
Script::getFilters()
{
    return filters;
}


bool 
Script::appendFilter( Filter* filter )
{
    filters.push_back( filter );

    //if ( !first_filter.valid() )
    //{
    //    if (dynamic_cast<FeatureFilter*>( filter ) ||
    //        dynamic_cast<DrawableFilter*>( filter ) ||
    //        dynamic_cast<CollectionFilter*>( filter ) )
    //    {
    //        first_filter = filter;
    //    }
    //    else
    //    {
    //        osg::notify( osg::WARN ) 
    //            << "Illegal; first filter must be a DrawableFilter or a FeatureFilter" 
    //            << std::endl;
    //        return false;
    //    }
    //}
    //else
    //{
    //    if ( !first_filter->appendFilter( filter ) )
    //        return false;
    //}

    //NodeFilter* node_filter = dynamic_cast<NodeFilter*>( filter );
    //if ( node_filter )
    //{
    //    output_filter = node_filter;
    //}

    return true;
}


Filter*
Script::getFilter( const std::string& name )
{
    for( FilterList::iterator i = filters.begin(); i != filters.end(); i++ )
    {
        if ( i->get()->getName() == name )
            return i->get();
    }
    return NULL;
}

    //for( Filter* f = getFirstFilter(); f != NULL; f = f->getNextFilter() )
    //{
    //    if ( f->getName() == name )
    //    {
    //        return f;
    //    }
    //}
    //return NULL;
//}


//void 
//Script::resetFilters( ScriptContext* context )
//{
//    if ( first_filter.get() )
//    {
//        first_filter->reset( context );
//    }
//}


bool
Script::run( FeatureCursor* cursor, FilterEnv* env, osg::NodeList& output )
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

//bool
//Script::run( FeatureCursor* cursor, FilterEnv* env )
//{
//    bool ok = false;
//    if ( first_filter.valid() )
//    {
//        ok = true;
//        int count = 0;
//
//        osg::Timer_t start = osg::Timer::instance()->tick();
//
//        env->setOutputSRS( env->getInputSRS() );
//        
//        if ( dynamic_cast<FeatureFilter*>( first_filter.get() ) )
//        {
//            FeatureFilter* filter = static_cast<FeatureFilter*>( first_filter.get() );
//            while( ok && cursor->hasNext() )
//            {
//                filter->push( cursor->next() );
//                ok = filter->traverse( env );
//                count++;
//            }
//            if ( ok )
//            {
//                ok = filter->signalCheckpoint();
//            }
//        }
//        else if ( dynamic_cast<DrawableFilter*>( first_filter.get() ) )
//        {
//            DrawableFilter* filter = static_cast<DrawableFilter*>( first_filter.get() );
//            while( ok && cursor->hasNext() )
//            {
//                filter->push( cursor->next() );
//                ok = filter->traverse( env );
//                count++;           
//            }
//            if ( ok )
//            {
//                ok = filter->signalCheckpoint();
//            }
//        }
//        else if ( dynamic_cast<CollectionFilter*>( first_filter.get() ) )
//        {
//            CollectionFilter* filter = static_cast<CollectionFilter*>( first_filter.get() );
//            while( ok && cursor->hasNext() )
//            {
//                filter->push( cursor->next() );
//                ok = filter->traverse( env );
//                count++;           
//            }
//            if ( ok )
//            {
//                ok = filter->signalCheckpoint();
//            }
//        }
//
//        osg::Timer_t end = osg::Timer::instance()->tick();
//
//        double dur = osg::Timer::instance()->delta_s( start, end );
//        //osg::notify( osg::ALWAYS ) << std::endl << "Time = " << dur << " s; Per Feature Avg = " << (dur/(double)count) << " s" << std::endl;
//    }
//    return ok;
//}


//osg::NodeList
//Script::getOutput( bool reset )
//{
//    osg::NodeList result = output_filter.valid()?
//        output_filter->getOutput() :
//        osg::NodeList();
//
//    if ( reset )
//    {
//        resetFilters( NULL );
//    }
//
//    return result;
//}


//Filter*
//Script::getFirstFilter()
//{
//    return first_filter.get();
//}