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

#include <osgGIS/NodeFilter>
#include <osgGIS/NodeFilterState>
#include <osg/Notify>
#include <osg/Group>
#include <osg/Geode>
#include <osgSim/ShapeAttribute>

using namespace osgGIS;


NodeFilter::NodeFilter()
{
    //NOP
}

NodeFilter::NodeFilter( const NodeFilter& rhs )
: Filter( rhs )
{
    //NOP
}

NodeFilter::~NodeFilter()
{
    //NOP
}

FilterState*
NodeFilter::newState() const
{
    return new NodeFilterState( static_cast<NodeFilter*>( clone() ) );
}


AttributedNodeList 
NodeFilter::process( FeatureList& input, FilterEnv* env )
{
    AttributedNodeList output;
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        AttributedNodeList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


AttributedNodeList 
NodeFilter::process( Feature* input, FilterEnv* env )
{
    AttributedNodeList output;
    //NOP - never called
    return output;
}


AttributedNodeList 
NodeFilter::process( FragmentList& input, FilterEnv* env )
{
    AttributedNodeList output;
    for( FragmentList::iterator i = input.begin(); i != input.end(); i++ )
    {
        AttributedNodeList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


AttributedNodeList 
NodeFilter::process( Fragment* input, FilterEnv* env )
{
    AttributedNodeList output;
    osg::Geode* geode = new osg::Geode();
    for( DrawableList::const_iterator i = input->getDrawables().begin(); i != input->getDrawables().end(); i++ )
        geode->addDrawable( i->get() );
    output.push_back( new AttributedNode( geode ) );
    return output;
}


AttributedNodeList 
NodeFilter::process( AttributedNodeList& input, FilterEnv* env )
{
    AttributedNodeList output;
    for( AttributedNodeList::iterator i = input.begin(); i != input.end(); i++ )
    {
        AttributedNodeList interim = process( i->get(), env );
        output.insert( output.end(), interim.begin(), interim.end() );
    }
    return output;
}


AttributedNodeList
NodeFilter::process( AttributedNode* input, FilterEnv* env )
{
    AttributedNodeList output;
    output.push_back( input );
    return output;
}


void
NodeFilter::embedAttributes( osg::Node* node, const AttributeList& attrs )
{
    osgSim::ShapeAttributeList* to_embed = new osgSim::ShapeAttributeList();

    for( AttributeList::const_iterator a = attrs.begin(); a != attrs.end(); a++ )
    {
        switch( a->getType() )
        {
        case Attribute::TYPE_INT:
        case Attribute::TYPE_BOOL:
            to_embed->push_back( osgSim::ShapeAttribute( a->getKey().c_str(), a->asInt() ) );
            break;
        case Attribute::TYPE_DOUBLE:
            to_embed->push_back( osgSim::ShapeAttribute( a->getKey().c_str(), a->asDouble() ) );
            break;
        case Attribute::TYPE_STRING:
            to_embed->push_back( osgSim::ShapeAttribute( a->getKey().c_str(), a->asString() ) );
        }
    }

    node->setUserData( to_embed );
}