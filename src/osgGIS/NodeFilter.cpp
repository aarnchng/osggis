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
#include <osgGIS/NodeFilterState>
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

FilterState*
NodeFilter::newState()
{
    return new NodeFilterState( this );
}


osg::NodeList 
NodeFilter::process( FeatureList& input, FilterEnv* env )
{
    osg::NodeList output;
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        osg::NodeList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


osg::NodeList 
NodeFilter::process( Feature* input, FilterEnv* env )
{
    osg::NodeList output;
    //NOP
    return output;
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
