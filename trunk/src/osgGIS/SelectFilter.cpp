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

#include <osgGIS/SelectFilter>
#include <osg/Notify>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( SelectFilter );


SelectFilter::SelectFilter()
{
    select_expr = "";
}


SelectFilter::SelectFilter( const std::string& _select_expr )
{
    setSelectExpr( _select_expr );
}


SelectFilter::~SelectFilter()
{
    //NOP
}

void
SelectFilter::setSelectExpr( const std::string& value )
{
    select_expr = value;
}

const std::string&
SelectFilter::getSelectExpr() const
{
    return select_expr;
}

void
SelectFilter::setProperty( const Property& p )
{
    if ( p.getName() == "select" )
        setSelectExpr( p.getValue() );
    FeatureFilter::setProperty( p );
}

Properties
SelectFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getSelectExpr().length() > 0 )
        p.push_back( Property( "select", getSelectExpr() ) );
    return p;
}

FeatureList
SelectFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;
    
    //todo
    output.push_back( input );

    return output;
}