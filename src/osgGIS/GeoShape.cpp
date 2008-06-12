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

#include <osgGIS/GeoShape>
#include <osg/Notify>

using namespace osgGIS;

GeoShape::GeoShape()
{
    extent_cache  = GeoExtent::invalid();
}


GeoShape::GeoShape( const GeoShape& rhs )
{
    shape_type    = rhs.shape_type;
    parts         = rhs.parts;
    extent_cache  = rhs.extent_cache;
    srs           = rhs.srs.get();
}


GeoShape::GeoShape( const GeoShape::ShapeType& _shape_type,
                    const SpatialReference*    _srs )
: shape_type( _shape_type ),
  srs( (SpatialReference*)_srs )
{
    extent_cache = GeoExtent::invalid();
}


GeoShape::~GeoShape()
{
	//NOP
}


const GeoShape::ShapeType&
GeoShape::getShapeType() const
{
	return shape_type;
}


void
GeoShape::setShapeType( const ShapeType& _type )
{
    shape_type = _type;
}


const SpatialReference*
GeoShape::getSRS() const
{
    return srs.get();
}


SpatialReference*
GeoShape::getSRS()
{
    return srs.get();
}


void
GeoShape::setSpatialReference( const SpatialReference* _srs )
{
    extent_cache = GeoExtent::invalid();
    srs = (SpatialReference*)_srs;
}


unsigned int
GeoShape::getPartCount() const
{
    return parts.size();
}


const GeoPointList&
GeoShape::getPart( unsigned int i ) const
{
    return parts[i];
}


GeoPointList&
GeoShape::getPart( unsigned int i )
{
    extent_cache = GeoExtent::invalid();
    return parts[i];
}


GeoPartList&
GeoShape::getParts()
{
    extent_cache = GeoExtent::invalid();
    return parts;
}


const GeoPartList&
GeoShape::getParts() const
{
    return parts;
}


GeoPointList&
GeoShape::addPart()
{
    extent_cache = GeoExtent::invalid();
    parts.push_back( GeoPointList() );
    return parts.back();
}


GeoPointList&
GeoShape::addPart( const GeoPointList& part )
{
    extent_cache = GeoExtent::invalid();
    parts.push_back( part );
    return parts.back();
}


GeoPointList&
GeoShape::addPart( unsigned int size )
{
    extent_cache = GeoExtent::invalid();
    parts.push_back( GeoPointList( size ) );
    return parts.back();
}


unsigned int
GeoShape::getTotalPointCount() const
{
    unsigned int total = 0;
    for( GeoPartList::const_iterator i = parts.begin(); i != parts.end(); i++ )
        total += i->size();
    return total;
}


const GeoExtent&
GeoShape::getExtent() const
{
    if ( !extent_cache.isValid() )
    {
        struct ExtentVisitor : public GeoPointVisitor {
            ExtentVisitor() : e( GeoExtent::invalid() ) { }
            GeoExtent e;
            bool visitPoint( const GeoPoint& p ) {
                if ( !e.isValid() && p.isValid() )
                    e = GeoExtent();
                e.expandToInclude( p );
                return true;
            }
        };

        ExtentVisitor vis;
        accept( vis );

        // cast to non-const is OK for caching only
        const_cast<GeoShape*>(this)->extent_cache = vis.e;
    }
    return extent_cache;
}


bool
GeoShape::accept( GeoPointVisitor& visitor )
{
    extent_cache = GeoExtent::invalid();
    for( unsigned int pi = 0; pi < getPartCount(); pi++ )
    {
        GeoPointList& part = getPart( pi );
        for( unsigned int vi = 0; vi < part.size(); vi++ )
        {
            if ( !visitor.visitPoint( part[vi] ) )
                return false;
        }
    }
    return true;
}


bool
GeoShape::accept( GeoPointVisitor& visitor ) const
{
    for( unsigned int pi = 0; pi < getPartCount(); pi++ )
    {
        const GeoPointList& part = getPart( pi );
        for( unsigned int vi = 0; vi < part.size(); vi++ )
        {
            if ( !visitor.visitPoint( part[vi] ) )
                return false;
        }
    }
    return true;
}

bool
GeoShape::intersects( const GeoExtent& ex ) const
{
    if ( ex.isInfinite() )
        return true;

    for( GeoPartList::const_iterator i = getParts().begin(); i != getParts().end(); i++ )
    {
        const GeoPointList& part = *i;
        if ( part.intersects( ex ) )
            return true;
    }
    return false;
}

/**************************************************************************/


bool
GeoShapeList::intersects( const GeoExtent& ex ) const
{
    if ( ex.isInfinite() )
        return true;

    for( GeoShapeList::const_iterator i = begin(); i != end(); i++ )
    {
        const GeoShape& shape = *i;
        if ( shape.intersects( ex ) )
            return true;
    }
    return false;
}


bool
GeoShapeList::accept( GeoPartVisitor& visitor )
{
    for( GeoShapeList::iterator i = begin(); i != end(); i++ )
    {
        for( GeoPartList::iterator j = (*i).getParts().begin(); j != (*i).getParts().end(); j++ )
        {
            if ( !visitor.visitPart( *j ) )
                return false;
        }
    }
    return true;
}

bool 
GeoShapeList::accept( GeoPartVisitor& visitor ) const
{
    for( GeoShapeList::const_iterator i = begin(); i != end(); i++ )
    {
        for( GeoPartList::const_iterator j = (*i).getParts().begin(); j != (*i).getParts().end(); j++ )
        {
            if ( !visitor.visitPart( *j ) )
                return false;;
        }
    }
    return true;
}

