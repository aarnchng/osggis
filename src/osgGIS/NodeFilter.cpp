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

#include <osgGIS/NodeFilter>
#include <osgGIS/CollectionFilter>
#include <osg/Notify>
#include <osg/Group>
#include <osg/Geode>

using namespace osgGIS;


NodeFilter::NodeFilter()
{
}


NodeFilter::~NodeFilter()
{
}


void
NodeFilter::reset( ScriptContext* _context )
{
    in_drawables.clear();
    in_nodes.clear();
    out_nodes.clear();
    Filter::reset( _context );
}


void
NodeFilter::push( osg::Drawable* input )
{
    in_drawables.push_back( input );
}


void
NodeFilter::push( DrawableList& input )
{
    in_drawables.insert( in_drawables.end(), input.begin(), input.end() );
}


void
NodeFilter::push( osg::Node* input )
{
    in_nodes.push_back( input );
}


void
NodeFilter::push( osg::NodeList& input )
{
    in_nodes.insert( in_nodes.end(), input.begin(), input.end() );
}


osg::NodeList 
NodeFilter::process( DrawableList& input, FilterEnv* env )
{
    osg::NodeList output;
    for( DrawableList::iterator i = input.begin(); i != input.end(); i++ )
    {
        osg::NodeList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


osg::NodeList 
NodeFilter::process( osg::Drawable* input, FilterEnv* env )
{
    osg::NodeList output;
    osg::Geode* geode = new osg::Geode();
    geode->addDrawable( input );
    output.push_back( geode );
    return output;
}


osg::NodeList 
NodeFilter::process( osg::NodeList& input, FilterEnv* env )
{
    osg::NodeList output;
    for( osg::NodeList::iterator i = input.begin(); i != input.end(); i++ )
    {
        osg::NodeList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


osg::NodeList
NodeFilter::process( osg::Node* input, FilterEnv* env )
{
    osg::NodeList output;
    output.push_back( input );
    return output;
}



bool
NodeFilter::traverse( FilterEnv* in_env )
{
    bool ok = true;

    osg::ref_ptr<FilterEnv> env = in_env->advance();

    if ( in_drawables.size() > 0 )
    {
        out_nodes = process( in_drawables, env.get() );
    }
    else if ( in_nodes.size() > 0 )
    {
        out_nodes = process( in_nodes, env.get() );
    }
    
    Filter* next = getNextFilter();
    if ( next && out_nodes.size() > 0 )
    {
        if ( dynamic_cast<NodeFilter*>( next ) )
        {
            NodeFilter* filter = static_cast<NodeFilter*>( next );
            filter->push( out_nodes );
        }
        else if ( dynamic_cast<CollectionFilter*>( next ) )
        {
            CollectionFilter* filter = static_cast<CollectionFilter*>( next );
            filter->push( out_nodes );
        }

        ok = next->traverse( env.get() );
    }

    in_drawables.clear();
    in_nodes.clear();

    return ok;
}


osg::NodeList&
NodeFilter::getOutput()
{
    return out_nodes;
}

