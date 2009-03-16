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

#include <osgGIS/SkinResource>
#include <osgGIS/Utils>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/TexEnv>
#include <osg/BlendFunc>
#include <OpenThreads/ScopedLock>

using namespace osgGIS;
using namespace OpenThreads;

#include <osgGIS/Registry>
OSGGIS_DEFINE_RESOURCE(SkinResource);

static std::string EMPTY_STRING = "";

#define DEFAULT_TEXTURE_MODE osg::TexEnv::MODULATE
#define DEFAULT_MAX_TEXTURE_SIZE 0

osg::Image* createErrorImage()
{
    osg::Image* image = new osg::Image;

    image->allocateImage(32,32,1,GL_RGB, GL_UNSIGNED_BYTE);

    for (int c = 0; c < image->s(); ++c)
    {
        for (int r = 0; r < image->t(); ++r)
        {
            bool red = ((c == r) || ((image->s() - c) == r));
            unsigned char* data = image->data(c,r);
            *(data+0) = red ? 255 : 0;
            *(data+1) = 0;
            *(data+2) = 0;
        }
    }
    return image;
}


SkinResource::SkinResource()
{
    init();
}

SkinResource::SkinResource( const std::string& _name )
: Resource( _name )
{
    init();
}

void
SkinResource::init()
{
    setTextureWidthMeters( 3.0 );
    setTextureHeightMeters( 4.0 );
    setMinTextureHeightMeters( 0.0 );
    setMaxTextureHeightMeters( DBL_MAX );
    setColor( osg::Vec4( 1, 1, 1, 1 ) );
    setRepeatsVertically( true );
    setTextureMode( DEFAULT_TEXTURE_MODE );
    setMaxTextureSize( DEFAULT_MAX_TEXTURE_SIZE );
    setSingleUse( false );
}

SkinResource::~SkinResource()
{
    //NOP
}

void
SkinResource::setProperty( const Property& prop )
{
    if ( prop.getName() == "texture_width" )
        setTextureWidthMeters( prop.getDoubleValue( getTextureWidthMeters() ) );
    else if ( prop.getName() == "texture_height" )
        setTextureHeightMeters( prop.getDoubleValue( getTextureHeightMeters() ) );
    else if ( prop.getName() == "min_texture_height" )
        setMinTextureHeightMeters( prop.getDoubleValue( getMinTextureHeightMeters() ) );
    else if ( prop.getName() == "max_texture_height" )
        setMaxTextureHeightMeters( prop.getDoubleValue( getMaxTextureHeightMeters() ) );
    else if ( prop.getName() == "color" )
        setColor( prop.getVec4Value() );
    else if ( prop.getName() == "repeats_vertically" )
        setRepeatsVertically( prop.getBoolValue( getRepeatsVertically() ) );
    else if ( prop.getName() == "texture_mode" )
        setTextureMode( prop.getValue()=="decal"? osg::TexEnv::DECAL: prop.getValue()=="replace"? osg::TexEnv::REPLACE: prop.getValue()=="blend"? osg::TexEnv::BLEND : osg::TexEnv::MODULATE );
    else if ( prop.getName() == "texture_path" )
        setURI( prop.getValue() ); // for backwards compat... use <uri> instead
    else if ( prop.getName() == "max_texture_size" )
        setMaxTextureSize( prop.getIntValue( getMaxTextureSize() ) );
    else
        Resource::setProperty( prop );
}

Properties
SkinResource::getProperties() const
{
    Properties props = Resource::getProperties();
    props.push_back( Property( "texture_width", getTextureWidthMeters() ) );
    props.push_back( Property( "texture_height", getTextureHeightMeters() ) );
    props.push_back( Property( "min_texture_height", getMinTextureHeightMeters() ) );
    props.push_back( Property( "max_texture_height", getMaxTextureHeightMeters() ) );
    props.push_back( Property( "color", getColor() ) );
    props.push_back( Property( "repeats_vertically", getRepeatsVertically() ) );
    if ( getTextureMode() != DEFAULT_TEXTURE_MODE )
        props.push_back( Property( "texture_mode", getTextureMode()==osg::TexEnv::DECAL? "decal" : getTextureMode()==osg::TexEnv::REPLACE? "replace" : getTextureMode()==osg::TexEnv::BLEND? "blend" : "modulate" ) );
    if ( getMaxTextureSize() != DEFAULT_MAX_TEXTURE_SIZE )
        props.push_back( Property( "max_texture_size", (int)getMaxTextureSize() ) );
    return props;
}

void 
SkinResource::setTextureWidthMeters( double value )
{
    tex_width_m = value;
}

double 
SkinResource::getTextureWidthMeters() const
{
    return tex_width_m;
}

void 
SkinResource::setTextureHeightMeters( double value )
{
    tex_height_m = value;
}

double 
SkinResource::getTextureHeightMeters() const
{
    return tex_height_m;
}

void
SkinResource::setMinTextureHeightMeters( double value )
{
    min_tex_height_m = value;
}

double 
SkinResource::getMinTextureHeightMeters() const
{
    return min_tex_height_m;
}

void 
SkinResource::setMaxTextureHeightMeters( double value )
{
    max_tex_height_m = value;
}

double 
SkinResource::getMaxTextureHeightMeters() const
{
    return max_tex_height_m;
}

void
SkinResource::setMaxTextureSize( unsigned int value ) 
{
    max_texture_size = value;
}

unsigned int
SkinResource::getMaxTextureSize() const
{
    return max_texture_size;
}

void 
SkinResource::setRepeatsVertically( bool value )
{
    repeats_vertically = value;
}

bool 
SkinResource::getRepeatsVertically() const
{
    return repeats_vertically;
}

void
SkinResource::setTextureMode( const osg::TexEnv::Mode& value )
{
    texture_mode = value;
}

const osg::TexEnv::Mode&
SkinResource::getTextureMode() const
{
    return texture_mode;
}

void
SkinResource::setColor( const osg::Vec4& value )
{
    color = value;
}

const osg::Vec4&
SkinResource::getColor() const
{
    return color;
}

osg::StateSet*
SkinResource::createStateSet( osg::Image* image )
{
    osg::StateSet* state_set = NULL;
    if ( image )
    {
        osg::Texture* tex = new osg::Texture2D( image );
        tex->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT );
        tex->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT );

        osg::TexEnv* texenv = new osg::TexEnv();
        texenv = new osg::TexEnv();
        texenv->setMode( getTextureMode() );

        state_set = new osg::StateSet();
        state_set->setTextureAttributeAndModes( 0, tex, osg::StateAttribute::ON );
        state_set->setTextureAttribute( 0, texenv, osg::StateAttribute::ON );

        if ( ImageUtils::hasAlpha( image ) )
        {
            osg::BlendFunc* blend_func = new osg::BlendFunc();
            blend_func->setFunction( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            state_set->setAttributeAndModes( blend_func, osg::StateAttribute::ON );
            state_set->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );
        }
    }

    return state_set;
}

osg::StateSet*
SkinResource::createStateSet()
{
    return createStateSet( createImage() );
}

osg::Image*
SkinResource::createImage()
{
    ScopedLock<ReentrantMutex> lock( getMutex() );
    osg::Image* image = osgDB::readImageFile( getAbsoluteURI() );
    if (!image)
    {
        //There was an error reading the SkinResource, so display a red X for the texture
        osg::notify(osg::NOTICE) << "Error loading SkinResource from " << getAbsoluteURI() << std::endl;
        image = createErrorImage();
        image->setFileName(std::string(getName()) + "error.png");
    }
    return image;
    //if ( !image.valid() )
    //{
    //    ScopedLock<ReentrantMutex> lock( getMutex() );
    //    if ( !image.valid() ) // check again in case the image was created before obtaining the lock
    //    {
    //        const_cast<SkinResource*>(this)->image = osgDB::readImageFile( getAbsoluteURI() );

    //        if ( !image.valid() )
    //        {
    //            osgGIS::notify( osg::WARN ) << "** WARNING: unable to load image file: " << getAbsoluteURI() << std::endl;
    //        }
    //    }
    //}

    //return image.get();
}

SkinResourceQuery::SkinResourceQuery()
{
    has_tex_height     = false;
    has_min_tex_height = false;
    has_max_tex_height = false;
    has_repeat_vert    = false;
}

void 
SkinResourceQuery::setTextureHeight( double value )
{
    tex_height = value;
    has_tex_height = true;
}

bool 
SkinResourceQuery::hasTextureHeight() const
{
    return has_tex_height;
}

double 
SkinResourceQuery::getTextureHeight() const
{
    return tex_height;
}

void 
SkinResourceQuery::setMinTextureHeight( double value )
{
    min_tex_height = value;
    has_min_tex_height = true;
}

bool 
SkinResourceQuery::hasMinTextureHeight() const
{
    return has_min_tex_height;
}

double 
SkinResourceQuery::getMinTextureHeight() const
{
    return min_tex_height;
}

void 
SkinResourceQuery::setMaxTextureHeight( double value )
{
    max_tex_height = value;
    has_max_tex_height = true;
}

bool 
SkinResourceQuery::hasMaxTextureHeight() const
{
    return has_max_tex_height;
}

double 
SkinResourceQuery::getMaxTextureHeight() const
{
    return max_tex_height;
}


void 
SkinResourceQuery::setRepeatsVertically( bool value )
{
    repeat_vert = value;
    has_repeat_vert = true;
}

bool 
SkinResourceQuery::hasRepeatsVertically() const
{
    return has_repeat_vert;
}

bool 
SkinResourceQuery::getRepeatsVertically() const
{
    return repeat_vert;
}

void 
SkinResourceQuery::addTag( const char* tag )
{
    tags.push_back( tag );
}

const TagList& 
SkinResourceQuery::getTags() const
{
    return tags;
}


const std::string& 
SkinResourceQuery::getHashCode()
{
    //TODO
    return EMPTY_STRING;
}

