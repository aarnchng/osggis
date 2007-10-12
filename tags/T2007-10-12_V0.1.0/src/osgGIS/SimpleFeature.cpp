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

#include <osgGIS/SimpleFeature>
#include <osg/Notify>
#include <algorithm>

using namespace osgGIS;

static FeatureOID NULL_FEATURE_OID = -1;

SimpleFeature::SimpleFeature()
{
    //NOP
}


const FeatureOID&
SimpleFeature::getOID() const
{
    return NULL_FEATURE_OID;
}


const GeoShapeList&
SimpleFeature::getShapes() const
{
    return shapes;
}


GeoShapeList&
SimpleFeature::getShapes()
{
    return shapes;
}


const GeoExtent&
SimpleFeature::getExtent() const
{
    return extent;
}


Attribute
SimpleFeature::getAttribute( const std::string& key ) const
{
    AttributeTable::const_iterator i = attributes.find( key );
    return i != attributes.end()? i->second : Attribute::invalid();
}


void 
SimpleFeature::setAttribute( const std::string& key, const std::string& value )
{
    attributes[key] = Attribute( key, value );
}

void 
SimpleFeature::setAttribute( const std::string& key, int value )
{
    attributes[key] = Attribute( key, value );
}

void 
SimpleFeature::setAttribute( const std::string& key, double value )
{
    attributes[key] = Attribute( key, value );
}
