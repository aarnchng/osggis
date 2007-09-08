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


ChangeShapeTypeFilter::ChangeShapeTypeFilter()
{
    new_type = GeoShape::TYPE_LINE;
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
