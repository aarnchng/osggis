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

#include <osgGIS/RemoveHolesFilter>
#include <osgGIS/Utils>
#include <float.h>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( RemoveHolesFilter );



RemoveHolesFilter::RemoveHolesFilter()
{
    //NOP
}

RemoveHolesFilter::RemoveHolesFilter( const RemoveHolesFilter& rhs )
: FeatureFilter( rhs )
{
    //NOP
}

RemoveHolesFilter::~RemoveHolesFilter()
{
    //NOP
}


FeatureList
RemoveHolesFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    for( GeoShapeList::iterator i = input->getShapes().begin(); i != input->getShapes().end(); i++ )
    {
        GeoShape& shape = *i;

        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        {
            GeoPartList new_parts;

            for( GeoPartList::iterator j = shape.getParts().begin(); j != shape.getParts().end(); j++ )
            {
                if ( j->size() > 2 && GeomUtils::isPolygonCCW( *j ) )
                    new_parts.push_back( *j );
            }

            shape.getParts().swap( new_parts );
        }
    }

    output.push_back( input );
    return output;
}