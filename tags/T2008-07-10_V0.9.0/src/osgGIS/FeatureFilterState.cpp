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

#include <osgGIS/FeatureFilterState>
#include <osgGIS/FeatureFilter>
#include <osgGIS/FragmentFilterState>
#include <osgGIS/CollectionFilterState>
//#include <osgGIS/DisperseFilterState>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>

using namespace osgGIS;


FeatureFilterState::FeatureFilterState( FeatureFilter* _filter )
{
    filter = _filter;
}

void
FeatureFilterState::push( Feature* input )
{
    if ( input && input->hasShapeData() )
        in_features.push_back( input );
}

void
FeatureFilterState::push( const FeatureList& input )
{
    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
        if ( i->get()->hasShapeData() )
            in_features.push_back( i->get() );
}

bool
FeatureFilterState::traverse( FilterEnv* in_env )
{
    bool ok = true;

    if ( in_features.size() > 0 )
    {
        // clone a new environment:
        current_env = in_env->advance();
        //osg::ref_ptr<FilterEnv> env = in_env->advance();

        FeatureList output = filter->process( in_features, current_env.get() );
        
        FilterState* next = getNextState();
        if ( next )
        {
            if ( dynamic_cast<FeatureFilterState*>( next ) )
            {
                FeatureFilterState* state = static_cast<FeatureFilterState*>( next );
                state->push( output );
            }
            else if ( dynamic_cast<FragmentFilterState*>( next ) )
            {
                FragmentFilterState* state = static_cast<FragmentFilterState*>( next );
                state->push( output );
            }
            else if ( dynamic_cast<NodeFilterState*>( next ) )
            {
                NodeFilterState* state = static_cast<NodeFilterState*>( next );
                state->push( output );
            }
            else if ( dynamic_cast<CollectionFilterState*>( next ) )
            {
                CollectionFilterState* state = static_cast<CollectionFilterState*>( next );
                state->push( output );
            }

            ok = next->traverse( current_env.get() );
        }
    }
    else
    {
        //osg::notify( osg::WARN ) << "Traverse called before all inputs were set" << std::endl;
        ok = false;
    }

    // clean up
    in_features.clear();

    return true;
}

