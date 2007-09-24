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

#include <osgGIS/DrawableFilter>
#include <osgGIS/NodeFilter>
#include <osgGIS/CollectionFilter>
#include <osg/Notify>

using namespace osgGIS;

DrawableFilter::DrawableFilter()
{
}


DrawableFilter::~DrawableFilter()
{
}


void
DrawableFilter::push( Feature* input )
{
    in_features.push_back( input );
}

void
DrawableFilter::push( const FeatureList& input )
{
    in_features.insert( in_features.end(), input.begin(), input.end() );
}

void
DrawableFilter::push( osg::Drawable* input )
{
    in_drawables.push_back( input );
}

void
DrawableFilter::push( const DrawableList& input )
{
    in_drawables.insert( in_drawables.end(), input.begin(), input.end() );
}

void
DrawableFilter::reset( ScriptContext* context )
{
    in_features.clear();
    in_drawables.clear();
    Filter::reset( context );
}


DrawableList
DrawableFilter::process( Feature* f, FilterEnv* env )
{
    return DrawableList();
}


DrawableList
DrawableFilter::process( FeatureList& input, FilterEnv* env )
{
    DrawableList output;
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        DrawableList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


DrawableList
DrawableFilter::process( osg::Drawable* drawable, FilterEnv* env )
{
    DrawableList output;
    output.push_back( drawable );
    return output;
}


DrawableList 
DrawableFilter::process( DrawableList& input, FilterEnv* env )
{
    DrawableList output;
    for( DrawableList::iterator i = input.begin(); i != input.end(); i++ )
    {
        DrawableList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


bool
DrawableFilter::traverse( FilterEnv* in_env )
{
    bool ok = true;

    osg::ref_ptr<FilterEnv> env = in_env->advance();

    Filter* next = getNextFilter();
    if ( next )
    {
        DrawableList output =
            in_features.size() > 0? process( in_features, env.get() ) :
            in_drawables.size() > 0? process( in_drawables, env.get() ) :
            DrawableList();
        
        if ( dynamic_cast<NodeFilter*>( next ) )
        {
            NodeFilter* filter = static_cast<NodeFilter*>( next );
            filter->push( output );
        }
        else if ( dynamic_cast<DrawableFilter*>( next ) )
        {
            DrawableFilter* filter = static_cast<DrawableFilter*>( next );
            filter->push( output );
        }
        else if ( dynamic_cast<CollectionFilter*>( next ) )
        {
            CollectionFilter* filter = static_cast<CollectionFilter*>( next );
            filter->push( output );
        }

        ok = next->traverse( env.get() );
    }

    in_features.clear();
    in_drawables.clear();
    //in_feature = NULL;
    //in_drawable = NULL;

    return ok;
}