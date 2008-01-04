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

#include <osgGIS/FeatureFilterState>
#include <osgGIS/FeatureFilter>
#include <osgGIS/DrawableFilterState>
#include <osgGIS/CollectionFilterState>
#include <osg/Notify>

using namespace osgGIS;


FeatureFilterState::FeatureFilterState( FeatureFilter* _filter )
{
    filter = _filter;
}

void
FeatureFilterState::push( Feature* input )
{
    in_features.push_back( input );
}

void
FeatureFilterState::push( const FeatureList& input )
{
    in_features.insert( in_features.end(), input.begin(), input.end() );
}

bool
FeatureFilterState::traverse( FilterEnv* in_env )
{
    bool ok = true;

    if ( in_features.size() > 0 )
    {
        // clone a new environment:
        osg::ref_ptr<FilterEnv> env = in_env->advance();

        FilterState* next = getNextState();
        if ( next )
        {
            FeatureList output = filter->process( in_features, env.get() );

            if ( dynamic_cast<FeatureFilterState*>( next ) )
            {
                FeatureFilterState* state = static_cast<FeatureFilterState*>( next );
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
    }
    else
    {
        osg::notify( osg::WARN ) << "Traverse called before all inputs were set" << std::endl;
        ok = false;
    }

    // clean up
    in_features.clear();

    return true;
}