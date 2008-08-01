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

#include <osgGISProjects/QuadKey>
#include <osgGIS/GeoExtent>
#include <osgGIS/Registry>
#include <sstream>

using namespace osgGISProjects;
using namespace osgGIS;

#define DEFAULT_STRING_STYLE STYLE_QUADKEY

QuadMap::QuadMap()
: bounds( GeoExtent::invalid() ),
  string_style( DEFAULT_STRING_STYLE )
{
    //NOP
}

QuadMap::QuadMap( const GeoExtent& _bounds )
: bounds( _bounds ),
  string_style( DEFAULT_STRING_STYLE )
{
    //NOP
}

QuadMap::QuadMap( const QuadMap& rhs )
: bounds( rhs.bounds ),
  string_style( rhs.string_style )
{
    //NOP
}

bool
QuadMap::isValid() const
{
    return bounds.isValid();
}

const GeoExtent&
QuadMap::getBounds() const
{
    return bounds;
}

void
QuadMap::getCellSize( unsigned int lod, double& out_width, double& out_height ) const
{
    out_width = getBounds().getWidth();
    out_height = getBounds().getHeight();

    for( unsigned int i=0; i<lod; i++ )
    {
        out_width *= 0.5;
        out_height *= 0.5;
    }
}

unsigned int
QuadMap::getStartingLodForCellsOfSize( double width, double height ) const
{
    unsigned int lod = 1;
    for( ; ; lod++ )
    {
        double cell_x, cell_y;
        getCellSize( lod-1, cell_x, cell_y );
        if ( width > cell_x || height > cell_y )
            break;
    }
    return lod-1;
}

bool
QuadMap::getCells(const GeoExtent& aoi, unsigned int lod,
                  unsigned int& out_cell_xmin, unsigned int& out_cell_ymin,
                  unsigned int& out_cell_xmax, unsigned int& out_cell_ymax ) const
{
    GeoExtent map_aoi = bounds.getSRS()->transform( aoi );
    if ( map_aoi.isValid() )
    {
        double cell_width, cell_height;
        getCellSize( lod, cell_width, cell_height );
        double xmin = (map_aoi.getXMin()-bounds.getXMin()) / cell_width;
        double xmax = (map_aoi.getXMax()-bounds.getXMin()) / cell_width;
        double ymin = (map_aoi.getYMin()-bounds.getYMin()) / cell_height;
        double ymax = (map_aoi.getYMax()-bounds.getYMin()) / cell_height;

        out_cell_xmin = (unsigned int)xmin;
        out_cell_xmax = (unsigned int)( xmax < ceil(xmax)? xmax : xmax - 1.0 );
        out_cell_ymin = (unsigned int)ymin;
        out_cell_ymax = (unsigned int)( ymax < ceil(ymax)? ymax : ymax - 1.0 );

        return true;
    }
    return false;
}

QuadMap::StringStyle
QuadMap::getStringStyle() const {
    return string_style;
}

void
QuadMap::setStringStyle( QuadMap::StringStyle value ) {
    string_style = value;
}


/*****************************************************************************/


QuadKey::QuadKey( const QuadKey& rhs )
: qstr( rhs.qstr ),
  extent( rhs.extent ),
  map( rhs.map )
{
    //NOP
}

QuadKey::QuadKey( const std::string& _qstr, const QuadMap& _map )
: qstr( _qstr ),
  map( _map ),
  extent( GeoExtent::invalid() )
{
}

QuadKey::QuadKey( unsigned int cell_x, unsigned int cell_y, unsigned int lod, const QuadMap& _map )
: map( _map ),
  extent( GeoExtent::invalid() )
{
    std::stringstream ss;
    for( unsigned i = lod; i > 0; i-- )
    {
        char digit = '0';
        unsigned int mask = 1 << (i-1);
        if ( (cell_x & mask) != 0 )
        {
            digit++;
        }
        if ( (cell_y & mask) != 0 )
        {
            digit += 2;
        }
        ss << digit;
    }
    qstr = ss.str();
}

unsigned int
QuadKey::getLOD() const
{
    return qstr.length();
}

std::string
QuadKey::toString() const
{
    switch( map.getStringStyle() )
    {
    case QuadMap::STYLE_LOD_QUADKEY:
        std::stringstream s;
        s << "L" << getLOD() << "_" << qstr;
        return s.str();
    }
        
    return qstr;
}

const QuadMap&
QuadKey::getMap() const
{
    return map;
}

QuadKey
QuadKey::createSubKey( unsigned int quadrant ) const
{
    return QuadKey( qstr + (char)('0'+quadrant), getMap() );
}

QuadKey
QuadKey::createParentKey() const
{
    return qstr.length() > 0?
        QuadKey( qstr.substr( 0, qstr.length()-1 ), map ) :
        QuadKey( qstr, map );
}

void
QuadKey::getCellSize( double& out_width, double& out_height ) const
{
    map.getCellSize( getLOD(), out_width, out_height );
}

const GeoExtent&
QuadKey::getExtent() const
{
    if ( !extent.isValid() )
    {
        unsigned int lod = getLOD();
        double xmin = map.getBounds().getXMin();
        double ymin = map.getBounds().getYMin();
        double dx = map.getBounds().getWidth();
        double dy = map.getBounds().getHeight();
        for( unsigned int i=0; i<lod; i++ )
        {
            dx *= 0.5;
            dy *= 0.5;
            switch( qstr[i] ) {
                case '1': xmin += dx; break;
                case '2': ymin += dy; break;
                case '3': xmin += dx; ymin += dy; break;
            }
        }
        double xmax = xmin + dx;
        double ymax = ymin + dy;
        const_cast<QuadKey*>(this)->extent = 
            GeoExtent( xmin, ymin, xmax, ymax, map.getBounds().getSRS() );
    }
    return extent;
}

bool
QuadKey::operator < ( const QuadKey& rhs ) const
{
    return toString() < rhs.toString();
}