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

#include <osgGIS/GDAL_RasterStore>
#include <osgGIS/Registry>
#include <osgGIS/OGR_SpatialReference>
#include <osgGIS/OGR_Utils>
#include <osgGIS/Utils>
#include <osgDB/FileNameUtils>
#include <osgDB/WriteFile>
#include <osg/Notify>


using namespace osgGIS;


// opening an existing feature store.
GDAL_RasterStore::GDAL_RasterStore( const std::string& abs_path )
: extent( GeoExtent::invalid() )
{
    OGR_SCOPE_LOCK();
    uri = abs_path;

    if ( StringUtils::endsWith( uri, ".ecw" ) )
        osgGIS::notify(osg::WARN) << "WARNING: ECW files cannot report SRS...consider creating a VRT" << std::endl;
    
    dataset = (GDALDataset*) GDALOpen( uri.c_str(), GA_ReadOnly );
    if ( dataset )
    {
        has_geo_transform = dataset->GetGeoTransform( geo_transform ) == CE_None;
        num_bands = dataset->GetRasterCount();

        size_x = dataset->GetRasterXSize();
        size_y = dataset->GetRasterYSize();

        //osgGIS::notify(osg::NOTICE)
        //    << "Opened GDAL Raster at " << uri << std::endl 
        //    << "   Driver = " << dataset->GetDriver()->GetDescription() << std::endl
        //    << "   Size = " << size_x << " x " << size_y << std::endl
        //    << "   Bands = " << num_bands << std::endl
        //    << "   Proj = " << dataset->GetProjectionRef() << std::endl;

        if ( has_geo_transform )
        {
            //osgGIS::notify(osg::NOTICE)
            //    << "   Origin = " << geo_transform[0] << ", " << geo_transform[3] << std::endl
            //    << "   Pixel size = " << geo_transform[1] << ", " << geo_transform[5] << std::endl
            //    << "   Extent = " << getExtent().toString() << std::endl;
        }
        else
        {
            geo_transform[0] = 0.0; geo_transform[3] = 0.0;
            geo_transform[1] = 1.0; geo_transform[5] = 1.0;
            //TODO: rotation
        }
    }
    else
    {
        osgGIS::notify(osg::WARN) << "Failed to open GDAL raster store at " << uri << std::endl;
    }
}


GDAL_RasterStore::~GDAL_RasterStore()
{
    OGR_SCOPE_LOCK();

	if ( dataset )
	{
        delete dataset;
		dataset = NULL;
	}

    //osgGIS::notify(osg::NOTICE) << "Closed GDAL raster store at " << getName() << std::endl;
}


const std::string&
GDAL_RasterStore::getName() const
{
    return uri;
}


SpatialReference*
GDAL_RasterStore::getSRS() const
{
	if ( !spatial_ref.get() )
	{
        OGR_SCOPE_LOCK();

        SpatialReference* result = NULL;

        const char* wkt = dataset->GetProjectionRef();
        if ( wkt )
        {
            result = Registry::SRSFactory()->createSRSfromWKT( wkt );
        }
		const_cast<GDAL_RasterStore*>(this)->spatial_ref = result;
	}
	return spatial_ref.get();
}


bool
GDAL_RasterStore::isReady() const
{
	return dataset != NULL;
}


const GeoExtent&
GDAL_RasterStore::getExtent() const
{
    if ( !extent.isValid() )
    {
        (const_cast<GDAL_RasterStore*>(this))->calcExtent();
    }
    return extent;
}

void
GDAL_RasterStore::calcExtent()
{
    OGR_SCOPE_LOCK();

    // resolution values might be <0. For example, if res_y<0, then the Y offset is the north 
    // coordinate instead of the south (it's a top-down raster).
    res_x = geo_transform[1], res_y = geo_transform[5];

    double xmin = res_x > 0.0? geo_transform[0] : geo_transform[0] + size_x * res_x;
    double xmax = res_x > 0.0? xmin + size_x * res_x : geo_transform[0];
    double ymin = res_y > 0.0? geo_transform[3] : geo_transform[3] + size_y * res_y;
    double ymax = res_y > 0.0? ymin + size_y * res_y : geo_transform[3];

    extent = GeoExtent( xmin, ymin, xmax, ymax, getSRS() );
}


GLenum
GDAL_RasterStore::getImagePixelFormat() const
{
    return num_bands >= 4? GL_RGBA : GL_RGB;
}


bool 
GDAL_RasterStore::getOptimalImageSize(const GeoExtent& input_aoi,
                                      unsigned int     max_span_pixels,
                                      bool             force_power_of_2_dimensions,
                                      unsigned int&    out_image_width,
                                      unsigned int&    out_image_height ) const
{
    // transform the requested AOI into the local SRS:
    GeoExtent output_aoi = getSRS()->transform( input_aoi );

    // calculate the optimum width/height of the output image:
    if ( max_span_pixels <= 0 )
        max_span_pixels = 1024; //TODO: configurable to max supported texture size

    if ( force_power_of_2_dimensions )
        max_span_pixels = ImageUtils::roundToNearestPowerOf2( max_span_pixels );

    double aspect_ratio = output_aoi.getWidth()/output_aoi.getHeight();
    unsigned int max_pixels_x = (unsigned int)( output_aoi.getWidth() / osg::absolute( geo_transform[1] ) ); // gt[1] == x-resolution
    unsigned int max_pixels_y = (unsigned int)( output_aoi.getHeight() / osg::absolute( geo_transform[5] ) ); // gt[5] = y-resolution

    if ( aspect_ratio >= 1.0 )
    {
        out_image_width = osg::minimum( max_pixels_x, max_span_pixels );
        out_image_height = (unsigned int)ceilf( out_image_width / aspect_ratio );
    }
    else
    {
        out_image_height = osg::minimum( max_pixels_y, max_span_pixels );
        out_image_width = (unsigned int)ceilf( out_image_height * aspect_ratio );
    }
    out_image_width  = osg::maximum( (unsigned int)1, out_image_width );
    out_image_height = osg::maximum( (unsigned int)1, out_image_height );

    // round to nearest power of 2 in each dimension:
    if ( force_power_of_2_dimensions )
    {
        out_image_width  = ImageUtils::roundUpToPowerOf2( out_image_width, max_span_pixels );
        out_image_height = ImageUtils::roundUpToPowerOf2( out_image_height, max_span_pixels );
    }

    return true;
}


osg::Image* 
GDAL_RasterStore::createImage(const GeoExtent& requested_aoi,
                              unsigned int     image_width,
                              unsigned int     image_height ) const
{
    if ( !getSRS() )
    {
        osgGIS::notify(osg::FATAL) << "GDAL_RasterStore: no SRS, getImage() failed" << std::endl;
        return NULL;
    }

    // transform the requested AOI into the local SRS:
    GeoExtent output_aoi = getSRS()->transform( requested_aoi );

    // make sure the requested extent is within the image extent:
    if ( !getExtent().contains( output_aoi ) )
    {
        osgGIS::notify(osg::WARN) 
            << "GDAL_RasterStore: requested AOI is out of bounds" << std::endl
            << "    asked for " << output_aoi.toString() << ", but raster is " << getExtent().toString()
            << std::endl;
            
        return NULL;
    }

    if ( !output_aoi.isValid() )
    {
        osgGIS::notify(osg::WARN) << "GDAL_RasterSource::getImage failed, cannot reproject requested AOI" << std::endl;
        return NULL;
    }

    osg::Image* image = new osg::Image();
    GLenum pixel_format = num_bands >= 4? GL_RGBA : GL_RGB;
    image->allocateImage( image_width, image_height, 1, pixel_format, GL_UNSIGNED_BYTE );

    const GeoExtent& s_bb = extent;
    const GeoExtent& d_bb = output_aoi;

    // note, we have to handle the possibility of goegraphic datasets wrapping over on themselves when they pass over the dateline
    // to do this we have to test geographic datasets via two passes, each with a 360 degree shift of the source cata.
    double xoffset = d_bb.getXMin() < s_bb.getXMin() && d_bb.getSRS()->isGeographic()? -360.0 : 0.0;
    unsigned int numXChecks = d_bb.getSRS()->isGeographic()? 2 : 1;

    //TODO: move this so that it wraps the GDAL calls more tightly
    OGR_SCOPE_LOCK();

    for( unsigned int ic=0; ic < numXChecks; ++ic, xoffset += 360.0 )
    {
        GeoExtent intersect_bb(
            osg::maximum( d_bb.getXMin(), s_bb.getXMin() + xoffset ),
            osg::maximum( d_bb.getYMin(), s_bb.getYMin() ),
            osg::minimum( d_bb.getXMax(), s_bb.getXMax() + xoffset ),
            osg::minimum( d_bb.getYMax(), s_bb.getYMax() ),
            d_bb.getSRS() );

        if ( !intersect_bb.isValid() )
        {
            continue;
        }


        int windowX = osg::maximum((int)floorf((float)size_x*(intersect_bb.getXMin()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),0);
        int windowY = osg::maximum((int)floorf((float)size_y*(intersect_bb.getYMin()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),0);
        int windowWidth = osg::minimum((int)ceilf((float)size_x*(intersect_bb.getXMax()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),(int)size_x)-windowX;
        int windowHeight = osg::minimum((int)ceilf((float)size_y*(intersect_bb.getYMax()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),(int)size_y)-windowY;

        int destX = osg::maximum((int)floorf((float)image->s()*(intersect_bb.getXMin()-d_bb.getXMin())/(d_bb.getXMax()-d_bb.getXMin())),0);
        int destY = osg::maximum((int)floorf((float)image->t()*(intersect_bb.getYMin()-d_bb.getYMin())/(d_bb.getYMax()-d_bb.getYMin())),0);
        int destWidth = osg::minimum((int)ceilf((float)image->s()*(intersect_bb.getXMax()-d_bb.getXMin())/(d_bb.getXMax()-d_bb.getXMin())),(int)image->s())-destX;
        int destHeight = osg::minimum((int)ceilf((float)image->t()*(intersect_bb.getYMax()-d_bb.getYMin())/(d_bb.getYMax()-d_bb.getYMin())),(int)image->t())-destY;

        //log(osg::INFO,"   copying from %d\t%d\t%d\t%d",windowX,windowY,windowWidth,windowHeight);
        //log(osg::INFO,"             to %d\t%d\t%d\t%d",destX,destY,destWidth,destHeight);

        int readWidth = destWidth;
        int readHeight = destHeight;
        bool doResample = false;

        float destWindowWidthRatio = (float)destWidth/(float)windowWidth;
        float destWindowHeightRatio = (float)destHeight/(float)windowHeight;
        const float resizeTolerance = 1.1;

        bool interpolateSourceImagery = true; // destination._dataSet->getUseInterpolatedImagerySampling();

        
        if (interpolateSourceImagery && 
            (destWindowWidthRatio>resizeTolerance || destWindowHeightRatio>resizeTolerance) &&
            windowWidth>=2 && windowHeight>=2)
        {
            readWidth = windowWidth;
            readHeight = windowHeight;
            doResample = true;
        }

        bool hasRGB        = num_bands >= 3;
        bool hasAlpha      = num_bands >= 4;
        bool hasColorTable = num_bands >= 1 && dataset->GetRasterBand(1)->GetColorTable();
        bool hasGreyScale  = num_bands == 1;
        unsigned int numSourceComponents = hasAlpha?4:3;

        if (hasRGB || hasColorTable || hasGreyScale)
        {
            // RGB

            unsigned int numBytesPerPixel = 1;
            GDALDataType targetGDALType = GDT_Byte;

            int pixelSpace = numSourceComponents * numBytesPerPixel;

            //log(osg::INFO,"reading RGB");

            unsigned char* tempImage = new unsigned char[readWidth*readHeight*pixelSpace];


            /* New code courtesy of Frank Warmerdam of the GDAL group */

            // RGB images ... or at least we assume 3+ band images can be treated 
            // as RGB. 
            if( hasRGB ) 
            { 
                GDALRasterBand* bandRed   = dataset->GetRasterBand(1); 
                GDALRasterBand* bandGreen = dataset->GetRasterBand(2); 
                GDALRasterBand* bandBlue  = dataset->GetRasterBand(3); 
                GDALRasterBand* bandAlpha = hasAlpha ? dataset->GetRasterBand(4) : 0; 

                bandRed->RasterIO(GF_Read, 
                                  windowX,size_y-(windowY+windowHeight), 
                                  windowWidth,windowHeight, 
                                  (void*)(tempImage+0),readWidth,readHeight, 
                                  targetGDALType,pixelSpace,pixelSpace*readWidth); 
                bandGreen->RasterIO(GF_Read, 
                                    windowX,size_y-(windowY+windowHeight), 
                                    windowWidth,windowHeight, 
                                    (void*)(tempImage+1),readWidth,readHeight, 
                                    targetGDALType,pixelSpace,pixelSpace*readWidth); 
                bandBlue->RasterIO(GF_Read, 
                                   windowX,size_y-(windowY+windowHeight), 
                                   windowWidth,windowHeight, 
                                   (void*)(tempImage+2),readWidth,readHeight, 
                                   targetGDALType,pixelSpace,pixelSpace*readWidth); 

                if (bandAlpha)
                {
                    bandAlpha->RasterIO(GF_Read, 
                                       windowX,size_y-(windowY+windowHeight), 
                                       windowWidth,windowHeight, 
                                       (void*)(tempImage+3),readWidth,readHeight, 
                                       targetGDALType,pixelSpace,pixelSpace*readWidth); 
                }
            } 

            else if( hasColorTable ) 
            { 
                // Pseudocolored image.  Convert 1 band + color table to 24bit RGB. 

                GDALRasterBand *band; 
                GDALColorTable *ct; 
                int i; 


                band = dataset->GetRasterBand(1); 


                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+0),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 


                ct = band->GetColorTable(); 


                for( i = 0; i < readWidth * readHeight; i++ ) 
                { 
                    GDALColorEntry sEntry; 


                    // default to greyscale equilvelent. 
                    sEntry.c1 = tempImage[i*3]; 
                    sEntry.c2 = tempImage[i*3]; 
                    sEntry.c3 = tempImage[i*3]; 


                    ct->GetColorEntryAsRGB( tempImage[i*3], &sEntry ); 


                    // Apply RGB back over destination image. 
                    tempImage[i*3 + 0] = sEntry.c1; 
                    tempImage[i*3 + 1] = sEntry.c2; 
                    tempImage[i*3 + 2] = sEntry.c3; 
                } 
            } 


            else if (hasGreyScale)
            { 
                // Greyscale image.  Convert 1 band to 24bit RGB. 
                GDALRasterBand *band; 


                band = dataset->GetRasterBand(1); 


                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+0),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 
                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+1),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 
                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+2),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 
            } 

            if (doResample || readWidth!=destWidth || readHeight!=destHeight)
            {
                unsigned char* destImage = new unsigned char[destWidth*destHeight*pixelSpace];

                // rescale image by hand as glu seem buggy....
                for(int j=0;j<destHeight;++j)
                {
                    float  t_d = (destHeight>1)?((float)j/((float)destHeight-1)):0;
                    for(int i=0;i<destWidth;++i)
                    {
                        float s_d = (destWidth>1)?((float)i/((float)destWidth-1)):0;

                        float flt_read_i = s_d * ((float)readWidth-1);
                        float flt_read_j = t_d * ((float)readHeight-1);

                        int read_i = (int)flt_read_i;
                        if (read_i>=readWidth) read_i=readWidth-1;

                        float flt_read_ir = flt_read_i-read_i;
                        if (read_i==readWidth-1) flt_read_ir=0.0f;

                        int read_j = (int)flt_read_j;
                        if (read_j>=readHeight) read_j=readHeight-1;

                        float flt_read_jr = flt_read_j-read_j;
                        if (read_j==readHeight-1) flt_read_jr=0.0f;

                        unsigned char* dest = destImage + (j*destWidth + i) * pixelSpace;
                        if (flt_read_ir==0.0f)  // no need to interpolate i axis.
                        {
                            if (flt_read_jr==0.0f)  // no need to interpolate j axis.
                            {
                                // copy pixels
                                unsigned char* src = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                dest[0] = src[0];
                                dest[1] = src[1];
                                dest[2] = src[2];
                                if (numSourceComponents==4) dest[3] = src[3];
                                //std::cout<<"copy");
                            }
                            else  // need to interpolate j axis.
                            {
                                // copy pixels
                                unsigned char* src_0 = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                unsigned char* src_1 = src_0 + readWidth*pixelSpace;
                                float r_0 = 1.0f-flt_read_jr;
                                float r_1 = flt_read_jr;
                                dest[0] = (unsigned char)((float)src_0[0]*r_0 + (float)src_1[0]*r_1);
                                dest[1] = (unsigned char)((float)src_0[1]*r_0 + (float)src_1[1]*r_1);
                                dest[2] = (unsigned char)((float)src_0[2]*r_0 + (float)src_1[2]*r_1);
                                if (numSourceComponents==4) dest[3] = (unsigned char)((float)src_0[3]*r_0 + (float)src_1[3]*r_1);
                                //std::cout<<"interpolate j axis");
                            }
                        }
                        else // need to interpolate i axis.
                        {
                            if (flt_read_jr==0.0f) // no need to interpolate j axis.
                            {
                                // copy pixels
                                unsigned char* src_0 = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                unsigned char* src_1 = src_0 + pixelSpace;
                                float r_0 = 1.0f-flt_read_ir;
                                float r_1 = flt_read_ir;
                                dest[0] = (unsigned char)((float)src_0[0]*r_0 + (float)src_1[0]*r_1);
                                dest[1] = (unsigned char)((float)src_0[1]*r_0 + (float)src_1[1]*r_1);
                                dest[2] = (unsigned char)((float)src_0[2]*r_0 + (float)src_1[2]*r_1);
                                if (numSourceComponents==4) dest[3] = (unsigned char)((float)src_0[3]*r_0 + (float)src_1[3]*r_1);
                                //std::cout<<"interpolate i axis");
                            }
                            else  // need to interpolate i and j axis.
                            {
                                unsigned char* src_0 = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                unsigned char* src_1 = src_0 + readWidth*pixelSpace;
                                unsigned char* src_2 = src_0 + pixelSpace;
                                unsigned char* src_3 = src_1 + pixelSpace;
                                float r_0 = (1.0f-flt_read_ir)*(1.0f-flt_read_jr);
                                float r_1 = (1.0f-flt_read_ir)*flt_read_jr;
                                float r_2 = (flt_read_ir)*(1.0f-flt_read_jr);
                                float r_3 = (flt_read_ir)*flt_read_jr;
                                dest[0] = (unsigned char)(((float)src_0[0])*r_0 + ((float)src_1[0])*r_1 + ((float)src_2[0])*r_2 + ((float)src_3[0])*r_3);
                                dest[1] = (unsigned char)(((float)src_0[1])*r_0 + ((float)src_1[1])*r_1 + ((float)src_2[1])*r_2 + ((float)src_3[1])*r_3);
                                dest[2] = (unsigned char)(((float)src_0[2])*r_0 + ((float)src_1[2])*r_1 + ((float)src_2[2])*r_2 + ((float)src_3[2])*r_3);
                                if (numSourceComponents==4) dest[3] = (unsigned char)(((float)src_0[3])*r_0 + ((float)src_1[3])*r_1 + ((float)src_2[3])*r_2 + ((float)src_3[3])*r_3);
                                //std::cout<<"interpolate i & j axis");
                            }
                        }

                    }
                }

                delete [] tempImage;  
                tempImage = destImage;
            }

            // now copy into destination image
            unsigned char* sourceRowPtr = tempImage;
            int sourceRowDelta = pixelSpace*destWidth;
            unsigned char* destinationRowPtr = image->data(destX,destY+destHeight-1);
            int destinationRowDelta = -(int)(image->getRowSizeInBytes());
            int destination_pixelSpace = image->getPixelSizeInBits()/8;
            bool destination_hasAlpha = osg::Image::computeNumComponents(image->getPixelFormat())==4;

            // copy image to destination image
            for(int row=0;
                row<destHeight;
                ++row, sourceRowPtr+=sourceRowDelta, destinationRowPtr+=destinationRowDelta)
            {
                unsigned char* sourceColumnPtr = sourceRowPtr;
                unsigned char* destinationColumnPtr = destinationRowPtr;

                for(int col=0;
                    col<destWidth;
                    ++col, sourceColumnPtr+=pixelSpace, destinationColumnPtr+=destination_pixelSpace)
                {
                    if (hasAlpha)
                    {
                        // only copy over source pixel if its alpha value is not 0
                        if (sourceColumnPtr[3]!=0)
                        {
                            if (sourceColumnPtr[3]==255)
                            {
                                // source alpha is full on so directly copy over.
                                destinationColumnPtr[0] = sourceColumnPtr[0];
                                destinationColumnPtr[1] = sourceColumnPtr[1];
                                destinationColumnPtr[2] = sourceColumnPtr[2];

                                if (destination_hasAlpha)
                                    destinationColumnPtr[3] = sourceColumnPtr[3];
                            }
                            else
                            {
                                // source value isn't full on so blend it with destination 
                                float rs = (float)sourceColumnPtr[3]/255.0f; 
                                float rd = 1.0f-rs;

                                destinationColumnPtr[0] = (int)(rd * (float)destinationColumnPtr[0] + rs * (float)sourceColumnPtr[0]);
                                destinationColumnPtr[1] = (int)(rd * (float)destinationColumnPtr[1] + rs * (float)sourceColumnPtr[1]);
                                destinationColumnPtr[2] = (int)(rd * (float)destinationColumnPtr[2] + rs * (float)sourceColumnPtr[2]);

                                if (destination_hasAlpha)
                                    destinationColumnPtr[3] = osg::maximum(destinationColumnPtr[3],sourceColumnPtr[3]);

                            }
                        }
                    }
                    else if (sourceColumnPtr[0]!=0 || sourceColumnPtr[1]!=0 || sourceColumnPtr[2]!=0)
                    {
                        destinationColumnPtr[0] = sourceColumnPtr[0];
                        destinationColumnPtr[1] = sourceColumnPtr[1];
                        destinationColumnPtr[2] = sourceColumnPtr[2];
                    }
                }
            }

            delete [] tempImage;

        }
        else
        {
            //log(osg::INFO,"Warning image not read as Red, Blue and Green bands not present");
        }
    }

    //test: write the dumb image:
    //osgDB::writeImageFile( *image, "f:/temp/hi.jpg" );

    return image;
}


// the bulk of code in this method is adapted from VPB::SourceData::readImage()
// written by Robert Osfield with contributions from Frank Warmerdam
osg::Image*
GDAL_RasterStore::createImage(const GeoExtent& requested_aoi,
                              int max_span_pixels,
                              bool force_power_of_2_dimensions ) const
{
    if ( !getSRS() )
    {
        osgGIS::notify(osg::FATAL) << "GDAL_RasterStore: no SRS, getImage() failed" << std::endl;
        return NULL;
    }

    // transform the requested AOI into the local SRS:
    GeoExtent output_aoi(
        getSRS()->transform( requested_aoi.getSouthwest() ),
        getSRS()->transform( requested_aoi.getNortheast() ) );

    // make sure the requested extent is within the image extent:
    if ( !getExtent().contains( output_aoi ) )
    {
        osgGIS::notify(osg::WARN) 
            << "GDAL_RasterStore: requested AOI is out of bounds" << std::endl
            << "    asked for " << output_aoi.toString() << ", but raster is " << getExtent().toString()
            << std::endl;
            
        return NULL;
    }

    // calculate the optimum width/height of the output image:
    if ( max_span_pixels <= 0 )
        max_span_pixels = 1024; //TODO: configurable to max supported texture size

    if ( force_power_of_2_dimensions )
        max_span_pixels = ImageUtils::roundToNearestPowerOf2( max_span_pixels );


    double aspect_ratio = output_aoi.getWidth()/output_aoi.getHeight();
    int max_pixels_x = (int)( output_aoi.getWidth() / osg::absolute( geo_transform[1] ) ); // gt[1] == x-resolution
    int max_pixels_y = (int)( output_aoi.getHeight() / osg::absolute( geo_transform[5] ) ); // gt[5] = y-resolution

    int image_width, image_height;
    if ( aspect_ratio >= 1.0 )
    {
        image_width = osg::minimum( max_pixels_x, max_span_pixels );
        image_height = (int)ceilf( image_width / aspect_ratio );
    }
    else
    {
        image_height = osg::minimum( max_pixels_y, max_span_pixels );
        image_width = (int)ceilf( image_height * aspect_ratio );
    }
    image_width = osg::maximum( 1, image_width );
    image_height = osg::maximum( 1, image_height );

    // round to nearest power of 2 in each dimension:
    if ( force_power_of_2_dimensions )
    {
        image_width = ImageUtils::roundUpToPowerOf2( image_width, max_span_pixels );
        image_height = ImageUtils::roundUpToPowerOf2( image_height, max_span_pixels );
    }

    if ( !output_aoi.isValid() )
    {
        osgGIS::notify(osg::WARN) << "GDAL_RasterSource::getImage failed, cannot reproject requested AOI" << std::endl;
        return NULL;
    }

    osg::Image* image = new osg::Image();
    GLenum pixel_format = num_bands >= 4? GL_RGBA : GL_RGB;
    image->allocateImage( image_width, image_height, 1, pixel_format, GL_UNSIGNED_BYTE );

    const GeoExtent& s_bb = extent;
    const GeoExtent& d_bb = output_aoi;

    // note, we have to handle the possibility of goegraphic datasets wrapping over on themselves when they pass over the dateline
    // to do this we have to test geographic datasets via two passes, each with a 360 degree shift of the source cata.
    double xoffset = d_bb.getXMin() < s_bb.getXMin() && d_bb.getSRS()->isGeographic()? -360.0 : 0.0;
    unsigned int numXChecks = d_bb.getSRS()->isGeographic()? 2 : 1;

    //TODO: move this so that it wraps the GDAL calls more tightly
    OGR_SCOPE_LOCK();

    for( unsigned int ic=0; ic < numXChecks; ++ic, xoffset += 360.0 )
    {
        GeoExtent intersect_bb(
            osg::maximum( d_bb.getXMin(), s_bb.getXMin() + xoffset ),
            osg::maximum( d_bb.getYMin(), s_bb.getYMin() ),
            osg::minimum( d_bb.getXMax(), s_bb.getXMax() + xoffset ),
            osg::minimum( d_bb.getYMax(), s_bb.getYMax() ),
            d_bb.getSRS() );

        if ( !intersect_bb.isValid() )
        {
            continue;
        }


        int windowX = osg::maximum((int)floorf((float)size_x*(intersect_bb.getXMin()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),0);
        int windowY = osg::maximum((int)floorf((float)size_y*(intersect_bb.getYMin()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),0);
        int windowWidth = osg::minimum((int)ceilf((float)size_x*(intersect_bb.getXMax()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),(int)size_x)-windowX;
        int windowHeight = osg::minimum((int)ceilf((float)size_y*(intersect_bb.getYMax()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),(int)size_y)-windowY;

        int destX = osg::maximum((int)floorf((float)image->s()*(intersect_bb.getXMin()-d_bb.getXMin())/(d_bb.getXMax()-d_bb.getXMin())),0);
        int destY = osg::maximum((int)floorf((float)image->t()*(intersect_bb.getYMin()-d_bb.getYMin())/(d_bb.getYMax()-d_bb.getYMin())),0);
        int destWidth = osg::minimum((int)ceilf((float)image->s()*(intersect_bb.getXMax()-d_bb.getXMin())/(d_bb.getXMax()-d_bb.getXMin())),(int)image->s())-destX;
        int destHeight = osg::minimum((int)ceilf((float)image->t()*(intersect_bb.getYMax()-d_bb.getYMin())/(d_bb.getYMax()-d_bb.getYMin())),(int)image->t())-destY;

        //log(osg::INFO,"   copying from %d\t%d\t%d\t%d",windowX,windowY,windowWidth,windowHeight);
        //log(osg::INFO,"             to %d\t%d\t%d\t%d",destX,destY,destWidth,destHeight);

        int readWidth = destWidth;
        int readHeight = destHeight;
        bool doResample = false;

        float destWindowWidthRatio = (float)destWidth/(float)windowWidth;
        float destWindowHeightRatio = (float)destHeight/(float)windowHeight;
        const float resizeTolerance = 1.1;

        bool interpolateSourceImagery = true; // destination._dataSet->getUseInterpolatedImagerySampling();

        
        if (interpolateSourceImagery && 
            (destWindowWidthRatio>resizeTolerance || destWindowHeightRatio>resizeTolerance) &&
            windowWidth>=2 && windowHeight>=2)
        {
            readWidth = windowWidth;
            readHeight = windowHeight;
            doResample = true;
        }

        bool hasRGB        = num_bands >= 3;
        bool hasAlpha      = num_bands >= 4;
        bool hasColorTable = num_bands >= 1 && dataset->GetRasterBand(1)->GetColorTable();
        bool hasGreyScale  = num_bands == 1;
        unsigned int numSourceComponents = hasAlpha?4:3;

        if (hasRGB || hasColorTable || hasGreyScale)
        {
            // RGB

            unsigned int numBytesPerPixel = 1;
            GDALDataType targetGDALType = GDT_Byte;

            int pixelSpace = numSourceComponents * numBytesPerPixel;

            //log(osg::INFO,"reading RGB");

            unsigned char* tempImage = new unsigned char[readWidth*readHeight*pixelSpace];


            /* New code courtesy of Frank Warmerdam of the GDAL group */

            // RGB images ... or at least we assume 3+ band images can be treated 
            // as RGB. 
            if( hasRGB ) 
            { 
                GDALRasterBand* bandRed   = dataset->GetRasterBand(1); 
                GDALRasterBand* bandGreen = dataset->GetRasterBand(2); 
                GDALRasterBand* bandBlue  = dataset->GetRasterBand(3); 
                GDALRasterBand* bandAlpha = hasAlpha ? dataset->GetRasterBand(4) : 0; 

                bandRed->RasterIO(GF_Read, 
                                  windowX,size_y-(windowY+windowHeight), 
                                  windowWidth,windowHeight, 
                                  (void*)(tempImage+0),readWidth,readHeight, 
                                  targetGDALType,pixelSpace,pixelSpace*readWidth); 
                bandGreen->RasterIO(GF_Read, 
                                    windowX,size_y-(windowY+windowHeight), 
                                    windowWidth,windowHeight, 
                                    (void*)(tempImage+1),readWidth,readHeight, 
                                    targetGDALType,pixelSpace,pixelSpace*readWidth); 
                bandBlue->RasterIO(GF_Read, 
                                   windowX,size_y-(windowY+windowHeight), 
                                   windowWidth,windowHeight, 
                                   (void*)(tempImage+2),readWidth,readHeight, 
                                   targetGDALType,pixelSpace,pixelSpace*readWidth); 

                if (bandAlpha)
                {
                    bandAlpha->RasterIO(GF_Read, 
                                       windowX,size_y-(windowY+windowHeight), 
                                       windowWidth,windowHeight, 
                                       (void*)(tempImage+3),readWidth,readHeight, 
                                       targetGDALType,pixelSpace,pixelSpace*readWidth); 
                }
            } 

            else if( hasColorTable ) 
            { 
                // Pseudocolored image.  Convert 1 band + color table to 24bit RGB. 

                GDALRasterBand *band; 
                GDALColorTable *ct; 
                int i; 


                band = dataset->GetRasterBand(1); 


                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+0),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 


                ct = band->GetColorTable(); 


                for( i = 0; i < readWidth * readHeight; i++ ) 
                { 
                    GDALColorEntry sEntry; 


                    // default to greyscale equilvelent. 
                    sEntry.c1 = tempImage[i*3]; 
                    sEntry.c2 = tempImage[i*3]; 
                    sEntry.c3 = tempImage[i*3]; 


                    ct->GetColorEntryAsRGB( tempImage[i*3], &sEntry ); 


                    // Apply RGB back over destination image. 
                    tempImage[i*3 + 0] = sEntry.c1; 
                    tempImage[i*3 + 1] = sEntry.c2; 
                    tempImage[i*3 + 2] = sEntry.c3; 
                } 
            } 


            else if (hasGreyScale)
            { 
                // Greyscale image.  Convert 1 band to 24bit RGB. 
                GDALRasterBand *band; 


                band = dataset->GetRasterBand(1); 


                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+0),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 
                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+1),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 
                band->RasterIO(GF_Read, 
                               windowX,size_y-(windowY+windowHeight), 
                               windowWidth,windowHeight, 
                               (void*)(tempImage+2),readWidth,readHeight, 
                               targetGDALType,pixelSpace,pixelSpace*readWidth); 
            } 

            if (doResample || readWidth!=destWidth || readHeight!=destHeight)
            {
                unsigned char* destImage = new unsigned char[destWidth*destHeight*pixelSpace];

                // rescale image by hand as glu seem buggy....
                for(int j=0;j<destHeight;++j)
                {
                    float  t_d = (destHeight>1)?((float)j/((float)destHeight-1)):0;
                    for(int i=0;i<destWidth;++i)
                    {
                        float s_d = (destWidth>1)?((float)i/((float)destWidth-1)):0;

                        float flt_read_i = s_d * ((float)readWidth-1);
                        float flt_read_j = t_d * ((float)readHeight-1);

                        int read_i = (int)flt_read_i;
                        if (read_i>=readWidth) read_i=readWidth-1;

                        float flt_read_ir = flt_read_i-read_i;
                        if (read_i==readWidth-1) flt_read_ir=0.0f;

                        int read_j = (int)flt_read_j;
                        if (read_j>=readHeight) read_j=readHeight-1;

                        float flt_read_jr = flt_read_j-read_j;
                        if (read_j==readHeight-1) flt_read_jr=0.0f;

                        unsigned char* dest = destImage + (j*destWidth + i) * pixelSpace;
                        if (flt_read_ir==0.0f)  // no need to interpolate i axis.
                        {
                            if (flt_read_jr==0.0f)  // no need to interpolate j axis.
                            {
                                // copy pixels
                                unsigned char* src = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                dest[0] = src[0];
                                dest[1] = src[1];
                                dest[2] = src[2];
                                if (numSourceComponents==4) dest[3] = src[3];
                                //std::cout<<"copy");
                            }
                            else  // need to interpolate j axis.
                            {
                                // copy pixels
                                unsigned char* src_0 = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                unsigned char* src_1 = src_0 + readWidth*pixelSpace;
                                float r_0 = 1.0f-flt_read_jr;
                                float r_1 = flt_read_jr;
                                dest[0] = (unsigned char)((float)src_0[0]*r_0 + (float)src_1[0]*r_1);
                                dest[1] = (unsigned char)((float)src_0[1]*r_0 + (float)src_1[1]*r_1);
                                dest[2] = (unsigned char)((float)src_0[2]*r_0 + (float)src_1[2]*r_1);
                                if (numSourceComponents==4) dest[3] = (unsigned char)((float)src_0[3]*r_0 + (float)src_1[3]*r_1);
                                //std::cout<<"interpolate j axis");
                            }
                        }
                        else // need to interpolate i axis.
                        {
                            if (flt_read_jr==0.0f) // no need to interpolate j axis.
                            {
                                // copy pixels
                                unsigned char* src_0 = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                unsigned char* src_1 = src_0 + pixelSpace;
                                float r_0 = 1.0f-flt_read_ir;
                                float r_1 = flt_read_ir;
                                dest[0] = (unsigned char)((float)src_0[0]*r_0 + (float)src_1[0]*r_1);
                                dest[1] = (unsigned char)((float)src_0[1]*r_0 + (float)src_1[1]*r_1);
                                dest[2] = (unsigned char)((float)src_0[2]*r_0 + (float)src_1[2]*r_1);
                                if (numSourceComponents==4) dest[3] = (unsigned char)((float)src_0[3]*r_0 + (float)src_1[3]*r_1);
                                //std::cout<<"interpolate i axis");
                            }
                            else  // need to interpolate i and j axis.
                            {
                                unsigned char* src_0 = tempImage + (read_j*readWidth + read_i) * pixelSpace;
                                unsigned char* src_1 = src_0 + readWidth*pixelSpace;
                                unsigned char* src_2 = src_0 + pixelSpace;
                                unsigned char* src_3 = src_1 + pixelSpace;
                                float r_0 = (1.0f-flt_read_ir)*(1.0f-flt_read_jr);
                                float r_1 = (1.0f-flt_read_ir)*flt_read_jr;
                                float r_2 = (flt_read_ir)*(1.0f-flt_read_jr);
                                float r_3 = (flt_read_ir)*flt_read_jr;
                                dest[0] = (unsigned char)(((float)src_0[0])*r_0 + ((float)src_1[0])*r_1 + ((float)src_2[0])*r_2 + ((float)src_3[0])*r_3);
                                dest[1] = (unsigned char)(((float)src_0[1])*r_0 + ((float)src_1[1])*r_1 + ((float)src_2[1])*r_2 + ((float)src_3[1])*r_3);
                                dest[2] = (unsigned char)(((float)src_0[2])*r_0 + ((float)src_1[2])*r_1 + ((float)src_2[2])*r_2 + ((float)src_3[2])*r_3);
                                if (numSourceComponents==4) dest[3] = (unsigned char)(((float)src_0[3])*r_0 + ((float)src_1[3])*r_1 + ((float)src_2[3])*r_2 + ((float)src_3[3])*r_3);
                                //std::cout<<"interpolate i & j axis");
                            }
                        }

                    }
                }

                delete [] tempImage;  
                tempImage = destImage;
            }

            // now copy into destination image
            unsigned char* sourceRowPtr = tempImage;
            int sourceRowDelta = pixelSpace*destWidth;
            unsigned char* destinationRowPtr = image->data(destX,destY+destHeight-1);
            int destinationRowDelta = -(int)(image->getRowSizeInBytes());
            int destination_pixelSpace = image->getPixelSizeInBits()/8;
            bool destination_hasAlpha = osg::Image::computeNumComponents(image->getPixelFormat())==4;

            // copy image to destination image
            for(int row=0;
                row<destHeight;
                ++row, sourceRowPtr+=sourceRowDelta, destinationRowPtr+=destinationRowDelta)
            {
                unsigned char* sourceColumnPtr = sourceRowPtr;
                unsigned char* destinationColumnPtr = destinationRowPtr;

                for(int col=0;
                    col<destWidth;
                    ++col, sourceColumnPtr+=pixelSpace, destinationColumnPtr+=destination_pixelSpace)
                {
                    if (hasAlpha)
                    {
                        // only copy over source pixel if its alpha value is not 0
                        if (sourceColumnPtr[3]!=0)
                        {
                            if (sourceColumnPtr[3]==255)
                            {
                                // source alpha is full on so directly copy over.
                                destinationColumnPtr[0] = sourceColumnPtr[0];
                                destinationColumnPtr[1] = sourceColumnPtr[1];
                                destinationColumnPtr[2] = sourceColumnPtr[2];

                                if (destination_hasAlpha)
                                    destinationColumnPtr[3] = sourceColumnPtr[3];
                            }
                            else
                            {
                                // source value isn't full on so blend it with destination 
                                float rs = (float)sourceColumnPtr[3]/255.0f; 
                                float rd = 1.0f-rs;

                                destinationColumnPtr[0] = (int)(rd * (float)destinationColumnPtr[0] + rs * (float)sourceColumnPtr[0]);
                                destinationColumnPtr[1] = (int)(rd * (float)destinationColumnPtr[1] + rs * (float)sourceColumnPtr[1]);
                                destinationColumnPtr[2] = (int)(rd * (float)destinationColumnPtr[2] + rs * (float)sourceColumnPtr[2]);

                                if (destination_hasAlpha)
                                    destinationColumnPtr[3] = osg::maximum(destinationColumnPtr[3],sourceColumnPtr[3]);

                            }
                        }
                    }
                    else if (sourceColumnPtr[0]!=0 || sourceColumnPtr[1]!=0 || sourceColumnPtr[2]!=0)
                    {
                        destinationColumnPtr[0] = sourceColumnPtr[0];
                        destinationColumnPtr[1] = sourceColumnPtr[1];
                        destinationColumnPtr[2] = sourceColumnPtr[2];
                    }
                }
            }

            delete [] tempImage;

        }
        else
        {
            //log(osg::INFO,"Warning image not read as Red, Blue and Green bands not present");
        }
    }

    //test: write the dumb image:
    //osgDB::writeImageFile( *image, "f:/temp/hi.jpg" );

    return image;
}


struct ValidValueOperator
{
    ValidValueOperator(GDALRasterBand *band):
        defaultValue(0.0f),
        noDataValue(-32767.0f),
        minValue(-32000.0f),
        maxValue(FLT_MAX)
    {
        if (band)
        {
            int success = 0;
            float value = band->GetNoDataValue(&success);
            if (success)
            {
                noDataValue = value;
            }
        }
    }

    inline bool isNoDataValue(float value)
    {
        if (noDataValue==value) return true;
        if (value<minValue) return true;
        return (value>maxValue);
    }
    
    inline float getValidValue(float value)
    {
        if (isNoDataValue(value)) return value;
        else return defaultValue;
    }
    
    float defaultValue;
    float noDataValue;
    float minValue;
    float maxValue;
};


osg::HeightField*
GDAL_RasterStore::createHeightField(const GeoExtent& requested_aoi) const
{
    if ( !getSRS() )
    {
        osgGIS::notify(osg::FATAL) << "GDAL_RasterStore: no SRS, createHeightField() failed" << std::endl;
        return NULL;
    }

    // transform the requested AOI into the local SRS:
    GeoExtent output_aoi(
        getSRS()->transform( requested_aoi.getSouthwest() ),
        getSRS()->transform( requested_aoi.getNortheast() ) );

    // make sure the requested extent is within the image extent:
    if ( !getExtent().contains( output_aoi ) )
    {
        osgGIS::notify(osg::WARN) << "GDAL_RasterStore::createHeightField, requested AOI is out of bounds" << std::endl;
        return NULL;
    }

    // now expand the output AOI to clamp it to the source data resolution:
    double t;
    t = fmod( output_aoi.getXMin(), res_x );
    double new_xmin = output_aoi.getXMin() - t;
    t = fmod( output_aoi.getXMax(), res_x );
    double new_xmax = output_aoi.getXMax() + (t > 0.0 ? res_x-t : 0.0);
    t = fmod( output_aoi.getYMin(), res_y );
    double new_ymin = output_aoi.getYMin() - t;
    t = fmod( output_aoi.getYMax(), res_y );
    double new_ymax = output_aoi.getYMax() + (t > 0.0 ? res_y-t : 0.0);

    output_aoi = GeoExtent( new_xmin, new_ymin, new_xmax, new_ymax, output_aoi.getSRS() );

    const GeoExtent& s_bb = getExtent();
    const GeoExtent& d_bb = output_aoi;

    // allocate a new height field:
    osg::HeightField* hf = new osg::HeightField();
    int num_rows = (int)(output_aoi.getWidth()/res_x);
    int num_cols = (int)(output_aoi.getHeight()/res_y);
    hf->allocate( num_cols, num_rows );
    hf->setOrigin( osg::Vec3( output_aoi.getXMin(), output_aoi.getYMin(), 0.0f ) );
    hf->setXInterval( res_x );
    hf->setYInterval( res_y );

    // note, we have to handle the possibility of goegraphic datasets wrapping over on themselves when they pass over the dateline
    // to do this we have to test geographic datasets via two passes, each with a 360 degree shift of the source cata.
    double xoffset = d_bb.getXMin() < s_bb.getXMin() && d_bb.getSRS()->isGeographic()? -360.0 : 0.0;
    unsigned int numXChecks = d_bb.getSRS()->isGeographic()? 2 : 1;

    //TODO: move this so that it wraps the GDAL calls more tightly
    OGR_SCOPE_LOCK();

    for( unsigned int ic=0; ic < numXChecks; ++ic, xoffset += 360.0 )
    {
        GeoExtent intersect_bb(
            osg::maximum( d_bb.getXMin(), s_bb.getXMin() + xoffset ),
            osg::maximum( d_bb.getYMin(), s_bb.getYMin() ),
            osg::minimum( d_bb.getXMax(), s_bb.getXMax() + xoffset ),
            osg::minimum( d_bb.getYMax(), s_bb.getYMax() ),
            d_bb.getSRS() );

        if ( !intersect_bb.isValid() )
        {
            continue;
        }

        //int windowX = osg::maximum((int)floorf((float)size_x*(intersect_bb.getXMin()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),0);
        //int windowY = osg::maximum((int)floorf((float)size_y*(intersect_bb.getYMin()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),0);
        //int windowWidth = osg::minimum((int)ceilf((float)size_x*(intersect_bb.getXMax()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),(int)size_x)-windowX;
        //int windowHeight = osg::minimum((int)ceilf((float)size_y*(intersect_bb.getYMax()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),(int)size_y)-windowY;

        int destX = osg::maximum((int)floorf((float)num_cols*(intersect_bb.getXMin()-d_bb.getXMin())/(d_bb.getXMax()-d_bb.getXMin())),0);
        int destY = osg::maximum((int)floorf((float)num_rows*(intersect_bb.getYMin()-d_bb.getYMin())/(d_bb.getYMax()-d_bb.getYMin())),0);
        int destWidth = osg::minimum((int)ceilf((float)num_cols*(intersect_bb.getXMax()-d_bb.getXMin())/(d_bb.getXMax()-d_bb.getXMin())),(int)num_cols)-destX;
        int destHeight = osg::minimum((int)ceilf((float)num_rows*(intersect_bb.getYMax()-d_bb.getYMin())/(d_bb.getYMax()-d_bb.getYMin())),(int)num_rows)-destY;


        // which band do we want to read from...        
        //int numBands = _gdalDataset->GetRasterCount();
        GDALRasterBand* bandGray = 0;
        GDALRasterBand* bandRed = 0;
        GDALRasterBand* bandGreen = 0;
        GDALRasterBand* bandBlue = 0;
        GDALRasterBand* bandAlpha = 0;

        for(int b=1;b<=num_bands;++b)
        {
            GDALRasterBand* band = dataset->GetRasterBand(b);
            if (band->GetColorInterpretation()==GCI_GrayIndex) bandGray = band;
            else if (band->GetColorInterpretation()==GCI_RedBand) bandRed = band;
            else if (band->GetColorInterpretation()==GCI_GreenBand) bandGreen = band;
            else if (band->GetColorInterpretation()==GCI_BlueBand) bandBlue = band;
            else if (band->GetColorInterpretation()==GCI_AlphaBand) bandAlpha = band;
            else if (bandGray == 0) bandGray = band;
        }


        GDALRasterBand* bandSelected = 0;
        if (!bandSelected && bandGray) bandSelected = bandGray;
        else if (!bandSelected && bandAlpha) bandSelected = bandAlpha;
        else if (!bandSelected && bandRed) bandSelected = bandRed;
        else if (!bandSelected && bandGreen) bandSelected = bandGreen;
        else if (!bandSelected && bandBlue) bandSelected = bandBlue;

        if (bandSelected)
        {

            //if (bandSelected->GetUnitType()) log(osg::INFO, "bandSelected->GetUnitType()= %d",bandSelected->GetUnitType());
            //else log(osg::INFO, "bandSelected->GetUnitType()= null" );


            ValidValueOperator validValueOperator(bandSelected);

            int success = 0;
            float offset = bandSelected->GetOffset(&success);
            if (success)
            {
//                log(osg::INFO,"We have Offset = %f",offset);
            }
            else
            {
//                log(osg::INFO,"We have no Offset");
                offset = 0.0f;
            }

            float scale = bandSelected->GetScale(&success);
            //if (success)
            //{
            //    log(osg::INFO,"We have Scale from GDALRasterBand = %f",scale);
            //}

            float datasetScale = 1.0f;
            //float datasetScale = destination._dataSet->getVerticalScale();
//            log(osg::INFO,"We have Scale from DataSet = %f",datasetScale);

            scale *= datasetScale;
//            log(osg::INFO,"We have total scale = %f",scale);


//            log(osg::INFO,"********* getLinearUnits = %f",getLinearUnits(_cs.get()));

            // raad the data.
            //osg::HeightField* hf = destination._heightField.get();

            float noDataValueFill = 0.0f;
            bool ignoreNoDataValue = true;

            
            //bool interpolateTerrain = destination._dataSet->getUseInterpolatedTerrainSampling();

            //if (interpolateTerrain)
            //{
            //    //Sample terrain at each vert to increase accuracy of the terrain.
            //    int endX = destX + destWidth;
            //    int endY = destY + destHeight;

            //    double orig_X = hf->getOrigin().x();
            //    double orig_Y = hf->getOrigin().y();
            //    double delta_X = hf->getXInterval();
            //    double delta_Y = hf->getYInterval();

            //    for (int c = destX; c < endX; ++c)
            //    {
            //        double geoX = orig_X + (delta_X * (double)c);
            //        for (int r = destY; r < endY; ++r)
            //        {
            //            double geoY = orig_Y + (delta_Y * (double)r);
            //            float h = getInterpolatedValue(bandSelected, geoX-xoffset, geoY);
            //            if (!validValueOperator.isNoDataValue(h)) hf->setHeight(c,r,offset + h*scale);
            //            else if (!ignoreNoDataValue) hf->setHeight(c,r,noDataValueFill);
            //        }
            //    }
            //}
            //else
            {
                // compute dimensions to read from.        
               int windowX = osg::maximum((int)floorf((float)size_x*(intersect_bb.getXMin()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),0);
               int windowY = osg::maximum((int)floorf((float)size_y*(intersect_bb.getYMin()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),0);
               int windowWidth = osg::minimum((int)ceilf((float)size_x*(intersect_bb.getXMax()-xoffset-s_bb.getXMin())/(s_bb.getXMax()-s_bb.getXMin())),(int)size_x)-windowX;
               int windowHeight = osg::minimum((int)ceilf((float)size_y*(intersect_bb.getYMax()-s_bb.getYMin())/(s_bb.getYMax()-s_bb.getYMin())),(int)size_y)-windowY;

                //log(osg::INFO,"   copying from %d\t%s\t%d\t%d",windowX,windowY,windowWidth,windowHeight);
                //log(osg::INFO,"             to %d\t%s\t%d\t%d",destX,destY,destWidth,destHeight);

                // read data into temporary array
                float* heightData = new float [ destWidth*destHeight ];

                //bandSelected->RasterIO(GF_Read,windowX,_numValuesY-(windowY+windowHeight),windowWidth,windowHeight,floatdata,destWidth,destHeight,GDT_Float32,numBytesPerZvalue,lineSpace);
                bandSelected->RasterIO(GF_Read,windowX,size_y-(windowY+windowHeight),windowWidth,windowHeight,heightData,destWidth,destHeight,GDT_Float32,0,0);

                float* heightPtr = heightData;

                for(int r=destY+destHeight-1;r>=destY;--r)
                {
                    for(int c=destX;c<destX+destWidth;++c)
                    {
                        float h = *heightPtr++;
                        if (!validValueOperator.isNoDataValue(h)) hf->setHeight(c,r,offset + h*scale);
                        else if (!ignoreNoDataValue) hf->setHeight(c,r,noDataValueFill);

                        h = hf->getHeight(c,r);
                    }
                }

                delete [] heightData;
            }          
        }
    }

    return hf;
}

