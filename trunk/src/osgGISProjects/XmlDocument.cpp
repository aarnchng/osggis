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

#include <osgGISProjects/XmlDocument>
#include <osg/Notify>
#include <expat.h>
#include <algorithm>

using namespace osgGISProjects;

XmlDocument::XmlDocument( const std::string& _source_uri )
: Document( _source_uri ), XmlElement( "Document" )
{
    //NOP
}

XmlDocument::~XmlDocument()
{
    //NOP
}


static XmlAttributes
getAttributes( const char** attrs )
{
    XmlAttributes map;
    const char** ptr = attrs;
    while( *ptr != NULL )
    {
        std::string name = *ptr++;
        std::string value = *ptr++;
        map[name] = value;
    }
    return map;
}


static void XMLCALL
startElement( void* user_data, const XML_Char* c_tag, const XML_Char** c_attrs )
{
    XmlElementNoRefStack& stack = *(XmlElementNoRefStack*)user_data;
    XmlElement* top = stack.top();

    std::string tag( c_tag );
    std::transform( tag.begin(), tag.end(), tag.begin(), tolower );
    XmlAttributes attrs = getAttributes( c_attrs );

    XmlElement* new_element = new XmlElement( tag, attrs );
    top->getChildren().push_back( new_element );
    stack.push( new_element );
}

static void XMLCALL
endElement( void* user_data, const XML_Char* c_tag )
{
    XmlElementNoRefStack& stack = *(XmlElementNoRefStack*)user_data;
    XmlElement* top = stack.top();
    stack.pop();
} 

static void XMLCALL
handleCharData( void* user_data, const XML_Char* c_data, int len )
{
    if ( len > 0 )
    {
        XmlElementNoRefStack& stack = *(XmlElementNoRefStack*)user_data;
        XmlElement* top = stack.top();
        std::string data( c_data, len );
        top->getChildren().push_back( new XmlText( data ) );
    }
}

XmlDocument*
XmlDocument::load( std::istream& in )
{
    XmlElementNoRefStack tree;

    XmlDocument* doc = new XmlDocument();
    tree.push( doc );

#define BUFSIZE 1024
    char buf[BUFSIZE];
    XML_Parser parser = XML_ParserCreate( NULL );
    bool done = false;
    XML_SetUserData( parser, &tree );
    XML_SetElementHandler( parser, startElement, endElement );
    XML_SetCharacterDataHandler( parser, (XML_CharacterDataHandler)handleCharData );
    while( !in.eof() )
    {
        in.read( buf, BUFSIZE );
        int bytes_read = in.gcount();
        if ( bytes_read > 0 )
        {
            if ( XML_Parse( parser, buf, bytes_read, in.eof() ) == XML_STATUS_ERROR )
            {
                osg::notify( osg::WARN ) 
                    << XML_ErrorString( XML_GetErrorCode( parser ) )
                    << ", "
                    << XML_GetCurrentLineNumber( parser ) 
                    << std::endl;

                XML_ParserFree( parser );
                return NULL;
            }
        }
    }
    XML_ParserFree( parser );
    return doc;
}

#define INDENT 4

static void
storeNode( XmlNode* node, int depth, std::ostream& out )
{
    for( int k=0; k<depth*INDENT; k++ )
        out << " ";

    if ( node->isElement() )
    {
        XmlElement* e = (XmlElement*)node;
        out << "<" << e->getName();
        for( XmlAttributes::iterator a = e->getAttrs().begin(); a != e->getAttrs().end(); a++ )
        {
            out << " " << a->first << "=" << "\"" << a->second << "\"";
        }
        out << ">" << std::endl;
        for( XmlNodeList::iterator i = e->getChildren().begin(); i != e->getChildren().end(); i++ )
        {
            storeNode( i->get(), depth+1, out );
        }
    }
    else if ( node->isText() )
    {
        XmlText* t = (XmlText*)node;
        //out << t->getValue() << std::endl;
    }
}

void
XmlDocument::store( std::ostream& out ) const
{
    out << "<?xml version=\"1.0\"?>" << std::endl;
    for( XmlNodeList::const_iterator i = getChildren().begin(); i != getChildren().end(); i++ )
    {
        storeNode( i->get(), 0, out );
    }
}

