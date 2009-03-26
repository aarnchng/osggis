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

#include <osgGIS/BuildLabelsFilter>
#include <osgText/Text>
#include <osgDB/ReadFile>
#include <osg/Depth>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BuildLabelsFilter );


#define DEFAULT_DISABLE_DEPTH_TEST true
#define DEFAULT_FONT_NAME          ""


BuildLabelsFilter::BuildLabelsFilter()
{
    setTextScript( new Script( "default", "lua", "'text'" ) );
    setDisableDepthTest( DEFAULT_DISABLE_DEPTH_TEST );
    setFontName( DEFAULT_FONT_NAME );
}

BuildLabelsFilter::BuildLabelsFilter( const BuildLabelsFilter& rhs )
: BuildGeomFilter( rhs ),
  text_script( rhs.text_script.get() ),
  font_size_script( rhs.font_size_script.get() ),
  disable_depth_test( rhs.disable_depth_test ),
  font_name( rhs.font_name )
{
    //NOP
}

BuildLabelsFilter::~BuildLabelsFilter()
{
    //NOP
}

void
BuildLabelsFilter::setTextScript( Script* value )
{
    text_script = value;
}

Script*
BuildLabelsFilter::getTextScript() const
{
    return text_script.get();
}

void 
BuildLabelsFilter::setFontSizeScript( Script* value )
{
    font_size_script = value;
}

Script*
BuildLabelsFilter::getFontSizeScript() const
{
    return font_size_script.get();
}

void
BuildLabelsFilter::setFontName( const std::string& value )
{
    font_name = value;
}

const std::string&
BuildLabelsFilter::getFontName() const
{
    return font_name;
}

void 
BuildLabelsFilter::setDisableDepthTest( bool value )
{
    disable_depth_test = value;
}

bool 
BuildLabelsFilter::getDisableDepthTest() const
{
    return disable_depth_test;
}

void
BuildLabelsFilter::setProperty( const Property& p )
{
    if ( p.getName() == "text" )
        setTextScript( new Script( p.getValue() ) );
    else if ( p.getName() == "disable_depth_test" )
        setDisableDepthTest( p.getBoolValue( getDisableDepthTest() ) );
    else if ( p.getName() == "font_size" )
        setFontSizeScript( new Script( p.getValue() ) );
    else if ( p.getName() == "font" )
        setFontName( p.getValue() );
    BuildGeomFilter::setProperty( p );
}


Properties
BuildLabelsFilter::getProperties() const
{
    Properties p = BuildGeomFilter::getProperties();
    if ( getTextScript() )
        p.push_back( Property( "text", getTextScript()->getCode() ) );
    if ( getFontSizeScript() )
        p.push_back( Property( "font_size", getFontSizeScript()->getCode() ) );
    if ( getFontName() != DEFAULT_FONT_NAME )
        p.push_back( Property( "font", getFontName() ) );
    if ( getDisableDepthTest() != DEFAULT_DISABLE_DEPTH_TEST )
        p.push_back( Property( "disable_depth_test", getDisableDepthTest() ) );
    return p;
}


FragmentList
BuildLabelsFilter::process( FeatureList& input, FilterEnv* env )
{
    // load and cache the font
    if ( getFontName().length() > 0 && !font.valid() )
    {
        font = osgText::readFontFile( getFontName() );
    }

    return BuildGeomFilter::process( input, env );
}

class ZCalc : public GeoPointVisitor {
public:
    double z_sum;
    int z_count;
    ZCalc() : z_sum(0.0), z_count(0) { }
    bool visitPoint( GeoPoint& point ) {
        z_sum += point.getDim() > 2? point.z() : 0.0;
        z_count++;
        return true;
    }
};

FragmentList
BuildLabelsFilter::process( Feature* input, FilterEnv* env )
{
    FragmentList output;

    // the text string:
    std::string text;
    if ( getTextScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getTextScript(), input, env );
        if ( r.isValid() ) 
            text = r.asString();
        else
            env->getReport()->error( r.asString() );
    }

    // resolve the size:
    double font_size = 16.0;
    if ( getFontSizeScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getFontSizeScript(), input, env );
        if ( r.isValid() )
            font_size = r.asDouble( font_size );
        else
            env->getReport()->error( r.asString() );
    }

    // the text color:
    osg::Vec4 color = getColorForFeature( input, env );

    // calculate the 3D centroid of the feature:
    // TODO: move this to the geoshapelist class
    osg::Vec3d point( input->getExtent().getCentroid() );    
    ZCalc zc;
    input->getShapes().accept( zc );
    point.z() = zc.z_count > 0? zc.z_sum/(double)zc.z_count : 0.0;
    
    // build the drawable:
    osgText::Text* t = new osgText::Text();
    t->setAutoRotateToScreen( true );
    t->setCharacterSizeMode( osgText::Text::SCREEN_COORDS );
    t->setAlignment( osgText::Text::CENTER_CENTER );
    t->setText( text.c_str() );
    t->setColor( color );
    t->setCharacterSize( (float)font_size );
    t->setPosition( point );
    t->setBackdropType( osgText::Text::OUTLINE );
    t->setBackdropColor( osg::Vec4(0,0,0,1) );

    if ( font.valid() )
    {
        t->setFont( font.get() );
    }

    if ( getDisableDepthTest() )
    {
        t->getOrCreateStateSet()->setAttribute( new osg::Depth( osg::Depth::ALWAYS, 0, 1, false ), osg::StateAttribute::ON );
    }

    output.push_back( new Fragment( t ) );

    return output;
}
