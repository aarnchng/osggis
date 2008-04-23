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

#include <osgGIS/RasterResource>
#include <osgGIS/Utils>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/TexEnv>
#include <OpenThreads/ScopedLock>

using namespace osgGIS;
using namespace OpenThreads;

#include <osgGIS/Registry>
OSGGIS_DEFINE_RESOURCE(RasterResource);


RasterResource::RasterResource()
{
    init();
}

RasterResource::RasterResource( const std::string& _name )
: Resource( _name )
{
    init();
}

void
RasterResource::init()
{
    //NOP
}

RasterResource::~RasterResource()
{
    //NOP
}

void
RasterResource::setProperty( const Property& prop )
{
    Resource::setProperty( prop );
}

Properties
RasterResource::getProperties() const
{
    Properties props = Resource::getProperties();
    return props;
}

bool
RasterResource::applyToStateSet( osg::StateSet* state_set, const GeoExtent& aoi, int max_span_pixels, const std::string& image_name, osg::Image** out_image )
{
    bool result = false;

    osg::ref_ptr<RasterStore> rstore = Registry::instance()->getRasterStoreFactory()->connectToRasterStore( getAbsoluteURI() );
    if ( rstore.valid() )
    {
        osg::ref_ptr<osg::Image> image = rstore->getImage( aoi, max_span_pixels, true );
        if ( image.valid() )
        {
            // handy in case we want to write it out later
            image->setFileName( image_name );

            osg::Texture* tex = new osg::Texture2D( image.get() );
            tex->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
            tex->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
            tex->setResizeNonPowerOfTwoHint( false );

            osg::TexEnv* texenv = new osg::TexEnv();
            texenv = new osg::TexEnv();
            texenv->setMode( osg::TexEnv::MODULATE );

            state_set->setTextureAttributeAndModes( 0, tex, osg::StateAttribute::ON );
            state_set->setTextureAttribute( 0, texenv, osg::StateAttribute::ON );

            if ( out_image )
            {
                *out_image = image.get();
            }

            result = true;
        }
        else
        {
            osg::notify(osg::WARN) << "RasterResource::createStateSet failed to get Image from raster store" << std::endl;
        }
    }
    else
    {
        osg::notify(osg::WARN) << "RasterResource::createStateSet failed to connect to raster store at " << getAbsoluteURI() << std::endl;
    }

    return result;
}
