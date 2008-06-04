/**
 * osgGIS - GIS Library for OpenSceneGraph
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

#include <osgGISProjects/XmlDOM>
#include <osgGIS/Utils>
#include <osg/Notify>
#include <expat.h>
#include <algorithm>
#include <sstream>

using namespace osgGISProjects;

static std::string EMPTY_VALUE = "";

XmlNode::XmlNode()
{
    //NOP
}

XmlElement::XmlElement( const std::string& _name )
{
    name = _name;
}

XmlElement::XmlElement( const std::string& _name, const XmlAttributes& _attrs )
{
    name = _name;
    attrs = _attrs;
}

const std::string&
XmlElement::getName() const
{
    return name;
}

XmlAttributes&
XmlElement::getAttrs()
{
    return attrs;
}

const XmlAttributes&
XmlElement::getAttrs() const
{
    return attrs;
}

const std::string&
XmlElement::getAttr( const std::string& key ) const
{
    XmlAttributes::const_iterator i = attrs.find( key );
    return i != attrs.end()? i->second : EMPTY_VALUE;
}

XmlNodeList&
XmlElement::getChildren()
{
    return children;
}

const XmlNodeList&
XmlElement::getChildren() const
{
    return children;
}
        
XmlElement*
XmlElement::getSubElement( const std::string& name ) const
{
    std::string name_lower = name;
    std::transform( name_lower.begin(), name_lower.end(), name_lower.begin(), tolower );

    for( XmlNodeList::const_iterator i = getChildren().begin(); i != getChildren().end(); i++ )
    {
        if ( i->get()->isElement() )
        {
            XmlElement* e = (XmlElement*)i->get();
            std::string name = e->getName();
            std::transform( name.begin(), name.end(), name.begin(), tolower );
            if ( name == name_lower )
                return e;
        }
    }
    return NULL;
}


std::string
XmlElement::getText() const
{
    std::stringstream builder;

    for( XmlNodeList::const_iterator i = getChildren().begin(); i != getChildren().end(); i++ )
    {
        if ( i->get()->isText() )
        {
            builder << ( static_cast<XmlText*>( i->get() ) )->getValue();
        }
    }

    std::string result = osgGIS::StringUtils::trim( builder.str() );
    return result;
}


std::string
XmlElement::getSubElementText( const std::string& name ) const
{
    XmlElement* e = getSubElement( name );
    return e? e->getText() : EMPTY_VALUE;
}


XmlNodeList 
XmlElement::getSubElements( const std::string& name ) const
{
    XmlNodeList results;

    std::string name_lower = name;
    std::transform( name_lower.begin(), name_lower.end(), name_lower.begin(), tolower );

    for( XmlNodeList::const_iterator i = getChildren().begin(); i != getChildren().end(); i++ )
    {
        if ( i->get()->isElement() )
        {
            XmlElement* e = (XmlElement*)i->get();
            std::string name = e->getName();
            std::transform( name.begin(), name.end(), name.begin(), tolower );
            if ( name == name_lower )
                results.push_back( e );
        }
    }

    return results;
}


XmlText::XmlText( const std::string& _value )
{
    value = _value;
}

const std::string&
XmlText::getValue() const
{
    return value;
}

