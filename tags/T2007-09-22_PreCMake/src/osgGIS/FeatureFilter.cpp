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

#include <osgGIS/FeatureFilter>
#include <osgGIS/DrawableFilter>
#include <osgGIS/CollectionFilter>
#include <osg/notify>

using namespace osgGIS;


FeatureFilter::FeatureFilter()
{
}


FeatureFilter::~FeatureFilter()
{
}


void
FeatureFilter::push( Feature* input )
{
    in_features.push_back( input );
}


void
FeatureFilter::push( FeatureList& input )
{
    in_features.insert( in_features.end(), input.begin(), input.end() );
}


void
FeatureFilter::reset( ScriptContext* context )
{
    in_features.clear();
    Filter::reset( context );
}


FeatureList
FeatureFilter::process( FeatureList& input, FilterEnv* env )
{
    FeatureList output;
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        FeatureList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


FeatureList
FeatureFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;
    output.push_back( input );
    return output;
}


bool
FeatureFilter::traverse( FilterEnv* in_env )
{
    bool ok = true;

    if ( in_features.size() > 0 )
    {
        // clone a new environment:
        osg::ref_ptr<FilterEnv> env = in_env->advance();

        Filter* next = getNextFilter();
        if ( next )
        {
            if ( dynamic_cast<FeatureFilter*>( next ) )
            {
                FeatureFilter* filter = static_cast<FeatureFilter*>( next );
                filter->push( process( in_features, env.get() ) );
            }
            else if ( dynamic_cast<DrawableFilter*>( next ) )
            {
                DrawableFilter* filter = static_cast<DrawableFilter*>( next );
                filter->push( process( in_features, env.get() ) );
            }
            else if ( dynamic_cast<CollectionFilter*>( next ) )
            {
                CollectionFilter* filter = static_cast<CollectionFilter*>( next );
                filter->push( process( in_features, env.get() ) );
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