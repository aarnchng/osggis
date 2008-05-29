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

#include <osgGIS/BuildLabelsFilter>
#include <osgText/Text>
#include <osgDB/ReadFile>
#include <osg/Depth>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BuildLabelsFilter );


#define FRAND (((double)(rand()%100))/100.0)


BuildLabelsFilter::BuildLabelsFilter()
{
    setTextExpr( "text" );
}

BuildLabelsFilter::BuildLabelsFilter( const BuildLabelsFilter& rhs )
: BuildGeomFilter( rhs ),
  text_expr( rhs.text_expr ),
  font_size_expr( rhs.font_size_expr ),
  disable_depth_test( rhs.disable_depth_test )
{
    //NOP
}

BuildLabelsFilter::~BuildLabelsFilter()
{
    //NOP
}

void
BuildLabelsFilter::setTextExpr( const std::string& value )
{
    text_expr = value;
}

const std::string&
BuildLabelsFilter::getTextExpr() const
{
    return text_expr;
}

void 
BuildLabelsFilter::setFontSizeExpr( const std::string& expr )
{
    font_size_expr = expr;
}

const std::string& 
BuildLabelsFilter::getFontSizeExpr() const
{
    return font_size_expr;
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
        setTextExpr( p.getValue() );
    else if ( p.getName() == "topmost" )
        setDisableDepthTest( true );
    else if ( p.getName() == "font_size" )
        setFontSizeExpr( p.getValue() );
    BuildGeomFilter::setProperty( p );
}


Properties
BuildLabelsFilter::getProperties() const
{
    Properties p = BuildGeomFilter::getProperties();
    if ( getTextExpr().length() > 0 )
        p.push_back( Property( "text", getTextExpr() ) );
    if ( getDisableDepthTest() )
        p.push_back( Property( "topmost", true ) );
    if ( getFontSizeExpr().length() > 0 )
        p.push_back( Property( "font_size", font_size_expr ) );
    return p;
}


FragmentList
BuildLabelsFilter::process( FeatureList& input, FilterEnv* env )
{
    return BuildGeomFilter::process( input, env );
}


FragmentList
BuildLabelsFilter::process( Feature* input, FilterEnv* env )
{
    FragmentList output;

    osg::Vec4 color = getColorForFeature( input, env );

    // the text string:

    //TODO: new Script() is a memory leak in the following line!!
    ScriptResult r = env->getScriptEngine()->run( new Script( getTextExpr() ), input, env );
    std::string text = r.isValid()? r.asString() : "error";

    // resolve the size:
    
    //TODO: new Script() is a memory leak in the following line!!
    r = env->getScriptEngine()->run( new Script( getFontSizeExpr() ), input, env );
    double font_size = r.asDouble( 16.0 );
    
    osgText::Text* t = new osgText::Text();
    t->setAutoRotateToScreen( true );
    t->setCharacterSizeMode( osgText::Text::SCREEN_COORDS );
    t->setText( text.c_str() );
    t->setColor( color );
    t->setCharacterSize( (float)font_size );
    t->setPosition( input->getExtent().getCentroid() );
    t->setBackdropType( osgText::Text::OUTLINE );
    t->setBackdropColor( osg::Vec4(0,0,0,1) );
    t->getOrCreateStateSet()->setAttribute( new osg::Depth( osg::Depth::ALWAYS, 0, 1, false ), osg::StateAttribute::ON );

    output.push_back( new Fragment( t ) );

    return output;
}
