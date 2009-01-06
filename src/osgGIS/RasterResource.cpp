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

#define DEFAULT_TEXTURE_MODE osg::TexEnv::MODULATE

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
    parts_initialized = false;
    texture_mode = DEFAULT_TEXTURE_MODE;
}

RasterResource::~RasterResource()
{
    //NOP
}

void
RasterResource::addPartURI( const std::string& uri )
{
    parts.push_back( RasterPart( GeoExtent::invalid(), uri ) );
    parts_initialized = false;
}

void
RasterResource::setProperty( const Property& prop )
{
    if ( prop.getName() == "texture_mode" )
        setTextureMode( prop.getValue()=="decal"? osg::TexEnv::DECAL: prop.getValue()=="replace"? osg::TexEnv::REPLACE: prop.getValue()=="blend"? osg::TexEnv::BLEND : osg::TexEnv::MODULATE );
    Resource::setProperty( prop );
}

Properties
RasterResource::getProperties() const
{
    Properties props = Resource::getProperties();
    if ( getTextureMode() != DEFAULT_TEXTURE_MODE )
        props.push_back( Property( "texture_mode", getTextureMode()==osg::TexEnv::DECAL? "decal" : getTextureMode()==osg::TexEnv::REPLACE? "replace" : getTextureMode()==osg::TexEnv::BLEND? "blend" : "modulate" ) );
    return props;
}

void
RasterResource::setTextureMode( const osg::TexEnv::Mode& value )
{
    texture_mode = value;
}

const osg::TexEnv::Mode&
RasterResource::getTextureMode() const
{
    return texture_mode;
}

bool
RasterResource::applyToStateSet(osg::StateSet*     state_set,
                                const GeoExtent&   aoi,
                                unsigned int       max_pixel_span,
                                osg::Image**       out_image )
{
    if ( !state_set || !aoi.isValid() || aoi.getArea() <= 0.0 )
        return false;

    bool result = false;

    if ( !parts_initialized )
        initParts();

    unsigned int image_width, image_height;

    osg::ref_ptr<osg::Image> image;
    
    // iterate over all the image parts, extract the appropriate parts, and piece them together:
    for( RasterParts::const_iterator i = parts.begin(); i != parts.end(); i++ )
    {
        const GeoExtent& part_extent = i->first;
        if ( part_extent.intersects( aoi ) )
        {
            std::string part_auri = PathUtils::getAbsPath( getBaseURI(), i->second );
            osg::ref_ptr<RasterStore> rstore = Registry::instance()->getRasterStoreFactory()->connectToRasterStore( part_auri );
            if ( rstore.valid() )
            {   
                // allocate the main image the first time through:
                if ( !image.valid() )
                {
                    rstore->getOptimalImageSize( aoi, max_pixel_span, true, /*out*/image_width, /*out*/image_height );
                    image = new osg::Image();
                    image->allocateImage( image_width, image_height, 1, pixel_format, GL_UNSIGNED_BYTE );
                }

                // calculate the overlap window as GeoExtent and pixels
                GeoExtent window_aoi = aoi.getIntersection( part_extent );
                float wr = window_aoi.getWidth()/aoi.getWidth(), wo = (window_aoi.getXMin()-aoi.getXMin())/aoi.getWidth();
                float hr = window_aoi.getHeight()/aoi.getHeight(), ho = (window_aoi.getYMin()-aoi.getYMin())/aoi.getHeight();
                unsigned int window_s = osg::minimum( (unsigned int)(wr * (float)image_width), image_width );
                unsigned int window_t = osg::minimum( (unsigned int)(hr * (float)image_height), image_height );
                unsigned int s_offset = osg::minimum( (unsigned int)(wo * image_width), image_width );
                unsigned int t_offset = osg::minimum( (unsigned int)(ho * image_height), image_height );

                // fetch the sub-image and copy it into the destination:
                osg::ref_ptr<osg::Image> part_image = rstore->createImage( window_aoi, window_s, window_t );
                if ( part_image.valid() )
                {
                    if ( ! ImageUtils::copyAsSubImage( part_image.get(), image.get(), s_offset, t_offset ) )
                    {
                        osgGIS::notify( osg::NOTICE ) 
                            << "***ERROR: ImageUtils::copyAsSubImage failed:" << std::endl
                            << "   image: s=" << image->s() << ", t=" << image->t() << std::endl
                            << "   part:  s=" << part_image->s() << ", t=" << part_image->t() << std::endl
                            << "   window_w=" << window_s << ", window_h=" << window_t << ", s_offset=" << s_offset << ", t_offset=" << t_offset << std::endl
                            << "   image_ex=" << aoi.toString() << std::endl
                            << "   part_ex =" << part_extent.toString() << std::endl
                            << "   isect_ex=" << window_aoi.toString() << std::endl
                            << std::endl;
                    }

                    // cannot use this b/c it requires a gl context :(
                    //image->copySubImage( s_offset, t_offset, 0, part_image.get() );
                }
            }
        }
    }

    if ( image.valid() )
    {
        osg::Texture* tex = new osg::Texture2D( image.get() );
        tex->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE );
        tex->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE );
        tex->setResizeNonPowerOfTwoHint( false );

        osg::TexEnv* texenv = new osg::TexEnv();
        texenv = new osg::TexEnv();
        texenv->setMode( getTextureMode() );

        state_set->setTextureAttributeAndModes( 0, tex, osg::StateAttribute::ON );
        state_set->setTextureAttribute( 0, texenv, osg::StateAttribute::ON );

        if ( out_image )
        {
            *out_image = image.get();
        }

        result = true;
    }

    return result;
}

void
RasterResource::initParts()
{
    osgGIS::notify(osg::NOTICE) << "Raster resource \"" << getName() << "\":" << std::endl;

    for( RasterParts::iterator i = parts.begin(); i != parts.end(); i++ )
    {
        std::string part_auri = PathUtils::getAbsPath( getBaseURI(), i->second );
        osg::ref_ptr<RasterStore> rstore = Registry::instance()->getRasterStoreFactory()->connectToRasterStore( part_auri );
        if ( rstore.valid() )
        {
            i->first = rstore->getExtent();
            pixel_format = rstore->getImagePixelFormat();
            osgGIS::notify( osg::NOTICE ) << "   " << part_auri << " -- " << i->first.toString() << std::endl;
        }      
        else
        {   
            osgGIS::notify( osg::WARN ) << "   WARNING: " << part_auri << " -- cannot connect to raster store" << std::endl;
        }
    }

    parts_initialized = true;
}
