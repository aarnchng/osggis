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

#include <osgGIS/Units>

using namespace osgGIS;

// linear baseline = meters
Units Units::METERS    ( Units::TYPE_LINEAR, 1.0 );
Units Units::FEET      ( Units::TYPE_LINEAR, 0.3048 );
Units Units::KILOMETERS( Units::TYPE_LINEAR, 1000.0 );
Units Units::MILES     ( Units::TYPE_LINEAR, 1609.344 );

// angular baseline = degrees
Units Units::DEGREES   ( Units::TYPE_ANGULAR, 1.0 );
Units Units::RADIANS   ( Units::TYPE_ANGULAR, 57.2957795 );

Units::Units( Type _type, double _to_baseline )
: type( _type ), to_baseline(_to_baseline)
{
    //NOP
}

bool
Units::convert( double value, const osgGIS::Units &from, const osgGIS::Units &to, double &output )
{
    if ( from.getType() != to.getType() ) 
        return false;

    output = value * from.getBaseline() / to.getBaseline();
    return true;
}

#define METERS_IN_ONE_DEG_OF_LAT 111120.0

bool
Units::convertAngularToLinearVector(const osg::Vec2d& p0_in, const osg::Vec2d& p1_in,
                                    const Units& from,
                                    const Units& to,
                                    osg::Vec2d& out_linear_vector )
{
    // convert input to degrees:
    osg::Vec2d p0( p0_in.x() * from.getBaseline(), p0_in.y() * from.getBaseline() );
    osg::Vec2d p1( p1_in.x() * from.getBaseline(), p1_in.y() * from.getBaseline() );

    double dx_m = (p1.x()-p0.x()) * METERS_IN_ONE_DEG_OF_LAT * sin( osg::DegreesToRadians( p0.y() ) );
    double dy_m = (p1.y()-p0.y()) * METERS_IN_ONE_DEG_OF_LAT;

    out_linear_vector.set( dx_m / to.getBaseline(), dy_m / to.getBaseline() );
    return true;
}

bool
Units::convertLinearToAngularVector(const osg::Vec2d& input_in,
                                    const Units& from,
                                    const Units& to,
                                    const osg::Vec2d& angular_p0,
                                    osg::Vec2d& out_angular_p1)
{
    osg::Vec2d input( input_in.x() * from.getBaseline(), input_in.y() * from.getBaseline() );
    
    double dy_deg = input.y() / METERS_IN_ONE_DEG_OF_LAT;
    out_angular_p1.y() = angular_p0.y() + dy_deg;
    double dx_deg = input.x() / ( METERS_IN_ONE_DEG_OF_LAT * cos( osg::DegreesToRadians( out_angular_p1.y() ) ) );
    out_angular_p1.x() = angular_p0.x() + dx_deg;

    out_angular_p1 *= to.getBaseline();
    return true;
}

Units::Type
Units::getType() const
{
    return type;
}

double 
Units::getBaseline() const
{
    return to_baseline;
}

bool
Units::isAngular() const
{
    return type == TYPE_ANGULAR;
}

bool
Units::isLinear() const
{
    return type == TYPE_LINEAR;
}