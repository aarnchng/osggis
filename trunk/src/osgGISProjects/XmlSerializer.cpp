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

#include <osgGISProjects/XmlSerializer>
#include <osgGISProjects/XmlDOM>
#include <osgGISProjects/XmlDocument>
#include <osgGIS/Registry>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <expat.h>
#include <osg/notify>
#include <string>
#include <algorithm>
#include <fstream>

using namespace osgGISProjects;
using namespace osgGIS;

XmlSerializer::XmlSerializer()
{
}


XmlSerializer::~XmlSerializer()
{
}


Document*
XmlSerializer::load( std::istream& in )
{
    return XmlDocument::load( in );
}


Document*
XmlSerializer::load( const std::string& uri )
{
    Document* doc = NULL;
    if ( osgDB::fileExists( uri ) )
    {
        std::fstream ins;
        ins.open( uri.c_str() );
        if ( ins.is_open() )
        {
            doc = load( ins );
            ins.close();
        }
    }
    return doc;
}


void
XmlSerializer::store( Document* doc, std::ostream& out )
{
    //TODO
    osg::notify( osg::FATAL ) << "XmlSerializer::store() is NYI" << std::endl;
}


Script*
XmlSerializer::decodeScript( XmlElement* e )
{
    Script* script = NULL;
    if ( e )
    {
        script = new Script();
        XmlNodeList filter_els = e->getElements( "filter" );
        for( XmlNodeList::const_iterator i = filter_els.begin(); i != filter_els.end(); i++ )
        {
            XmlElement* f_e = (XmlElement*)i->get();
            std::string type = f_e->getAttr( "type" );
            Filter* f = osgGIS::Registry::instance()->createFilterByType( type );
            if ( f )
            {
                XmlNodeList prop_els = f_e->getElements( "property" );
                for( XmlNodeList::const_iterator k = prop_els.begin(); k != prop_els.end(); k++ )
                {
                    XmlElement* k_e = (XmlElement*)k->get();
                    std::string name = k_e->getAttr( "name" );
                    std::string value = k_e->getAttr( "value" );
                    f->setProperty( Property( name, value ) );
                }
                script->appendFilter( f );
            }
        }
    }
    return script;
}


XmlElement*
XmlSerializer::encodeScript( Script* script )
{
    XmlElement* script_e = new XmlElement( "script" );
    for( Filter* f = script->getFirstFilter(); f != NULL; f = f->getNextFilter() )
    {
        XmlAttributes attrs;
        attrs[ "type" ] = f->getFilterType();
        XmlElement* filter_e = new XmlElement( "filter", attrs );
        
        Properties props = f->getProperties();
        for( Properties::const_iterator i = props.begin(); i != props.end(); i++ )
        {
            const Property& prop = *i;
        }

        script_e->getChildren().push_back( filter_e );
    }
    return script_e;
}


Script*
XmlSerializer::readScript( Document* doc )
{    
    Script* result = NULL;
    if ( doc )
    {
        XmlDocument* xml_doc = (XmlDocument*)doc;
        result = decodeScript( xml_doc->getElement( "script" ) );
    }
    return result;
}


Document*
XmlSerializer::writeScript( Script* script )
{
    XmlDocument* doc = new XmlDocument();

    if ( script )
    {
        XmlElement* script_el = encodeScript( script );
        doc->getChildren().push_back( script_el );
    }

    return doc;
}


Source*
XmlSerializer::decodeSource( XmlElement* e )
{
    Source* source = NULL;
    if ( e )
    {
        source = new Source();
        source->setName( e->getAttr( "name" ) );
        source->setURI( e->getElementText( "uri" ) );
    }
    return source;
}


Build*
XmlSerializer::decodeBuild( XmlElement* e )
{
    Build* build = NULL;
    if ( e )
    {
        build = new Build();
        build->setName( e->getAttr( "name" ) );
    }
    return build;
}


Project* 
XmlSerializer::decodeProject( XmlElement* e )
{
    Project* project = NULL;
    if ( e )
    {
        project = new Project();

        // scripts
        XmlNodeList scripts_nodes = e->getElements( "scripts" );
        for( XmlNodeList::const_iterator i = scripts_nodes.begin(); i != scripts_nodes.end(); i++ )
        {
            XmlElement* scripts_root = static_cast<XmlElement*>( i->get() );
            XmlNodeList script_nodes = scripts_root->getElements( "script" );
            for( XmlNodeList::const_iterator j = script_nodes.begin(); j != script_nodes.end(); j++ )
            {
                Script* script = decodeScript( static_cast<XmlElement*>( j->get() ) );
                if ( script )
                    project->getScripts().push_back( script );
            }
        }

        // sources
        XmlNodeList sources_nodes = e->getElements( "sources" );
        for( XmlNodeList::const_iterator i = sources_nodes.begin(); i != sources_nodes.end(); i++ )
        {
            XmlElement* sources_root = static_cast<XmlElement*>( i->get() );
            XmlNodeList source_nodes = sources_root->getElements( "source" );
            for( XmlNodeList::const_iterator j = source_nodes.begin(); j != source_nodes.end(); j++ )
            {
                Source* source = decodeSource( static_cast<XmlElement*>( j->get() ) );
                if ( source )
                    project->getSources().push_back( source );
            }
        }

        // builds
        XmlNodeList builds_nodes = e->getElements( "builds" );
        for( XmlNodeList::const_iterator i = builds_nodes.begin(); i != builds_nodes.end(); i++ )
        {
            XmlElement* builds_root = static_cast<XmlElement*>( i->get() );
            XmlNodeList build_nodes = builds_root->getElements( "build" );
            for( XmlNodeList::const_iterator j = build_nodes.begin(); j != build_nodes.end(); j++ )
            {
                Build* build = decodeBuild( static_cast<XmlElement*>( j->get() ) );
                if ( build )
                    project->getBuilds().push_back( build );
            }
        }
    }
    return project;
}


Project*
XmlSerializer::readProject( Document* doc )
{
    Project* result = NULL;
    if ( doc )
    {
        XmlDocument* xml_doc = (XmlDocument*)doc;
        result = decodeProject( xml_doc->getElement( "project" ) );
    }
    return result;
}


Document*
XmlSerializer::writeProject( Project* project )
{
    //TODO
    return new XmlDocument();
}

