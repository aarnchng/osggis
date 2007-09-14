#include <osgGISProjects/XmlDOM>
#include <osg/notify>
#include <expat.h>
#include <algorithm>

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
XmlElement::getElement( const std::string& name )
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

XmlNodeList 
XmlElement::getElements( const std::string& name ) const
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