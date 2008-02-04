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

#include <osgGIS/ModelResource>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osg/ProxyNode>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_RESOURCE(ModelResource);


ModelResource::ModelResource()
{
    init();
}

ModelResource::ModelResource( const std::string& _name )
: Resource( _name )
{
    init();
}

void
ModelResource::init()
{
    //nop
}

ModelResource::~ModelResource()
{
    //NOP
}

void
ModelResource::setProperty( const Property& prop )
{
    if ( prop.getName() == "model_path" || prop.getName() == "path" )
        setPath( prop.getValue() );
    else
        Resource::setProperty( prop );
}

Properties
ModelResource::getProperties() const
{
    Properties props = Resource::getProperties();
    props.push_back( Property( "path", getPath() ) );
    return props;
}

void 
ModelResource::setPath( const std::string& value )
{
    path = value;
}

const std::string& 
ModelResource::getPath() const
{
    return path;
}

osg::Node*
ModelResource::createNode()
{
    //osg::ProxyNode* proxy = new osg::ProxyNode();
    //proxy->addChild( new osg::Group(), getPath() );
    //return proxy;

    osg::Node* node = osgDB::readNodeFile( getPath() );
    return node;
};

ModelResourceQuery::ModelResourceQuery()
{
    //nop
}

void 
ModelResourceQuery::addTag( const char* tag )
{
    tags.push_back( tag );
}

const TagList& 
ModelResourceQuery::getTags() const
{
    return tags;
}


const std::string& 
ModelResourceQuery::getHashCode()
{
    //TODO
    return "";
}