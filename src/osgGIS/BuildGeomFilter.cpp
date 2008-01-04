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

#include <osgGIS/BuildGeomFilter>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgUtil/Tessellator>
#include <osgUtil/Optimizer>
#include <osgText/Text>
#include <sstream>

#define RANDCOL ((((float)(::rand()%255)))/255.0f)

#define PROP_COLOR "BuildGeomFilter::color"
#define PROP_BATCH "BuildGeomFilter::batch" 

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BuildGeomFilter );


// pseudo-random color table
std::vector<osg::Vec4d> color_table;
const int num_colors = 1024;
void buildColorMap( unsigned int seed, float alpha )
{
    ::srand( seed );
    for( int i=0; i<1024; i++ )
    {
        color_table.push_back( osg::Vec4f( RANDCOL, RANDCOL, RANDCOL, alpha ) );
    }
}



BuildGeomFilter::BuildGeomFilter()
{
    options = 0;
    overall_color = osg::Vec4f(1,1,1,1);
}


BuildGeomFilter::BuildGeomFilter( const int& _options )
{
    options = _options;
    overall_color = osg::Vec4f(1,1,1,1);
}


BuildGeomFilter::BuildGeomFilter( const osg::Vec4f& color )
{
    options = 0;
    overall_color = color;
}


BuildGeomFilter::~BuildGeomFilter()
{
}


void
BuildGeomFilter::setColor( const osg::Vec4f& _color )
{
    overall_color = _color;
}

const osg::Vec4f&
BuildGeomFilter::getColor() const
{
    return overall_color;
}

void
BuildGeomFilter::setColorExpr( const std::string& _expr )
{
    color_expr = _expr;
}

const std::string&
BuildGeomFilter::getColorExpr() const
{
    return color_expr;
}


void
BuildGeomFilter::setRandomizeColors( bool on_off )
{
    if ( on_off )
        options |= RANDOMIZE_COLORS;
    else
        options &= ~RANDOMIZE_COLORS;
}

bool
BuildGeomFilter::getRandomizeColors() const
{
    return options & RANDOMIZE_COLORS;
}


void
BuildGeomFilter::setProperty( const Property& prop )
{
    if ( prop.getName() == "color" )
        setColorExpr( prop.getValue() );
        //setColorExpr( prop.getVec4fValue() );
    if ( prop.getName() == "randomize_colors" )
        setRandomizeColors( prop.getBoolValue( getRandomizeColors() ) );

    DrawableFilter::setProperty( prop );
}


Properties
BuildGeomFilter::getProperties() const
{
    Properties p = DrawableFilter::getProperties();
    p.push_back( Property( "color", getColorExpr() ) );
    p.push_back( Property( "randomize_colors", getRandomizeColors() ) );
    return p;
}

DrawableList
BuildGeomFilter::process( FeatureList& input, FilterEnv* env )
{
    // if features are arriving in batch, resolve the color here.
    // otherwise we will resolve it later in process(feature,env).
    osg::Vec4 color = overall_color;
    bool batch = input.size() > 1;

    if ( batch && getColorExpr().length() > 0 )
    {
        ScriptResult r = env->getScriptEngine()->run(
            new Script( getColorExpr() ),
            env );

        if ( r.isValid() )
            color = r.asVec4();
    }

    env->setProperty( Property( PROP_COLOR, color ) );
    env->setProperty( Property( PROP_BATCH, batch ) );

    //if ( getRandomizeColors() )
    //    active_color = getRandomColor();

    return DrawableFilter::process( input, env );
}

DrawableList
BuildGeomFilter::process( Feature* input, FilterEnv* env ) //FeatureList& input, FilterEnv* env )
{
    //osg::notify( osg::ALWAYS )
    //    << "Feature (" << input->getOID() << ") extent = " << input->getExtent().toString()
    //    << std::endl;

    DrawableList output;

    // LIMITATION: this filter assumes all feature's shapes are the same
    // shape type! TODO: sort into bins of shape type and create a separate
    // geometry for each. Then merge the geometries.
    bool needs_tessellation = false;

    osg::Geometry* geom = new osg::Geometry();

    // TODO: pre-total points and pre-allocate these arrays:
    osg::Vec3Array* verts = new osg::Vec3Array();
    geom->setVertexArray( verts );
    unsigned int vert_ptr = 0;

    // per-vertex coloring takes more memory than per-primitive-set coloring,
    // but it renders faster.
    osg::Vec4Array* colors = new osg::Vec4Array();
    geom->setColorArray( colors );
    geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );

    //osg::Vec3Array* normals = new osg::Vec3Array();
    //geom->setNormalArray( normals );
    //geom->setNormalBinding( osg::Geometry::BIND_OVERALL );
    //normals->push_back( osg::Vec3( 0, 0, 1 ) );

    const GeoShapeList& shapes = input->getShapes();

    // if we're in batch mode, the color was resolved in the other process() function.
    // otherwise we still need to resolve it.
    osg::Vec4 color = getColorForFeature( input, env );

    for( GeoShapeList::const_iterator s = shapes.begin(); s != shapes.end(); s++ )
    {
        const GeoShape& shape = *s;

        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        {
            needs_tessellation = true;
        }

        GLenum prim_type = 
            shape.getShapeType() == GeoShape::TYPE_POINT? osg::PrimitiveSet::POINTS : 
            shape.getShapeType() == GeoShape::TYPE_LINE?  osg::PrimitiveSet::LINE_STRIP :
            osg::PrimitiveSet::LINE_LOOP;

        for( unsigned int pi = 0; pi < shape.getPartCount(); pi++ )
        {
            unsigned int part_ptr = vert_ptr;
            const GeoPointList& points = shape.getPart( pi );
            for( unsigned int vi = 0; vi < points.size(); vi++ )
            {
                verts->push_back( points[vi] );
                vert_ptr++;
                colors->push_back( color );
            }
            geom->addPrimitiveSet( new osg::DrawArrays( prim_type, part_ptr, vert_ptr-part_ptr ) );
        }
    }

    output.push_back( geom );
    
    // tessellate all polygon geometries. Tessellating each geometry separately
    // with TESS_TYPE_GEOMETRY is much faster than doing the whole bunch together
    // using TESS_TYPE_DRAWABLE.
    if ( needs_tessellation )
    {
        for( DrawableList::iterator i = output.begin(); i != output.end(); i++ )
        {
            osg::Geometry* geom = static_cast<osg::Geometry*>( i->get() );
            osgUtil::Tessellator tess;
            tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
            tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
            tess.retessellatePolygons( *geom );
        }
    }

    return output;
}

osg::Vec4
BuildGeomFilter::getColorForFeature( Feature* feature, FilterEnv* env )
{
    osg::Vec4 result = overall_color;
    
    bool batch = env->getProperties().getBoolValue( PROP_BATCH, false );
    if ( batch )
    {
        result = env->getProperties().getVec4Value( PROP_COLOR );
    }
    else
    {
        ScriptResult r = env->getScriptEngine()->run(
            new Script( getColorExpr() ),
            feature,
            env );

        if ( r.isValid() )
            result = r.asVec4();
    }

    return result;

    //err...address this later
    //return getRandomizeColors()? active_color : overall_color;
}

osg::Vec4
BuildGeomFilter::getRandomColor()
{
    if ( color_table.size() == 0 )
        buildColorMap( 123, 1.0f ); // TODO: make this user-provided

    //unsigned int index = ((int)f->getOID())%color_table.size();
    unsigned int index = ((int)::rand())%color_table.size();
    return color_table[index];
}