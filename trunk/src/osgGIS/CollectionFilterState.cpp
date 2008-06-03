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
#include <osgGIS/FragmentFilterState>
#include <osgGIS/FeatureFilterState>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>

using namespace osgGIS;

        
typedef std::map<std::string,FeatureList>   FeatureGroups;
typedef std::map<std::string,FragmentList>  FragmentGroups;
typedef std::map<std::string,osg::NodeList> NodeGroups;


CollectionFilterState::CollectionFilterState( CollectionFilter* _filter )
{
    filter = _filter;
}

void 
CollectionFilterState::push( const FeatureList& input )
{
    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
        if ( i->get()->hasShapeData() )
            features.push_back( i->get() );
}


void
CollectionFilterState::push( Feature* input )
{
    if ( input && input->hasShapeData() )
        features.push_back( input );
}


void 
CollectionFilterState::push( const FragmentList& input )
{
    fragments.insert( fragments.end(), input.begin(), input.end() );
}


void
CollectionFilterState::push( Fragment* input )
{
    fragments.push_back( input );
}


void 
CollectionFilterState::push( const osg::NodeList& input )
{
    nodes.insert( nodes.end(), input.begin(), input.end() );
}


void
CollectionFilterState::push( osg::Node* input )
{
    nodes.push_back( input );
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
meterGroups( CollectionFilter* filter, A source, B state, unsigned int metering, FilterEnv* env )
{
    bool ok = true;
    for( typename A::iterator i = source.begin(); i != source.end() && ok; i++ )
    {
        filter->preMeter( i->second, env );
        ok = meterData( i->second, state, metering, env );
    }
    return ok;
}


bool 
CollectionFilterState::signalCheckpoint()
{
    bool ok = true;
    bool has_data = false;

    FilterState* next = getNextState();
    if ( next )
    {
        int metering = filter->getMetering();

        if ( dynamic_cast<FeatureFilterState*>( next ) )
        {
            if ( !features.empty() )
            {
                FeatureGroups feature_groups;
                for( FeatureList::const_iterator i = features.begin(); i != features.end(); i++ )
                    feature_groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );

                FeatureFilterState* state = static_cast<FeatureFilterState*>( next );
                ok = meterGroups( filter.get(), feature_groups, state, metering, saved_env.get() );
            }
            else
            {
                ok = false;
            }
        }
        else if ( dynamic_cast<FragmentFilterState*>( next ) )
        {
            FragmentFilterState* state = static_cast<FragmentFilterState*>( next );
            if ( !features.empty() )
            {
                FeatureGroups groups;
                for( FeatureList::const_iterator i = features.begin(); i != features.end(); i++ )
                    groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
            }
            else if ( !fragments.empty() )
            {
                FragmentGroups groups;
                for( FragmentList::const_iterator i = fragments.begin(); i != fragments.end(); i++ )
                    groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
            }
            else
            {
                ok = false;
            }
        }
        else if ( dynamic_cast<NodeFilterState*>( next ) )
        {
            NodeFilterState* state = static_cast<NodeFilterState*>( next ); 
            if ( !features.empty() )
            {
                FeatureGroups feature_groups;
                for( FeatureList::const_iterator i = features.begin(); i != features.end(); i++ )
                    feature_groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), feature_groups, state, metering, saved_env.get() );
            }
            else if ( !fragments.empty() )
            {
                FragmentGroups groups;
                for( FragmentList::const_iterator i = fragments.begin(); i != fragments.end(); i++ )
                    groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
            }
            else if ( !nodes.empty() )
            {
                NodeGroups groups;
                for( osg::NodeList::const_iterator i = nodes.begin(); i != nodes.end(); i++ )
                    groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
            }
            else
            {
                ok = false;
            }
        }
        else if ( dynamic_cast<CollectionFilterState*>( next ) )
        {
            CollectionFilterState* state = static_cast<CollectionFilterState*>( next );   
            if ( !features.empty() )
            {
                FeatureGroups feature_groups;
                for( FeatureList::const_iterator i = features.begin(); i != features.end(); i++ )
                    feature_groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), feature_groups, state, metering, saved_env.get() );
            }
            else if ( !fragments.empty() )
            {
                FragmentGroups groups;
                for( FragmentList::const_iterator i = fragments.begin(); i != fragments.end(); i++ )
                    groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
            }
            else if ( !nodes.empty() )
            {
                NodeGroups groups;
                for( osg::NodeList::const_iterator i = nodes.begin(); i != nodes.end(); i++ )
                    groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
                ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
            }
            else
            {
                ok = false;         
            }
        }
        //else if ( dynamic_cast<DisperseFilterState*>( next ) )
        //{
        //    DisperseFilterState* state = static_cast<DisperseFilterState*>( next );   
        //    if ( !features.empty() )
        //    {
        //        FeatureGroups feature_groups;
        //        for( FeatureList::const_iterator i = features.begin(); i != features.end(); i++ )
        //            feature_groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
        //        ok = meterGroups( filter.get(), feature_groups, state, metering, saved_env.get() );
        //    }
        //    else if ( !fragments.empty() )
        //    {
        //        FragmentGroups groups;
        //        for( FragmentList::const_iterator i = fragments.begin(); i != fragments.end(); i++ )
        //            groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
        //        ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
        //    }
        //    else if ( !nodes.empty() )
        //    {
        //        NodeGroups groups;
        //        for( osg::NodeList::const_iterator i = nodes.begin(); i != nodes.end(); i++ )
        //            groups[ filter->assign( i->get(), saved_env.get() ) ].push_back( i->get() );
        //        ok = meterGroups( filter.get(), groups, state, metering, saved_env.get() );
        //    }
        //    else
        //    {
        //        ok = false;         
        //    }
        // }

        if ( ok )
        {
            ok = next->signalCheckpoint();
        }
    }

    // clean up the input:
    features.clear();
    fragments.clear();
    nodes.clear();
    saved_env = NULL;

    return ok;
}

