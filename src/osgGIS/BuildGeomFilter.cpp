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

#include <osgGIS/BuildGeomFilter>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgUtil/Tessellator>
#include <osgUtil/Optimizer>
#include <osgUtil/SmoothingVisitor>
#include <osgText/Text>
#include <sstream>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BuildGeomFilter );

#define DEFAULT_RASTER_OVERLAY_MAX_SIZE 0


BuildGeomFilter::BuildGeomFilter()
{
    overall_color = osg::Vec4f(1,1,1,1);
    setRasterOverlayMaxSize( DEFAULT_RASTER_OVERLAY_MAX_SIZE );
}

BuildGeomFilter::BuildGeomFilter( const BuildGeomFilter& rhs )
: FragmentFilter( rhs ),
  overall_color( rhs.overall_color ),
  raster_overlay_max_size( rhs.raster_overlay_max_size ),
  raster_overlay_script( rhs.raster_overlay_script.get() ),
  color_script( rhs.color_script.get() ),
  feature_name_script( rhs.feature_name_script.get() )
{
    //NOP
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
BuildGeomFilter::setRasterOverlayScript( Script* value )
{
    raster_overlay_script = value;
}

Script*
BuildGeomFilter::getRasterOverlayScript() const
{
    return const_cast<BuildGeomFilter*>(this)->raster_overlay_script.get();
}

void
BuildGeomFilter::setRasterOverlayMaxSize( int value )
{
    raster_overlay_max_size = value;
}

int
BuildGeomFilter::getRasterOverlayMaxSize() const
{
    return raster_overlay_max_size;
}

void 
BuildGeomFilter::setFeatureNameScript( Script* script )
{
    feature_name_script = script;
}

Script*
BuildGeomFilter::getFeatureNameScript() const
{
    return feature_name_script.get();
}

void
BuildGeomFilter::setProperty( const Property& prop )
{
    if ( prop.getName() == "color" )
        setColorScript( new Script( prop.getValue() ) );
    else if ( prop.getName() == "raster_overlay" )
        setRasterOverlayScript( new Script( prop.getValue() ) );
    else if ( prop.getName() == "raster_overlay_max_size" )
        setRasterOverlayMaxSize( prop.getIntValue( DEFAULT_RASTER_OVERLAY_MAX_SIZE ) );
    else if ( prop.getName() == "feature_name" )
        setFeatureNameScript( new Script( prop.getValue() ) );

    FragmentFilter::setProperty( prop );
}


Properties
BuildGeomFilter::getProperties() const
{
    Properties p = FragmentFilter::getProperties();
    if ( getColorScript() )
        p.push_back( Property( "color", getColorScript()->getCode() ) );
    if ( getRasterOverlayScript() )
        p.push_back( Property( "raster_overlay", getRasterOverlayScript()->getCode() ) );
    if ( getRasterOverlayMaxSize() != DEFAULT_RASTER_OVERLAY_MAX_SIZE )
        p.push_back( Property( "raster_overlay_max_size", getRasterOverlayMaxSize() ) );
    if ( getFeatureNameScript() )
        p.push_back( Property( "feature_name", getFeatureNameScript() ) );
    return p;
}

FragmentList
BuildGeomFilter::process( FeatureList& input, FilterEnv* env )
{
    // if features are arriving in batch, resolve the color here.
    // otherwise we will resolve it later in process(feature,env).
    is_batch = input.size() > 1;
    batch_feature_color = overall_color;
    if ( is_batch && getColorScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getColorScript(), env );
        if ( r.isValid() )
            batch_feature_color = r.asVec4();
        else
            env->getReport()->error( r.asString() );
    }

    return FragmentFilter::process( input, env );
}


FragmentList
BuildGeomFilter::process( Feature* input, FilterEnv* env )
{
    FragmentList output;

    // LIMITATION: this filter assumes all feature's shapes are the same
    // shape type! TODO: sort into bins of shape type and create a separate
    // geometry for each. Then merge the geometries.
    bool needs_tessellation = false;

    Fragment* frag = new Fragment();
    
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
        }

        generateNormals( geom );

        frag->addDrawable( geom );
    }

    frag->addAttributes( input->getAttributes() );
    applyFragmentName( frag, input, env );

    output.push_back( frag );

    return output;
}

osg::Vec4
BuildGeomFilter::getColorForFeature( Feature* feature, FilterEnv* env )
{
    osg::Vec4 result = overall_color;

    if ( is_batch )
    {
        result = batch_feature_color;
    }
    else if ( getColorScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getColorScript(), feature, env );
        if ( r.isValid() )
            result = r.asVec4();
        else
            env->getReport()->error( r.asString() );
    }

    return result;
}

void
BuildGeomFilter::generateNormals( osg::Geometry* geom )
{
    if ( geom )
    {
        osgUtil::SmoothingVisitor smoother;
        smoother.smooth( *geom );
    }
}


void
BuildGeomFilter::applyFragmentName( Fragment* frag, Feature* feature, FilterEnv* env )
{
    if ( getFeatureNameScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getFeatureNameScript(), feature, env );
        if ( r.isValid() )
            frag->setName( r.asString() );
        else
            env->getReport()->error( r.asString() );
    }
}


void
BuildGeomFilter::applyOverlayTexturing( osg::Geometry* geom, Feature* input, FilterEnv* env )
{
    GeoExtent tex_extent;

    if ( getRasterOverlayScript() )
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
        // if we are dealing with geocentric data, we will need to xform back to a real
        // projection in order to determine texture coords:
        GeoExtent tex_extent_geo;
        if ( env->getInputSRS()->isGeocentric() )
        {
            tex_extent_geo = GeoExtent(
                tex_extent.getSRS()->getGeographicSRS()->transform( tex_extent.getSouthwest() ),
                tex_extent.getSRS()->getGeographicSRS()->transform( tex_extent.getNortheast() ) );
        }

        osg::Vec2Array* texcoords = new osg::Vec2Array( verts->size() );
        for( unsigned int j=0; j<verts->size(); j++ )
        {
            // xform back to raw SRS w.o. ref frame:
            GeoPoint vert( (*verts)[j], env->getInputSRS() );
            GeoPoint vert_map = vert.getAbsolute();
            float tu, tv;
            if ( env->getInputSRS()->isGeocentric() )
            {
                tex_extent_geo.getSRS()->transformInPlace( vert_map );
                tu = (vert_map.x() - tex_extent_geo.getXMin()) / width;
                tv = (vert_map.y() - tex_extent_geo.getYMin()) / height;
            }
            else
            {
                tu = (vert_map.x() - tex_extent.getXMin()) / width;
                tv = (vert_map.y() - tex_extent.getYMin()) / height;
            }
            (*texcoords)[j].set( tu, tv );
        }
        geom->setTexCoordArray( 0, texcoords );
    }

    // if we are applying the raster per-feature, do so now.
    // TODO: deprecate? will we ever use this versus the BuildNodesFilter overlay? maybe
    if ( getRasterOverlayScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getRasterOverlayScript(), input, env );
        if ( r.isValid() )
        {
            RasterResource* raster = env->getSession()->getResources()->getRaster( r.asString() );
            if ( raster )
            {
                osg::Image* image = NULL;
                std::stringstream builder;
                builder << "rtex_" << input->getOID() << ".jpg"; //TODO: dds with DXT1 compression

                osg::ref_ptr<osg::StateSet> raster_ss = new osg::StateSet();
                if ( raster->applyToStateSet( raster_ss.get(), tex_extent, getRasterOverlayMaxSize(), &image ) )
                {
                    image->setFileName( builder.str() );
                    geom->setStateSet( raster_ss.get() );

                    // add this as a skin resource so the compiler can properly localize and deploy it.
                    env->getResourceCache()->addSkin( raster_ss.get() );
                }
            }
        }
        else
        {
            env->getReport()->error( r.asString() );
        }
    }
}
