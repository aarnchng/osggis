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

#include <osgGIS/ExtrudeGeomFilter>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/Image>
#include <osgDB/ReadFile>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Tessellator>
#include <float.h>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( ExtrudeGeomFilter );


//#define FRAND (((double)(rand()%100))/100.0)

#define PROP_BATCH "ExtrudeGeomFilter::batch"
#define PROP_WALL_SKIN "ExtrudeGeomFilter::wall_skin"

#define DEFAULT_USE_VBOS false


ExtrudeGeomFilter::ExtrudeGeomFilter()
{
    tex_index = 0;
}


ExtrudeGeomFilter::~ExtrudeGeomFilter()
{
    //NOP
}

void
ExtrudeGeomFilter::setHeightScript( Script* value )
{
    height_script = value;
}

Script*
ExtrudeGeomFilter::getHeightScript() const
{
    return height_script.get();
}

void
ExtrudeGeomFilter::setWallSkinScript( Script* value )
{
    wall_skin_script = value;
}

Script*
ExtrudeGeomFilter::getWallSkinScript() const
{
    return wall_skin_script.get();
}

void
ExtrudeGeomFilter::setProperty( const Property& p )
{
    if ( p.getName() == "height" )
        setHeightScript( new Script( p.getValue() ) );
    else if ( p.getName() == "wall_skin" )
        setWallSkinScript( new Script( p.getValue() ) );
    BuildGeomFilter::setProperty( p );
}


Properties
ExtrudeGeomFilter::getProperties() const
{
    Properties p = BuildGeomFilter::getProperties();
    if ( getHeightScript() )
        p.push_back( Property( "height", getHeightScript()->getCode() ) );
    if ( getWallSkinScript() )
        p.push_back( Property( "wall_skin", getWallSkinScript()->getCode() ) );
    return p;
}


static bool
extrudeWallsUp(const GeoShape&         shape, 
               const SpatialReference* srs, 
               double                  height, 
               osg::Geometry*          walls,
               osg::Geometry*          rooflines,
               const osg::Vec4&        color,
               SkinResource*           skin )
{
    bool made_geom = true;

    double tex_width_m = skin? skin->getTextureWidthMeters() : 1.0;
    double tex_height_m = skin? skin->getTextureHeightMeters() : 1.0;
    bool   tex_repeats_y = skin? skin->getRepeatsVertically() : true;

    int point_count = shape.getTotalPointCount();
    int num_verts = 2 * point_count;
    if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        num_verts += 2 * shape.getPartCount();

    osg::Vec3Array* verts = new osg::Vec3Array( num_verts );
    walls->setVertexArray( verts );

    osg::Vec2Array* texcoords = new osg::Vec2Array( num_verts );
    walls->setTexCoordArray( 0, texcoords );

    osg::Vec4Array* colors = new osg::Vec4Array( num_verts );
    walls->setColorArray( colors );
    walls->setColorBinding( osg::Geometry::BIND_PER_VERTEX );

    osg::Vec3Array* roof_verts = NULL;
    osg::Vec4Array* roof_colors = NULL;
    if ( rooflines )
    {
        roof_verts = new osg::Vec3Array( point_count );
        rooflines->setVertexArray( roof_verts );

        roof_colors = new osg::Vec4Array( point_count );
        rooflines->setColorArray( roof_colors );
        rooflines->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
    }

    int wall_vert_ptr = 0;
    int roof_vert_ptr = 0;

    GLenum prim_type = shape.getShapeType() == GeoShape::TYPE_POINT?
        osg::PrimitiveSet::LINES :
        osg::PrimitiveSet::TRIANGLE_STRIP;

    for( GeoPartList::const_iterator k = shape.getParts().begin(); k != shape.getParts().end(); k++ )
    {
        double target_len = -DBL_MAX;

        for( int pass=0; pass<2; pass++ )
        {
            const GeoPointList& part = *k;
            unsigned int wall_part_ptr = wall_vert_ptr;
            unsigned int roof_part_ptr = roof_vert_ptr;
            double part_len = 0.0;

            for( GeoPointList::const_iterator m = part.begin(); m != part.end(); m++ )
            {
                if ( pass == 0 )
                {
                    osg::Vec3d m_world = *m * srs->getInverseReferenceFrame();
                    if ( srs && srs->isGeocentric() )
                    {
                        osg::Vec3d p_vec = m_world;
                        osg::Vec3d e_vec = p_vec;
                        e_vec.normalize();
                        p_vec = p_vec + (e_vec * height);
                        double p_ex_len = p_vec.length();
                        if ( p_ex_len > target_len )
                            target_len = p_ex_len;
                    }
                    else
                    {
                        if ( m_world.z() + height > target_len )
                        {
                            target_len = m_world.z() + height;
                        }
                    }
                }
                else // if ( pass == 1 )
                {
                    osg::Vec3d extrude_vec;

                    if ( srs )
                    {
                        osg::Vec3d m_world = *m * srs->getInverseReferenceFrame();
                        if ( srs->isGeocentric() )
                        {
                            osg::Vec3d p_vec = m_world;
                            osg::Vec3d e_vec = p_vec;
                            e_vec.normalize();
                            double gap_len = target_len - (p_vec+(e_vec*height)).length();

                            double p_len = p_vec.length();
                            double ratio = target_len/p_len;
                            p_vec *= ratio;

                            extrude_vec = p_vec * srs->getReferenceFrame();
                        }
                        else
                        {
                            extrude_vec.set( m_world.x(), m_world.y(), target_len );
                            extrude_vec = extrude_vec * srs->getReferenceFrame();
                        }
                    }
                    else
                    {
                        extrude_vec.set( m->x(), m->y(), target_len );
                    }

                    if ( rooflines )
                    {
                        (*roof_colors)[roof_vert_ptr] = color;
                        (*roof_verts)[roof_vert_ptr++] = extrude_vec;
                    }

                    //double height = (extrude_vec - *m).length();
                     
                    part_len += wall_vert_ptr > wall_part_ptr?
                        (extrude_vec - (*verts)[wall_vert_ptr-2]).length() :
                        0.0;

                    double h;
                    if ( tex_repeats_y ) {
                        h = (extrude_vec - *m).length();
                        h += (tex_height_m - fmod( h, tex_height_m ) );
                    }
                    else {
                        h = tex_height_m;
                    }
                    int p;

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = extrude_vec;
                    (*texcoords)[p].set( part_len/tex_width_m, h/tex_height_m );

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = *m;
                    (*texcoords)[p].set( part_len/tex_width_m, 0.0f );
                }
            }

            if ( pass == 1 )
            {
                // close the wall if it's a poly:
                if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
                {
                    part_len += wall_vert_ptr > wall_part_ptr?
                        ((*verts)[wall_part_ptr] - (*verts)[wall_vert_ptr-2]).length() :
                        0.0;

                    double h;
                    if ( tex_repeats_y ) {
                        h = ((*verts)[wall_part_ptr] - (*verts)[wall_part_ptr+1]).length();
                        h += (tex_height_m - fmod( h, tex_height_m ) );
                    }
                    else {
                        h = 1.0;
                    }

                    int p;

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = (*verts)[wall_part_ptr];
                    (*texcoords)[p].set( part_len/tex_width_m, h );

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = (*verts)[wall_part_ptr+1];
                    (*texcoords)[p].set( part_len/tex_width_m, 0.0f );
                }

                walls->addPrimitiveSet( new osg::DrawArrays(
                    prim_type,
                    wall_part_ptr, wall_vert_ptr - wall_part_ptr ) );

                if ( rooflines )
                {
                    rooflines->addPrimitiveSet( new osg::DrawArrays(
                        osg::PrimitiveSet::LINE_LOOP,
                        roof_part_ptr, roof_vert_ptr - roof_part_ptr ) );
                }
            }
        }
    }

    return made_geom;
}


SkinResource*
ExtrudeGeomFilter::getWallSkinForFeature( Feature* f, FilterEnv* env )
{
    SkinResource* skin = NULL;
    bool batch = env->getProperties().getBoolValue( PROP_BATCH, false );
    if ( batch )
    {
        skin = dynamic_cast<SkinResource*>( env->getProperties().getRefValue( PROP_WALL_SKIN ) );
    }
    else if ( getWallSkinScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getWallSkinScript(), f, env );
        if ( r.isValid() )
        {
            skin = env->getSession()->getResources()->getSkin( r.asString() );
        }
    }
    return skin;
}

DrawableList
ExtrudeGeomFilter::process( FeatureList& input, FilterEnv* env )
{
    bool batch = input.size() > 1;
    env->setProperty( Property( PROP_BATCH, batch ) );

    if ( batch && getWallSkinScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getWallSkinScript(), env );
        if ( r.isValid() )
            env->setProperty( Property( PROP_WALL_SKIN, r.asRef() ) );
    }

    return BuildGeomFilter::process( input, env );
}


DrawableList
ExtrudeGeomFilter::process( Feature* input, FilterEnv* env )
{
    DrawableList output;

    // calcuate feature extent in the SRS, which we'll need for texture coordinates.
    GeoExtent abs_feature_extent(
        input->getExtent().getSouthwest().getAbsolute(),
        input->getExtent().getNortheast().getAbsolute() );

    osg::Vec4 color = getColorForFeature( input, env );

    for( GeoShapeList::const_iterator j = input->getShapes().begin(); j != input->getShapes().end(); j++ )
    {
        const GeoShape& shape = *j;

        double height = 0.0;
        
        // first try the height expression (takes precedence)
        if ( getHeightScript() )
        {
            ScriptResult r = env->getScriptEngine()->run( getHeightScript(), input, env );
            height = r.isValid()? r.asDouble( height ) : height;
        }

        // establish the wall skin resource:
        SkinResource* skin = getWallSkinForFeature( input, env );

        osg::ref_ptr<osg::Geometry> walls = new osg::Geometry();
        osg::ref_ptr<osg::Geometry> rooflines = NULL;
        
        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        {
            rooflines = new osg::Geometry();
        }

        if ( extrudeWallsUp( shape, env->getInputSRS(), height, walls.get(), rooflines.get(), color, skin ) )
        {      
            if ( skin )
            {
                walls->setStateSet( env->getSession()->getResources()->getStateSet( skin ) );
                env->getSession()->markResourceUsed( skin );
            }

            // generate per-vertex normals
            // todo: replace this nonsense
            osgUtil::SmoothingVisitor smoother;
            smoother.smooth( *(walls.get()) ); 

            output.push_back( walls.get() );

            // tessellate and add the roofs if necessary:
            if ( rooflines.valid() )
            {
                osgUtil::Tessellator tess;
                tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
                tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
                tess.retessellatePolygons( *(rooflines.get()) );

                // generate/smooth the normals.. TODO: replace this maybe
                smoother.smooth( *(rooflines.get()) );

                // texture the rooflines if necessary
                applyOverlayTexturing( rooflines.get(), input, env );

                //genTextureCoords( rooflines.get(), abs_feature_extent, env );
                //applyRasterTexture( rooflines.get(), abs_feature_extent, input, env );

                output.push_back( rooflines.get() );
            }
        }   
    }

    return output;
}
