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

#include <osgGISProjects/XmlSerializer>
#include <osgGISProjects/XmlDOM>
#include <osgGISProjects/XmlDocument>
#include <osgGIS/Registry>
#include <osgGIS/Utils>
#include <osgGIS/SRSResource>
#include <osgGIS/Tags>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <expat.h>
#include <osg/Notify>
#include <string>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

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


//static
Project*
XmlSerializer::loadProject( const std::string& uri )
{
    XmlSerializer ser;
    osg::ref_ptr<Document> doc = ser.load( uri );    
    if ( !doc.valid() )
        return NULL; // todo: report error

    Project* project = ser.readProject( doc.get() );
    return project;
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
            if ( doc )
                doc->setSourceURI( uri );
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

static RuntimeMapLayer*
decodeRuntimeMapLayer( XmlElement* e, Project* proj )
{
    RuntimeMapLayer* layer = NULL;
    if ( e )
    {
        layer = new RuntimeMapLayer();
        layer->setBuildLayer( proj->getLayer( e->getAttr( "layer" ) ) );
        layer->setSearchLayer( proj->getLayer( e->getAttr( "searchlayer" ) ) );
        if ( e->getAttr( "searchable" ) == "true" )
            layer->setSearchable( true );
        if ( e->getAttr( "visible" ) == "false" )
            layer->setVisible( false );
    }
    return layer;
}

static RuntimeMap*
decodeRuntimeMap( XmlElement* e, Project* proj )
{
    RuntimeMap* map = NULL;
    if ( e )
    {
        map = new RuntimeMap();
        map->setName( e->getAttr( "name" ) );
        map->setTerrain( proj->getTerrain( e->getAttr( "terrain" ) ) );
        
        XmlNodeList map_layers = e->getSubElements( "maplayer" );
        for( XmlNodeList::const_iterator i = map_layers.begin(); i != map_layers.end(); i++ )
        {
            XmlElement* e2 = (XmlElement*)i->get();
            RuntimeMapLayer* map_layer = decodeRuntimeMapLayer( e2, proj );
            if ( map_layer )
                map->getMapLayers().push_back( map_layer );
        }
    }
    return map;
}

static FilterGraph*
decodeFilterGraph( XmlElement* e, Project* proj )
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

static Script*
decodeScript( XmlElement* e, Project* proj )
{
    Script* script = NULL;
    if ( e )
    {
        script = new Script( e->getAttr( "name" ), e->getAttr( "language" ), e->getText() );
    }
    return script;
}

static void
parseSRSResource( XmlElement* e, SRSResource* resource )
{
    if ( !resource->getSRS() )
    {
        std::string wkt = e->getText();
        if ( wkt.length() > 0 )
        {
            resource->setSRS( Registry::SRSFactory()->createSRSfromWKT( wkt ) );
        }
    }
}

static SRSResource*
findSRSResource( const ResourceList& list, const std::string& name )
{
    for( ResourceList::const_iterator i = list.begin(); i != list.end(); i++ )
    {
        if ( dynamic_cast<SRSResource*>( i->get() ) && i->get()->getName() == name )
            return static_cast<SRSResource*>( i->get() );
    }
    return NULL;
}

static void
parseRasterResource( XmlElement* e, RasterResource* resource )
{
    XmlNodeList parts = e->getSubElements( "uri" );
    for( XmlNodeList::const_iterator i = parts.begin(); i != parts.end(); i++ )
    {
        resource->addPartURI( static_cast<XmlElement*>( i->get() )->getText() );
    }
}

static Resource*
decodeResource( XmlElement* e, Project* proj )
{
    Resource* resource = NULL;
    if ( e )
    {
        std::string type = e->getAttr( "type" );
        resource = osgGIS::Registry::instance()->createResourceByType( type );
        
        // try again with "Resource" suffix
        if ( !resource && !StringUtils::endsWith( type, "Resource", false ) )
            resource = osgGIS::Registry::instance()->createResourceByType( type + "Resource" );

        if ( resource )
        {
            resource->setBaseURI( proj->getBaseURI() );

            resource->setName( e->getAttr( "name" ) );

            std::string csv_tags = e->getAttr( "tags" );
            if ( csv_tags.length() > 0 )
            {
                std::istringstream iss( csv_tags );
                std::vector<std::string> tokens( (std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>() );
                for( std::vector<std::string>::const_iterator i = tokens.begin(); i != tokens.end(); i++ )
                    resource->addTag( *i );
            }

            resource->addTag( e->getAttr( "tags" ) );
            resource->setURI( e->getSubElementText( "uri" ) );

            XmlNodeList prop_els = e->getSubElements( "property" );
            for( XmlNodeList::const_iterator k = prop_els.begin(); k != prop_els.end(); k++ )
            {
                XmlElement* k_e = (XmlElement*)k->get();
                std::string name = k_e->getAttr( "name" );
                std::string value = k_e->getAttr( "value" );
                resource->setProperty( Property( name, value ) );
            }

            if ( dynamic_cast<SRSResource*>( resource ) )
            {
                parseSRSResource( e, static_cast<SRSResource*>( resource ) );
            }
            else if ( dynamic_cast<RasterResource*>( resource ) )
            {
                parseRasterResource( e, static_cast<RasterResource*>( resource ) );
            }
        }
        else
        {
            osg::notify( osg::WARN ) << "Unknown resource type: " << type << std::endl;
        }
    }
    return resource;
}

static Source*
decodeSource( XmlElement* e, Project* proj, int pass )
{
    Source* source = NULL;
    if ( e )
    {
        if ( pass == 0 )
        {
            // first pass: create the new source record
            source = new Source();
            source->setBaseURI( proj->getBaseURI() );
            source->setName( e->getAttr( "name" ) );
            source->setType( e->getAttr( "type" ) == "raster"? Source::TYPE_RASTER : Source::TYPE_FEATURE );
            source->setURI( e->getSubElementText( "uri" ) );
            source->setFilterGraph( proj->getFilterGraph( e->getAttr( "graph" ) ) );
        }
        else
        {
            // second pass: reference other sources
            source = proj->getSource( e->getAttr( "name" ) );
            source->setParentSource( proj->getSource( e->getAttr( "parent" ) ) );
        }
    }
    return source;
}


static Terrain*
decodeTerrain( XmlElement* e, Project* proj )
{
    Terrain* terrain = NULL;
    if ( e )
    {
        terrain = new Terrain();
        terrain->setBaseURI( proj->getBaseURI() );
        terrain->setName( e->getAttr( "name" ) );
        terrain->setURI( e->getSubElementText( "uri" ) );

        SRSResource* resource = findSRSResource( proj->getResources(), e->getAttr( "srs" ) );
        if ( resource )
            terrain->setExplicitSRS( resource->getSRS() );
    }
    return terrain;
}


static BuildLayerSlice*
decodeSlice( XmlElement* e, Project* proj )
{
    BuildLayerSlice* slice = NULL;
    if ( e )
    {
        slice = new BuildLayerSlice();

        if ( e->getAttr( "min_range" ).length() > 0 )
            slice->setMinRange( atof( e->getAttr( "min_range" ).c_str() ) );
        if ( e->getAttr( "max_range" ).length() > 0 )
            slice->setMaxRange( atof( e->getAttr( "max_range" ).c_str() ) );

        //if ( e->getAttr( "min_level" ).length() > 0 )
        //    slice->setMinResolutionLevel( atoi( e->getAttr( "min_level" ).c_str() ) );
        //if ( e->getAttr( "max_level" ).length() > 0 )
        //    slice->setMaxResolutionLevel( atoi( e->getAttr( "max_level" ).c_str() ) );

        // required filter graph:
        std::string graph = e->getAttr( "graph" );
        slice->setFilterGraph( proj->getFilterGraph( graph ) ); //TODO: warning?
       
        // optional source:
        slice->setSource( proj->getSource( e->getAttr( "source" ) ) );
    }
    return slice;
}


static BuildLayer*
decodeLayer( XmlElement* e, Project* proj )
{
    BuildLayer* layer = NULL;
    if ( e )
    {
        layer = new BuildLayer();
        layer->setBaseURI( proj->getBaseURI() );
        layer->setName( e->getAttr( "name" ) ); 

        std::string type = e->getAttr( "type" );
        if ( type == "correlated" )
            layer->setType( BuildLayer::TYPE_CORRELATED );
        else if ( type == "gridded" )
            layer->setType( BuildLayer::TYPE_GRIDDED );
        else if ( type == "new" )
            layer->setType( BuildLayer::TYPE_NEW );
        
        std::string source = e->getAttr( "source" );
        layer->setSource( proj->getSource( source ) );

        std::string terrain = e->getAttr( "terrain" );
        layer->setTerrain( proj->getTerrain( terrain ) );

        layer->setTargetPath( e->getAttr( "target" ) );

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


static BuildTarget*
decodeTarget( XmlElement* e, Project* proj )
{
    BuildTarget* target = NULL;
    if ( e )
    {
        target = new BuildTarget();
        target->setName( e->getAttr( "name" ) );
        
        Terrain* terrain = proj->getTerrain( e->getAttr( "terrain" ) );
        target->setTerrain( terrain );

        XmlNodeList layers = e->getSubElements( "layer" );
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


static Project*
decodeInclude( XmlElement* e, Project* proj )
{
    if ( e )
    {
        std::string uri = e->getText();
        if ( uri.length() > 0 )
        {
            uri = PathUtils::getAbsPath( proj->getBaseURI(), uri );

            osg::ref_ptr<Project> include_proj = XmlSerializer::loadProject( uri );
            if ( include_proj.valid() )
            {
                proj->merge( include_proj.get() );
            }
        }
    }
    return proj;
}


static Project* 
decodeProject( XmlElement* e, const std::string& source_uri )
{
    Project* project = NULL;
    if ( e )
    {
        project = new Project();
        project->setSourceURI( source_uri );
        project->setName( e->getAttr( "name" ) );
        project->setWorkingDirectory( e->getAttr( "workdir" ) );

        // includes
        XmlNodeList includes = e->getSubElements( "include" );
        for( XmlNodeList::const_iterator j = includes.begin(); j != includes.end(); j++ )
        {
            decodeInclude( static_cast<XmlElement*>( j->get() ), project );
        }
        
        // scripts
        XmlNodeList scripts = e->getSubElements( "script" );
        for( XmlNodeList::const_iterator j = scripts.begin(); j != scripts.end(); j++ )
        {
            Script* script = decodeScript( static_cast<XmlElement*>( j->get() ), project );
            if ( script )
                project->getScripts().push_back( script );
        }

        // resources
        XmlNodeList resources = e->getSubElements( "resource" );
        for( XmlNodeList::const_iterator j = resources.begin(); j != resources.end(); j++ )
        {
            Resource* resource = decodeResource( static_cast<XmlElement*>( j->get() ), project );
            if ( resource )
                project->getResources().push_back( resource );
        }

        // graphs
        XmlNodeList graphs = e->getSubElements( "graph" );
        for( XmlNodeList::const_iterator j = graphs.begin(); j != graphs.end(); j++ )
        {
            FilterGraph* graph = decodeFilterGraph( static_cast<XmlElement*>( j->get() ), project );
            if ( graph )
                project->getFilterGraphs().push_back( graph );
        }

        // terrains (depends on resources)
        XmlNodeList terrains = e->getSubElements( "terrain" );
        for( XmlNodeList::const_iterator j = terrains.begin(); j != terrains.end(); j++ )
        {
            Terrain* terrain = decodeTerrain( static_cast<XmlElement*>( j->get() ), project );
            if ( terrain )
                project->getTerrains().push_back( terrain );
        }

        // sources - 2 passes, since a source can reference another source
        XmlNodeList sources = e->getSubElements( "source" );
        for( XmlNodeList::const_iterator j = sources.begin(); j != sources.end(); j++ )
        {
            Source* source = decodeSource( static_cast<XmlElement*>( j->get() ), project, 0 );
            if ( source )
            {
                project->getSources().push_back( source );

                // also add each source as a feature layer resource
                Resource* resource = osgGIS::Registry::instance()->createResourceByType( "FeatureLayerResource" );
                resource->setBaseURI( project->getBaseURI() );
                resource->setURI( source->getURI() );
                resource->setName( source->getName() );
                project->getResources().push_back( resource );                
            }
        }
        for( XmlNodeList::const_iterator j = sources.begin(); j != sources.end(); j++ )
        {
            decodeSource( static_cast<XmlElement*>( j->get() ), project, 1 );
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

        // maps
        XmlNodeList maps = e->getSubElements( "map" );
        for( XmlNodeList::const_iterator j = maps.begin(); j != maps.end(); j++ )
        {
            RuntimeMap* map = decodeRuntimeMap( static_cast<XmlElement*>( j->get() ), project );
            if ( map )
                project->getMaps().push_back( map );
        }
    }
    return project;
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



Project*
XmlSerializer::readProject( Document* doc )
{
    Project* result = NULL;
    if ( doc )
    {
        XmlDocument* xml_doc = (XmlDocument*)doc;
        result = decodeProject( xml_doc->getSubElement( "project" ), xml_doc->getSourceURI() );
    }
    return result;
}


Document*
XmlSerializer::writeProject( Project* project )
{
    //TODO
    return new XmlDocument();
}


