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

//#define RANDCOL ((((float)(::rand()%255)))/255.0f)

#define PROP_COLOR "BuildGeomFilter::color"
#define PROP_BATCH "BuildGeomFilter::batch" 

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BuildGeomFilter );



BuildGeomFilter::BuildGeomFilter()
{
    overall_color = osg::Vec4f(1,1,1,1);
    max_raster_size = 0;
}


BuildGeomFilter::~BuildGeomFilter()
{
    //NOP
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
BuildGeomFilter::setColorScript( Script* value )
{
    color_script = value;
}

Script*
BuildGeomFilter::getColorScript() const
{
    return color_script.get();
}

void
BuildGeomFilter::setRasterScript( Script* value )
{
    raster_script = value;
}

Script*
BuildGeomFilter::getRasterScript() const
{
    return const_cast<BuildGeomFilter*>(this)->raster_script.get();
}

void
BuildGeomFilter::setMaxRasterSize( int value )
{
    max_raster_size = value;
}

int
BuildGeomFilter::getMaxRasterSize() const
{
    return max_raster_size;
}

void
BuildGeomFilter::setProperty( const Property& prop )
{
    if ( prop.getName() == "color" )
        setColorScript( new Script( prop.getValue() ) );
    else if ( prop.getName() == "raster" )
        setRasterScript( new Script( prop.getValue() ) );
    else if ( prop.getName() == "max_raster_size" )
        setMaxRasterSize( prop.getIntValue( 0 ) );

    DrawableFilter::setProperty( prop );
}


Properties
BuildGeomFilter::getProperties() const
{
    Properties p = DrawableFilter::getProperties();
    if ( getColorScript() )
        p.push_back( Property( "color", getColorScript()->getCode() ) );
    if ( getRasterScript() )
        p.push_back( Property( "raster", getRasterScript()->getCode() ) );
    if ( getMaxRasterSize() > 0 )
        p.push_back( Property( "max_raster_size", getMaxRasterSize() ) );
    return p;
}

DrawableList
BuildGeomFilter::process( FeatureList& input, FilterEnv* env )
{
    // if features are arriving in batch, resolve the color here.
    // otherwise we will resolve it later in process(feature,env).
    osg::Vec4 color = overall_color;
    bool batch = input.size() > 1;

    if ( batch && getColorScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getColorScript(), env );
        if ( r.isValid() )
            color = r.asVec4();
    }

    env->setProperty( Property( PROP_COLOR, color ) );
    env->setProperty( Property( PROP_BATCH, batch ) );

    return DrawableFilter::process( input, env );
}

DrawableList
BuildGeomFilter::process( Feature* input, FilterEnv* env ) //FeatureList& input, FilterEnv* env )
{
    //osg::notify( osg::ALWAYS )
    //    << "Feature (" << input->getOID() << ") extent = " << input->getExtent().toString()
    //    << std::endl;

    // calcuate feature extent in the SRS, which we'll need for texture coordinates.
    //GeoExtent abs_feature_extent(
    //    input->getExtent().getSouthwest().getAbsolute(),
    //    input->getExtent().getNortheast().getAbsolute() );

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
    
    // tessellate all polygon geometries. Tessellating each geometry separately
    // with TESS_TYPE_GEOMETRY is much faster than doing the whole bunch together
    // using TESS_TYPE_DRAWABLE.
    if ( needs_tessellation )
    {
        osgUtil::Tessellator tess;
        tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
        tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
        tess.retessellatePolygons( *geom );

        applyOverlayTexturing( geom, input, env );
        //genTextureCoords( geom, abs_feature_extent, env );
        //applyRasterTexture( geom, abs_feature_extent, input, env );
    }

    //    for( DrawableList::iterator i = output.begin(); i != output.end(); i++ )
    //    {
    //        osg::Geometry* drawable = static_cast<osg::Geometry*>( i->get() );
    //        osgUtil::Tessellator tess;
    //        tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
    //        tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
    //        tess.retessellatePolygons( *drawable );

    //        genTextureCoords( drawable, abs_feature_extent, env );
    //    }
    //}

    output.push_back( geom );

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
    else if ( getColorScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getColorScript(), feature, env );
        if ( r.isValid() )
            result = r.asVec4();
    }

    return result;
}


void
BuildGeomFilter::applyOverlayTexturing( osg::Geometry* geom, Feature* input, FilterEnv* env )
{
    GeoExtent tex_extent;

    if ( getRasterScript() )
    {
        // if there's a raster script for this filter, we're applying textures per-feature:
        tex_extent = GeoExtent(
            input->getExtent().getSouthwest().getAbsolute(),
            input->getExtent().getNortheast().getAbsolute() );
    }
    else
    {
        // otherwise prepare the geometry for an overlay texture covering the entire working extent:
        tex_extent = env->getExtent();
    }

    float width  = (float)tex_extent.getWidth();
    float height = (float)tex_extent.getHeight();

    // now visit the verts and calculate texture coordinates for each one.
    osg::Vec3Array* verts = dynamic_cast<osg::Vec3Array*>( geom->getVertexArray() );
    if ( verts )
    {
        osg::Vec2Array* texcoords = new osg::Vec2Array( verts->size() );
        for( unsigned int j=0; j<verts->size(); j++ )
        {
            // xform back to raw SRS w.o. ref frame:
            GeoPoint vert( (*verts)[j], env->getInputSRS() );
            GeoPoint vert_map = vert.getAbsolute();
            float tu = (vert_map.x() - tex_extent.getXMin()) / width;
            float tv = (vert_map.y() - tex_extent.getYMin()) / height;
            (*texcoords)[j].set( tu, tv );
        }
        geom->setTexCoordArray( 0, texcoords );
    }

    // if we are applying the raster per-feature, do so now.
    if ( getRasterScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getRasterScript(), input, env );
        if ( r.isValid() )
        {
            RasterResource* raster = env->getSession()->getResources()->getRaster( r.asString() );
            if ( raster )
            {
                osg::Image* image = NULL;
                std::stringstream builder;
                builder << "rtex_" << input->getOID() << ".jpg"; //TODO: dds with DXT1 compression

                osg::ref_ptr<osg::StateSet> raster_ss = new osg::StateSet();
                if ( raster->applyToStateSet( raster_ss.get(), tex_extent, getMaxRasterSize(), builder.str(), &image ) )
                {
                    geom->setStateSet( raster_ss.get() );

                    // add this as a skin resource so the compiler can properly localize and deploy it.
                    SkinResource* skin = new SkinResource( image );
                    env->getSession()->markResourceUsed( skin );
                }
            }
        }
    }
}
//
//
//void
//BuildGeomFilter::genTextureCoords( osg::Geometry* geom, const GeoExtent& abs_feature_extent, FilterEnv* env )
//{
//    if ( getRasterScript() )
//    {
//        float width  = (float)abs_feature_extent.getWidth();
//        float height = (float)abs_feature_extent.getHeight();
//
//        // now visit the verts and calculate texture coordinates for each one.
//        osg::Vec3Array* verts = dynamic_cast<osg::Vec3Array*>( geom->getVertexArray() );
//        if ( verts )
//        {
//            osg::Vec2Array* texcoords = new osg::Vec2Array( verts->size() );
//            for( unsigned int j=0; j<verts->size(); j++ )
//            {
//                // xform back to raw SRS w.o. ref frame:
//                GeoPoint vert( (*verts)[j], env->getInputSRS() );
//                GeoPoint vert_map = vert.getAbsolute();
//                float tu = (vert_map.x() - abs_feature_extent.getXMin()) / width;
//                float tv = (vert_map.y() - abs_feature_extent.getYMin()) / height;
//                (*texcoords)[j].set( tu, tv );
//            }
//            geom->setTexCoordArray( 0, texcoords );
//        }
//    }
//}
//
//void
//BuildGeomFilter::applyRasterTexture( osg::Geometry* geom, const GeoExtent& abs_feature_extent, Feature* input, FilterEnv* env )
//{
//    // apply a geo-raster texture if necessary:
//    if ( getRasterScript() )
//    {
//        ScriptResult r = env->getScriptEngine()->run( getRasterScript(), input, env );
//        if ( r.isValid() )
//        {
//            RasterResource* raster = env->getSession()->getResources()->getRaster( r.asString() );
//            if ( raster )
//            {
//                osg::Image* image = NULL;
//                std::stringstream builder;
//                builder << "rtex_" << input->getOID() << ".jpg"; //TODO: dds with DXT1 compression
//
//                osg::ref_ptr<osg::StateSet> raster_ss = new osg::StateSet();
//                if ( raster->applyToStateSet( raster_ss.get(), abs_feature_extent, getMaxRasterSize(), builder.str(), &image ) )
//                {
//                    geom->setStateSet( raster_ss.get() );
//
//                    // add this as a skin resource so the compiler can properly localize and deploy it.
//                    SkinResource* skin = new SkinResource( image );
//                    env->getSession()->markResourceUsed( skin );
//                }
//            }
//        }
//    }
//}