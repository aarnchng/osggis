/**
* osgGIS - GIS Library for OpenSceneGraph
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

#include <osgGIS/ElevationResource>
#include <osg/Image>

using namespace osgGIS;
using namespace OpenThreads;

#include <osgGIS/Registry>
OSGGIS_DEFINE_RESOURCE(ElevationResource);


ElevationResource::ElevationResource()
{
    //NOP
}

ElevationResource::ElevationResource( const std::string& _name )
: Resource( _name )
{
    //NOP
}

ElevationResource::~ElevationResource()
{
    //NOP
}

void
ElevationResource::setProperty( const Property& prop )
{
    Resource::setProperty( prop );
}

Properties
ElevationResource::getProperties() const
{
    Properties props = Resource::getProperties();
    return props;
}


class ElevationGridImpl : public ElevationGrid
{
public:
    ElevationGridImpl( const GeoExtent& aoi, const std::string& _store_uri, ReentrantMutex& _mutex )
        : store_uri( _store_uri ),
          mutex( _mutex )
    {
        cache( aoi );
    }

public:
    bool getHeight( const GeoPoint& point, double& out_height ) const
    {
        if ( !hf.valid() || !hf_aoi.isValid() )
            return false;

        // bring the query point into the grid's SRS:
        GeoPoint local_point = hf_aoi.getSRS()->transform( point );

        // borrowed this code from VPB::SourceData::getInterpolatedValue
        double c = (local_point.x() - hf_aoi.getXMin()) / hf_aoi.getWidth();
        double r = (local_point.y() - hf_aoi.getYMin()) / hf_aoi.getHeight();

        int rowMin = osg::maximum((int)floor(r), 0);
        int rowMax = osg::maximum(osg::minimum((int)ceil(r), (int)(hf->getNumRows()-1)), 0);
        int colMin = osg::maximum((int)floor(c), 0);
        int colMax = osg::maximum(osg::minimum((int)ceil(c), (int)(hf->getNumColumns()-1)), 0);

        if (rowMin > rowMax) rowMin = rowMax;
        if (colMin > colMax) colMin = colMax;

        float urHeight = hf->getHeight(colMax, rowMax);
        float llHeight = hf->getHeight(colMin, rowMin);
        float ulHeight = hf->getHeight(colMin, rowMax);
        float lrHeight = hf->getHeight(colMax, rowMin);

        double x_rem = c - (int)c;
        double y_rem = r - (int)r;

        double w00 = (1.0 - y_rem) * (1.0 - x_rem) * (double)llHeight;
        double w01 = (1.0 - y_rem) * x_rem * (double)lrHeight;
        double w10 = y_rem * (1.0 - x_rem) * (double)ulHeight;
        double w11 = y_rem * x_rem * (double)urHeight;

        out_height = (double)(w00 + w01 + w10 + w11);
        return true;
    }


private:
    void cache( const GeoExtent& extent )
    {
        osg::ref_ptr<RasterStore> rstore = Registry::instance()->getRasterStoreFactory()->connectToRasterStore( store_uri );
        if ( rstore.valid() )
        {
            hf = rstore->createHeightField( extent );
            hf_aoi = extent;
        }
        else
        {
            hf = NULL;
            hf_aoi = GeoExtent::invalid();
        }
    }

private:
    std::string store_uri;
    GeoExtent hf_aoi;
    osg::ref_ptr<osg::HeightField> hf;
    ReentrantMutex& mutex;
};


ElevationGrid*
ElevationResource::createGrid( const GeoExtent& aoi ) const
{
    return new ElevationGridImpl( aoi, getAbsoluteURI(), const_cast<ElevationResource*>(this)->getMutex() );
}

