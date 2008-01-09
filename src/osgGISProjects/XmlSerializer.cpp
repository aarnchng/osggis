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
#include <osgGIS/Utils>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <expat.h>
#include <osg/Notify>
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


static bool
isRef( const std::string& word )
{
    return word.length() > 1 && word.at(0) == '%';
}


static std::string
getRef( const std::string& word )
{
    return word.substr( 1 );
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


XmlElement*
XmlSerializer::encodeFilterGraph( FilterGraph* graph )
{
    XmlElement* graph_e = new XmlElement( "graph" );

    for( FilterList::const_iterator i = graph->getFilters().begin(); i != graph->getFilters().end(); i++ )
    {
        Filter* f = i->get();

        XmlAttributes attrs;
        attrs[ "type" ] = f->getFilterType();
        XmlElement* filter_e = new XmlElement( "filter", attrs );
        
        Properties props = f->getProperties();
        for( Properties::const_iterator i = props.begin(); i != props.end(); i++ )
        {
            const Property& prop = *i;
        }

        graph_e->getChildren().push_back( filter_e );
    }
    return graph_e;
}

FilterGraph*
XmlSerializer::readFilterGraph( Document* doc )
{    
    FilterGraph* result = NULL;
    if ( doc )
    {
        XmlDocument* xml_doc = (XmlDocument*)doc;
        result = decodeFilterGraph( xml_doc->getSubElement( "graph" ), NULL );
    }
    return result;
}


Document*
XmlSerializer::writeFilterGraph( FilterGraph* graph )
{
    XmlDocument* doc = new XmlDocument();

    if ( graph )
    {
        XmlElement* graph_e = encodeFilterGraph( graph );
        doc->getChildren().push_back( graph_e );
    }

    return doc;
}


FilterGraph*
XmlSerializer::decodeFilterGraph( XmlElement* e, Project* proj )
{
    FilterGraph* graph = NULL;
    if ( e )
    {
        std::string name = e->getAttr( "name" );
        //TODO: assert name

        std::string parent_name = e->getAttr( "inherits" );
        if ( parent_name.length() > 0 )
        {
            FilterGraph* parent_graph = proj->getFilterGraph( parent_name );
            if ( !parent_graph )
            {
                osg::notify( osg::WARN ) 
                    << "Parent graph \"" << parent_name << "\" not found for graph \""
                    << name << "\"" << std::endl;
            }
            else
            {
                graph = new FilterGraph( *parent_graph );
                //TODO...
            }
        }
        else
        {
            graph = new FilterGraph();
            graph->setName( name );

            XmlNodeList filter_els = e->getSubElements( "filter" );
            for( XmlNodeList::const_iterator i = filter_els.begin(); i != filter_els.end(); i++ )
            {
                XmlElement* f_e = (XmlElement*)i->get();

                std::string type = f_e->getAttr( "type" );
                Filter* f = osgGIS::Registry::instance()->createFilterByType( type );

                // try again with "Filter" suffix
                if ( !f && !StringUtils::endsWith( type, "Filter", false ) )
                    f = osgGIS::Registry::instance()->createFilterByType( type + "Filter" );

                if ( f )
                {
                    XmlNodeList prop_els = f_e->getSubElements( "property" );
                    for( XmlNodeList::const_iterator k = prop_els.begin(); k != prop_els.end(); k++ )
                    {
                        XmlElement* k_e = (XmlElement*)k->get();
                        std::string name = k_e->getAttr( "name" );
                        std::string value = k_e->getAttr( "value" );
                        f->setProperty( Property( name, value ) );
                    }
                    graph->appendFilter( f );
                }
            }
        }
    }
    return graph;
}

Script*
XmlSerializer::decodeScript( XmlElement* e, Project* proj )
{
    Script* script = NULL;
    if ( e )
    {
        script = new Script( e->getAttr( "name" ), e->getAttr( "language" ), e->getText() );
    }
    return script;
}

Resource*
XmlSerializer::decodeResource( XmlElement* e, Project* proj )
{
    Resource* resource = NULL;
    if ( e )
    {
        std::string type = e->getAttr( "type" );
        resource = osgGIS::Registry::instance()->createResourceByType( type );
        if ( resource )
        {
            XmlNodeList prop_els = e->getSubElements( "property" );
            for( XmlNodeList::const_iterator k = prop_els.begin(); k != prop_els.end(); k++ )
            {
                XmlElement* k_e = (XmlElement*)k->get();
                std::string name = k_e->getAttr( "name" );
                std::string value = k_e->getAttr( "value" );
                resource->setProperty( Property( name, value ) );
            }
        }
    }
    return resource;
}

Source*
XmlSerializer::decodeSource( XmlElement* e, Project* proj )
{
    Source* source = NULL;
    if ( e )
    {
        source = new Source();
        source->setName( e->getAttr( "name" ) );
        source->setURI( e->getSubElementText( "uri" ) );
    }
    return source;
}


Terrain*
XmlSerializer::decodeTerrain( XmlElement* e, Project* proj )
{
    Terrain* terrain = NULL;
    if ( e )
    {
        terrain = new Terrain();
        terrain->setName( e->getAttr( "name" ) );
        terrain->setURI( e->getSubElementText( "uri" ) );
    }
    return terrain;
}


BuildLayerSlice*
XmlSerializer::decodeSlice( XmlElement* e, Project* proj )
{
    BuildLayerSlice* slice = NULL;
    if ( e )
    {
        slice = new BuildLayerSlice();

        slice->setMinRange( atof( e->getAttr( "min_range" ).c_str() ) );
        slice->setMaxRange( atof( e->getAttr( "max_range" ).c_str() ) );

        slice->setMinResolutionLevel( atoi( e->getAttr( "min_level" ).c_str() ) );
        slice->setMaxResolutionLevel( atoi( e->getAttr( "max_level" ).c_str() ) );

        std::string graph = e->getAttr( "graph" );
        slice->setFilterGraph( proj->getFilterGraph( graph ) ); //TODO: warning?
    }
    return slice;
}


BuildLayer*
XmlSerializer::decodeLayer( XmlElement* e, Project* proj )
{
    BuildLayer* layer = NULL;
    if ( e )
    {
        layer = new BuildLayer();
        layer->setName( e->getAttr( "name" ) ); 

        std::string type = e->getAttr( "type" );
        if ( type == "correlated" )
            layer->setType( BuildLayer::TYPE_CORRELATED );
        else if ( type == "gridded" )
            layer->setType( BuildLayer::TYPE_GRIDDED );
        
        std::string source = e->getAttr( "source" );
        layer->setSource( proj->getSource( source ) != NULL?
            proj->getSource( source ) : 
            new Source( source ) );    

        std::string terrain = e->getAttr( "terrain" );
        layer->setTerrain( proj->getTerrain( terrain ) != NULL?
            proj->getTerrain( terrain ) :
            new Terrain( terrain ) );

        layer->setTarget( e->getAttr( "target" ) );

        XmlNodeList slices = e->getSubElements( "slice" );
        for( XmlNodeList::const_iterator i = slices.begin(); i != slices.end(); i++ )
        {
            BuildLayerSlice* slice = decodeSlice( static_cast<XmlElement*>( i->get() ), proj );
            if ( slice )
                layer->getSlices().push_back( slice );
        }

        XmlNodeList props = e->getSubElements( "property" );
        for( XmlNodeList::const_iterator i = props.begin(); i != props.end(); i++ )
        {
            XmlElement* k_e = (XmlElement*)i->get();
            std::string name = k_e->getAttr( "name" );
            std::string value = k_e->getAttr( "value" );
            layer->getProperties().push_back( Property( name, value ) );
        }
    }
    return layer;
}


BuildTarget*
XmlSerializer::decodeTarget( XmlElement* e, Project* proj )
{
    BuildTarget* target = NULL;
    if ( e )
    {
        target = new BuildTarget();
        target->setName( e->getAttr( "name" ) );

        XmlNodeList layers = e->getSubElements( "depends-on-layer" );
        for( XmlNodeList::const_iterator i = layers.begin(); i != layers.end(); i++ )
        {
            XmlElement* e = static_cast<XmlElement*>( i->get() );
            std::string layer_name = e->getText();
            if ( layer_name.length() > 0 )
            {
                BuildLayer* layer = proj->getLayer( layer_name );
                if ( layer )
                    target->addLayer( layer );
            }
        }
    }
    return target;
}


Project* 
XmlSerializer::decodeProject( XmlElement* e )
{
    Project* project = NULL;
    if ( e )
    {
        project = new Project();
        project->setName( e->getAttr( "name" ) );
        
        // scripts
        XmlNodeList scripts = e->getSubElements( "script" );
        for( XmlNodeList::const_iterator j = scripts.begin(); j != scripts.end(); j++ )
        {
            Script* script = decodeScript( static_cast<XmlElement*>( j->get() ), project );
            if ( script )
                project->getScripts().push_back( script );
        }

        // graphs
        XmlNodeList graphs = e->getSubElements( "graph" );
        for( XmlNodeList::const_iterator j = graphs.begin(); j != graphs.end(); j++ )
        {
            FilterGraph* graph = decodeFilterGraph( static_cast<XmlElement*>( j->get() ), project );
            if ( graph )
                project->getFilterGraphs().push_back( graph );
        }

        // terrains
        XmlNodeList terrains = e->getSubElements( "terrain" );
        for( XmlNodeList::const_iterator j = terrains.begin(); j != terrains.end(); j++ )
        {
            Terrain* terrain = decodeTerrain( static_cast<XmlElement*>( j->get() ), project );
            if ( terrain )
                project->getTerrains().push_back( terrain );
        }

        // sources
        XmlNodeList sources = e->getSubElements( "source" );
        for( XmlNodeList::const_iterator j = sources.begin(); j != sources.end(); j++ )
        {
            Source* source = decodeSource( static_cast<XmlElement*>( j->get() ), project );
            if ( source )
                project->getSources().push_back( source );
        }

        // layers
        XmlNodeList layers = e->getSubElements( "layer" );
        for( XmlNodeList::const_iterator j = layers.begin(); j != layers.end(); j++ )
        {
            BuildLayer* layer = decodeLayer( static_cast<XmlElement*>( j->get() ), project );
            if ( layer )
            {
                project->getLayers().push_back( layer );

                // automatically add a target for this layer alone:
                BuildTarget* layer_target = new BuildTarget();
                layer_target->setName( layer->getName() );
                layer_target->addLayer( layer );
                project->getTargets().push_back( layer_target );
            }
        }

        // targets
        XmlNodeList targets = e->getSubElements( "target" );
        for( XmlNodeList::const_iterator j = targets.begin(); j != targets.end(); j++ )
        {
            BuildTarget* target = decodeTarget( static_cast<XmlElement*>( j->get() ), project );
            if ( target )
                project->getTargets().push_back( target );
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
        result = decodeProject( xml_doc->getSubElement( "project" ) );
    }
    return result;
}


Document*
XmlSerializer::writeProject( Project* project )
{
    //TODO
    return new XmlDocument();
}

