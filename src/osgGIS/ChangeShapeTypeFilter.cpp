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

#include <osgGIS/ChangeShapeTypeFilter>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( ChangeShapeTypeFilter );


ChangeShapeTypeFilter::ChangeShapeTypeFilter()
{
    new_type = GeoShape::TYPE_LINE;
}

ChangeShapeTypeFilter::ChangeShapeTypeFilter( const ChangeShapeTypeFilter& rhs )
: FeatureFilter( rhs ),
  new_type( rhs.new_type )
{
    //NOP
}

ChangeShapeTypeFilter::ChangeShapeTypeFilter( const GeoShape::ShapeType& _new_type )
{
    new_type = _new_type;
}

ChangeShapeTypeFilter::~ChangeShapeTypeFilter()
{
}

void 
ChangeShapeTypeFilter::setNewShapeType( const GeoShape::ShapeType& _new_type )
{
    new_type = _new_type;
}

const GeoShape::ShapeType&
ChangeShapeTypeFilter::getNewShapeType() const
{
    return new_type;
}

void
ChangeShapeTypeFilter::setProperty( const Property& p )
{
    if ( p.getName() == "new_shape_type" )
    {
        setNewShapeType(
            p.getValue() == "point"? GeoShape::TYPE_POINT :
            p.getValue() == "line"? GeoShape::TYPE_LINE :
            p.getValue() == "polygon"? GeoShape::TYPE_POINT :
            GeoShape::TYPE_UNSPECIFIED );
    }
    FeatureFilter::setProperty( p );
}

Properties
ChangeShapeTypeFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    p.push_back( Property( "new_shape_type", 
        getNewShapeType() == GeoShape::TYPE_POINT? "point" :
        getNewShapeType() == GeoShape::TYPE_LINE? "line" :
        getNewShapeType() == GeoShape::TYPE_POLYGON? "polygon" :
        std::string("unspecified") ) );
    return p;
}

FeatureList
ChangeShapeTypeFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    GeoShapeList& shapes = input->getShapes();
    for( GeoShapeList::iterator i = shapes.begin(); i != shapes.end(); i++ )
    {
        i->setShapeType( new_type );
    }

    output.push_back( input );
    return output;
}
