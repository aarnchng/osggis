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
XmlSerializer::readScript( Document* _doc )
{    
    Script* script = NULL;

    if ( _doc )
    {
        XmlDocument* doc = (XmlDocument*)_doc;

        XmlElement* e = doc->getElement( "script" );
        if ( e )
        {
            script = new Script();
            XmlNodeList filter_els = doc->getElements( "filter" );
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
    }

    return script;
}


Document*
XmlSerializer::writeScript( Script* script )
{
    XmlDocument* doc = new XmlDocument();

    if ( script )
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
        doc->getChildren().push_back( script_e );
    }

    return doc;
}
