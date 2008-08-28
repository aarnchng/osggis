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

#include <osgGIS/GeoExtent>
#include <osg/Notify>
#include <sstream>

using namespace osgGIS;


GeoExtent GeoExtent::invalid()
{
    return GeoExtent( false, false );
}

GeoExtent GeoExtent::empty()
{
    return GeoExtent( false, false );
}

GeoExtent GeoExtent::infinite()
{
    return GeoExtent( true, true );
}


GeoExtent::GeoExtent( bool _is_valid, bool _is_infinite )
{
    is_valid    = _is_valid;
    is_infinite = _is_infinite;
}


GeoExtent::GeoExtent()
{
    is_valid = true;
    is_infinite = false;
}


GeoExtent::GeoExtent( const GeoExtent& rhs )
{
    ne = rhs.ne;
    sw = rhs.sw;
    se = rhs.se;
    nw = rhs.nw;
    is_valid = rhs.is_valid;
    is_infinite = rhs.is_infinite;
    recalc();
}


GeoExtent::GeoExtent( const GeoPoint& _sw, const GeoPoint& _ne )
{
    is_valid = false;
    is_infinite = false;
    if ( _sw.isValid() && _sw.getSRS() && _ne.isValid() && _ne.getSRS() )
    {
        sw = _sw;
        ne = _sw.getSRS()->transform( _ne );
        if ( ne.isValid() )
        {
            is_valid = true;
            recalc();
        }
    }
}


GeoExtent::GeoExtent(const GeoPoint& _sw, 
                     const GeoPoint& _ne,
                     const SpatialReference* _sr )
{
    is_valid = false;
    is_infinite = false;
    if ( _sw.isValid() && _sw.getSRS() && _ne.isValid() && _ne.getSRS() && _sr )
    {
        sw = _sr->transform( _sw );
        ne = _sr->transform( _ne );
        if ( sw.isValid() && ne.isValid() )
        {
            is_valid = true;
            recalc();
        }
    }
}


GeoExtent::GeoExtent( double xmin, double ymin, double xmax, double ymax, const SpatialReference* _srs )
{
    is_infinite = false;
    sw = GeoPoint( xmin, ymin, _srs );
    ne = GeoPoint( xmax, ymax, _srs );
    is_valid = sw.isValid() && ne.isValid() && _srs;
    recalc();
}


GeoExtent::~GeoExtent()
{
	//NOP
}

bool
GeoExtent::isPoint() const
{
    return isValid() && !isInfinite() && sw.isValid() && ne.isValid() && sw == ne;
}

void
GeoExtent::recalc()
{
    if ( isValid() && !isInfinite() )
    {
        se = GeoPoint( ne.x(), sw.y(), sw.getSRS() );
        nw = GeoPoint( sw.x(), ne.y(), sw.getSRS() );
    }

    if ( sw.x() > ne.x() || sw.y() > ne.y() )
    {
        is_valid = false;
    }
}


const GeoPoint&
GeoExtent::getSouthwest() const
{
	return sw;
}


const GeoPoint&
GeoExtent::getNortheast() const
{
	return ne;
}


const GeoPoint&
GeoExtent::getSoutheast() const
{
    return se;
}


const GeoPoint&
GeoExtent::getNorthwest() const
{
    return nw;
}


const double
GeoExtent::getXMin() const 
{
    return isValid()? sw.x() : -1.0;
}


const double 
GeoExtent::getXMax() const
{
    return isValid()? ne.x() : -1.0;
}


const double 
GeoExtent::getYMin() const
{
    return isValid()? sw.y() : -1.0;
}


const double
GeoExtent::getYMax() const
{
    return isValid()? ne.y() : -1.0;
}


const double
GeoExtent::getWidth() const
{
    return getXMax() - getXMin();
}


const double
GeoExtent::getHeight() const
{
    return getYMax() - getYMin();
}


GeoPoint
GeoExtent::getCentroid() const
{
    GeoPoint result;
    if ( isValid() && !isInfinite() )
    {
        result = GeoPoint(
                (sw.x() + ne.x()) / 2.0,
                (sw.y() + ne.y()) / 2.0,
                getSRS() );
    }
    return result;
}


double
GeoExtent::getArea() const
{
    if ( isValid() )
    {
        if ( isInfinite() )
        {
            return -1.0;
        }
        else if ( isEmpty() )
        {
            return 0.0;
        }
        else
        {
            return (ne.x()-sw.x()) * (ne.y()-sw.y());
        }
    }
    else
    {
        return -1.0;
    }
}

GeoExtent
GeoExtent::getIntersection( const GeoExtent& _rhs ) const
{
    GeoExtent rhs = getSRS()->transform( _rhs );

    if (rhs.getXMin() >= getXMax() || rhs.getXMax() <= getXMin() ||
        rhs.getYMin() >= getYMax() || rhs.getYMax() <= getYMin() )
    {
        return GeoExtent::empty();
    }

    double xmin = rhs.getXMin() < getXMin()? getXMin() : rhs.getXMin();
    double xmax = rhs.getXMax() > getXMax()? getXMax() : rhs.getXMax();
    double ymin = rhs.getYMin() < getYMin()? getYMin() : rhs.getYMin();
    double ymax = rhs.getYMax() > getYMax()? getYMax() : rhs.getYMax();
    
    return GeoExtent( xmin, ymin, xmax, ymax, getSRS() );
}


bool
GeoExtent::isValid() const
{
    return is_valid;
}


bool
GeoExtent::isInfinite() const
{
    return is_infinite;
}


bool
GeoExtent::isFinite() const
{
    return getArea() > 0;
}


bool
GeoExtent::isEmpty() const
{
    return !isValid() || ( !isInfinite() && ( !sw.isValid() || !ne.isValid() ) );
}


const SpatialReference*
GeoExtent::getSRS() const
{
    return sw.isValid()? sw.getSRS() : NULL;
}


SpatialReference*
GeoExtent::getSRS()
{
    return sw.isValid()? sw.getSRS() : NULL;
}


bool
GeoExtent::intersects( const GeoPoint& input ) const
{
    if ( isValid() && input.isValid() && !isEmpty() )
    {
        if ( isInfinite() )
            return true;

        GeoPoint point = sw.getSRS()->transform( input );

        return
            point.isValid() && 
            point.x() >= sw.x() && point.x() <= ne.x() &&
            point.y() >= sw.y() && point.y() <= ne.y();
    }

    return false;              
}


bool
GeoExtent::intersects( const GeoExtent& input ) const
{
    if ( !isValid() || !input.isValid() || isEmpty() || input.isEmpty() )
        return false;

    if ( isInfinite() || input.isInfinite() )
        return true;

    GeoPoint input_sw = getSRS()->transform( input.getSouthwest() );
    GeoPoint input_ne = getSRS()->transform( input.getNortheast() );

    bool isect;

    if (ne.x() < input_sw.x() || sw.x() > input_ne.x() || 
        ne.y() < input_sw.y() || sw.y() > input_ne.y() )
    {
        isect = false;
    }
    else
    {
        isect = true;
    }

    return isect;
}


bool
GeoExtent::intersectsExtent( const GeoPointList& input ) const
{
    GeoExtent input_extent;
    input_extent.expandToInclude( input );
    return intersects( input_extent );
}


bool
GeoExtent::contains( double x, double y ) const 
{
    return contains( GeoPoint( x, y, getSRS() ) );
}

bool
GeoExtent::contains( const GeoPoint& input ) const 
{
    if ( isInfinite() && input.isValid() )
        return true;

    if ( !isValid() || !input.isValid() )
        return false;

    GeoPoint p = getSRS()->transform( input );
    if ( !p.isValid() )
        return false;

    return sw.x() <= p.x() && p.x() <= ne.x() && sw.y() <= p.y() && p.y() <= ne.y();
}


bool
GeoExtent::contains( const GeoExtent& input ) const
{
    return
        input.isValid() &&
        contains( input.getSouthwest() ) &&
        contains( input.getNortheast() );
}


bool
GeoExtent::contains( const GeoPointList& input ) const
{
    for( unsigned int i=0; i<input.size(); i++ )
        if ( !contains( input[i] ) )
            return false;
    return true;
}

void
GeoExtent::expand( double x, double y )
{
    sw.x() -= .5*x;
    ne.x() += .5*x;
    sw.y() -= .5*y;
    ne.y() += .5*y;
}

void
GeoExtent::expandToInclude( const GeoPoint& input )
{
    if ( !isValid() || !input.isValid() )
    {
        osgGIS::notify( osg::WARN ) << "GeoExtent::expandToInclude: Illegal: either the extent of the input point is invalid" << std::endl;
        return;
    }

    if ( !isInfinite() )
    {
        if ( isEmpty() )
        {
		    ne = sw = input;
        }
    	else
    	{
            GeoPoint new_point = getSRS()->transform( input );
            if ( new_point.isValid() )
            {
		        double xmin = sw.x();
		        double ymin = sw.y();
		        double xmax = ne.x();
		        double ymax = ne.y();

		        if ( new_point.x() < xmin )
    			    xmin = new_point.x();
		        if ( new_point.x() > xmax )
    			    xmax = new_point.x();
		        if ( new_point.y() < ymin )
    			    ymin = new_point.y();
		        if ( new_point.y() > ymax )
    			    ymax = new_point.y();

                sw.set( xmin, ymin, sw.z() );
                ne.set( xmax, ymax, ne.z() );
            }
            else
            {
                osgGIS::notify( osg::WARN ) 
                    << "GeoExtent::expandToInclude: "
                    << "Unable to reproject coordinates" << std::endl;
            }
        }
	}
    
    recalc();
}


void
GeoExtent::expandToInclude( const GeoExtent& input )
{
    if ( input.isValid() && !input.isEmpty() && !isInfinite() )
    {
        if ( input.isInfinite() )
        {
            is_infinite = true;
        }
        else
        {
	        expandToInclude( input.getSouthwest() );
	        expandToInclude( input.getNortheast() );
        }
    }
}


void
GeoExtent::expandToInclude( const GeoPointList& input )
{
    for( GeoPointList::const_iterator i = input.begin(); i != input.end(); i++ )
        expandToInclude( *i );
}


std::string
GeoExtent::toString() const
{
    if ( !isValid() )
    {
        return "INVALID";
    }
	else if ( isEmpty() )
	{
		return "EMPTY";
	}
    else if ( isInfinite() )
    {
        return "INFINITE";
    }
	else
	{
		std::stringstream str;
		str
			<< "("
			<< sw.x() << ", " << sw.y()
			<< " => "
			<< ne.x() << ", " << ne.y()
			<< ")";

		return str.str();
	}
}

