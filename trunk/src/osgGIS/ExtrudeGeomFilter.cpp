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

#include <osgGIS/ExtrudeGeomFilter>
#include <osgGIS/Utils>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/Image>
#include <osgDB/ReadFile>
#include <osgUtil/Tessellator>
#include <float.h>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( ExtrudeGeomFilter );


#define DEFAULT_UNIFORM_HEIGHT true


ExtrudeGeomFilter::ExtrudeGeomFilter()
{
    setUniformHeight( DEFAULT_UNIFORM_HEIGHT );
}

ExtrudeGeomFilter::ExtrudeGeomFilter( const ExtrudeGeomFilter& rhs )
: BuildGeomFilter( rhs ),
  height_script( rhs.height_script.get() ),
  wall_skin_script( rhs.wall_skin_script.get() ),
  uniform_height( rhs.uniform_height )
{
    //NOP
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
ExtrudeGeomFilter::setUniformHeight( bool value )
{
    uniform_height = value;
}

bool
ExtrudeGeomFilter::getUniformHeight() const
{
    return uniform_height;
}

void
ExtrudeGeomFilter::setProperty( const Property& p )
{
    if ( p.getName() == "height" )
        setHeightScript( new Script( p.getValue() ) );
    else if ( p.getName() == "wall_skin" )
        setWallSkinScript( new Script( p.getValue() ) );
    else if ( p.getName() == "uniform_height" )
        setUniformHeight( p.getBoolValue( getUniformHeight() ) );
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
    if ( getUniformHeight() != DEFAULT_UNIFORM_HEIGHT )
        p.push_back( Property( "uniform_height", getUniformHeight() ) );
    return p;
}


//static bool
//extrudeWallsUp(const GeoShape&         shape, 
//               const SpatialReference* srs, 
//               double                  height,
//               bool                    uniform_height,
//               osg::Geometry*          walls,
//               osg::Geometry*          rooflines,
//               const osg::Vec4&        color,
//               SkinResource*           skin )
//{
//    bool made_geom = true;
//
//    double tex_width_m = skin? skin->getTextureWidthMeters() : 1.0;
//    double tex_height_m = skin? skin->getTextureHeightMeters() : 1.0;
//
//    //Adjust the texture height so it is a multiple of the extrusion height
//    bool   tex_repeats_y = skin? skin->getRepeatsVertically() : true;
//
//    int point_count = shape.getTotalPointCount();
//    int num_verts = 2 * point_count;
//    if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
//        num_verts += 2 * shape.getPartCount();
//
//    osg::Vec3Array* verts = new osg::Vec3Array( num_verts );
//    walls->setVertexArray( verts );
//
//    osg::Vec2Array* texcoords = new osg::Vec2Array( num_verts );
//    walls->setTexCoordArray( 0, texcoords );
//
//    osg::Vec4Array* colors = new osg::Vec4Array( num_verts );
//    walls->setColorArray( colors );
//    walls->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
//
//    osg::Vec3Array* normals = new osg::Vec3Array( num_verts );
//    walls->setNormalArray( normals );
//    walls->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
//
//    osg::Vec3Array* roof_verts = NULL;
//    osg::Vec4Array* roof_colors = NULL;
//    if ( rooflines )
//    {
//        roof_verts = new osg::Vec3Array( point_count );
//        rooflines->setVertexArray( roof_verts );
//
//        roof_colors = new osg::Vec4Array( point_count );
//        rooflines->setColorArray( roof_colors );
//        rooflines->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
//    }
//
//    int wall_vert_ptr = 0;
//    int roof_vert_ptr = 0;
//
//    GLenum prim_type = shape.getShapeType() == GeoShape::TYPE_POINT?
//        osg::PrimitiveSet::LINES :
//        osg::PrimitiveSet::TRIANGLE_STRIP;
//
//    for( GeoPartList::const_iterator k = shape.getParts().begin(); k != shape.getParts().end(); k++ )
//    {
//        double target_len = -DBL_MAX;
//        double max_height = -DBL_MAX;
//
//        osg::Vec3d min_loc(DBL_MAX, DBL_MAX, DBL_MAX);
//
//        double tex_height_m_adj = tex_height_m;
//
//        for( int pass=0; pass<2; pass++ )
//        {
//            const GeoPointList& part = *k;
//            unsigned int wall_part_ptr = wall_vert_ptr;
//            unsigned int roof_part_ptr = roof_vert_ptr;
//            double part_len = 0.0;
//
//            if (pass == 1)
//            {
//                double max_height = 0;
//                if (srs && srs->isGeocentric())
//                {
//                    max_height = target_len - min_loc.length();
//                }
//                else
//                {
//                    max_height = target_len - min_loc.z();
//                }
//
//                //Adjust the texture height so it is a multiple of the maximum height
//                double div = osg::round(max_height / tex_height_m);
//                //Prevent divide by zero
//                if (div == 0) div = 1;
//                tex_height_m_adj = max_height / div;
//            }
//
//            for( GeoPointList::const_iterator m = part.begin(); m != part.end(); m++ )
//            {
//                if ( pass == 0 )
//                {
//                    osg::Vec3d m_world = *m * srs->getInverseReferenceFrame();
//                    if ( srs && srs->isGeocentric() )
//                    {
//                        osg::Vec3d p_vec = m_world;
//                        osg::Vec3d e_vec = p_vec;
//                        e_vec.normalize();
//                        p_vec = p_vec + (e_vec * height);
//
//                        if (m_world.length() < min_loc.length())
//                        {
//                            min_loc = m_world;
//                        }
//
//                        double p_ex_len = p_vec.length();
//                        if ( p_ex_len > target_len )
//                        {
//                            target_len = p_ex_len;
//                        }
//                    }
//                    else
//                    {
//                        if ( m_world.z() + height > target_len )
//                        {
//                            target_len = m_world.z() + height;
//                        }
//
//                        if (m_world.z() < min_loc.z())
//                        {
//                            min_loc = m_world;
//                        }                     
//                    }
//                }
//                else // if ( pass == 1 )
//                {
//                    osg::Vec3d extrude_vec;
//
//                    if ( srs )
//                    {
//                        osg::Vec3d m_world = *m * srs->getInverseReferenceFrame();
//                        if ( srs->isGeocentric() )
//                        {
//                            osg::Vec3d p_vec = m_world;
//                            
//                            if ( uniform_height )
//                            {
//                                double p_len = p_vec.length();
//                                double ratio = target_len/p_len;
//                                p_vec *= ratio;
//                            }
//                            else
//                            {
//                                osg::Vec3d unit_vec = p_vec; 
//                                unit_vec.normalize();
//                                p_vec = p_vec + unit_vec*height;
//                            }
//
//                            extrude_vec = p_vec * srs->getReferenceFrame();
//                        }
//                        else
//                        {
//                            if ( uniform_height )
//                            {
//                                extrude_vec.set( m_world.x(), m_world.y(), target_len );
//                            }
//                            else
//                            {
//                                extrude_vec.set( m_world.x(), m_world.y(), m_world.z() + height );
//                            }
//                            extrude_vec = extrude_vec * srs->getReferenceFrame();
//                        }
//                    }
//                    else
//                    {
//                        extrude_vec.set( m->x(), m->y(), target_len );
//                    }
//
//                    if ( rooflines )
//                    {
//                        (*roof_colors)[roof_vert_ptr] = color;
//                        (*roof_verts)[roof_vert_ptr++] = extrude_vec;
//                    }
//
//                     
//                    part_len += wall_vert_ptr > wall_part_ptr?
//                        (extrude_vec - (*verts)[wall_vert_ptr-2]).length() :
//                        0.0;
//
//                    double h;
//                    if ( tex_repeats_y ) {
//                        h = -(extrude_vec - *m).length();
//                    }
//                    else {
//                        h = -tex_height_m_adj;
//                    }
//                    int p;
//
//                    p = wall_vert_ptr++;
//                    (*colors)[p] = color;
//                    (*verts)[p] = extrude_vec;
//                    (*texcoords)[p].set( part_len/tex_width_m, 0.0f );
//
//                    p = wall_vert_ptr++;
//                    (*colors)[p] = color;
//                    (*verts)[p] = *m;
//                    (*texcoords)[p].set( part_len/tex_width_m, h/tex_height_m_adj );
//                }
//            }
//
//            if ( pass == 1 )
//            {
//                // close the wall if it's a poly:
//                if ( shape.getShapeType() == GeoShape::TYPE_POLYGON && !part.isClosed() )
//                {
//                    part_len += wall_vert_ptr > wall_part_ptr?
//                        ((*verts)[wall_part_ptr] - (*verts)[wall_vert_ptr-2]).length() :
//                        0.0;
//
//                    int p;
//
//                    p = wall_vert_ptr++;
//                    (*colors)[p] = color;
//                    (*verts)[p] = (*verts)[wall_part_ptr];
//                    (*texcoords)[p].set( part_len/tex_width_m, (*texcoords)[wall_part_ptr].y() ); //h/tex_height_m );
//
//                    p = wall_vert_ptr++;
//                    (*colors)[p] = color;
//                    (*verts)[p] = (*verts)[wall_part_ptr+1];
//                    (*texcoords)[p].set( part_len/tex_width_m, (*texcoords)[wall_part_ptr+1].y() );
//                }
//
//                walls->addPrimitiveSet( new osg::DrawArrays(
//                    prim_type,
//                    wall_part_ptr, wall_vert_ptr - wall_part_ptr ) );
//
//                if ( rooflines )
//                {
//                    rooflines->addPrimitiveSet( new osg::DrawArrays(
//                        osg::PrimitiveSet::LINE_LOOP,
//                        roof_part_ptr, roof_vert_ptr - roof_part_ptr ) );
//                }
//            }
//        }
//    }
//
//    return made_geom;
//}


bool
ExtrudeGeomFilter::extrudeWallsUp(const GeoShape&         shape, 
                                  const SpatialReference* srs, 
                                  double                  height,
                                  bool                    uniform_height,
                                  osg::Geometry*          walls,
                                  osg::Geometry*          top_cap,
                                  osg::Geometry*          bottom_cap,
                                  const osg::Vec4&        color,
                                  SkinResource*           skin )
{
    bool made_geom = true;

    double tex_width_m = skin? skin->getTextureWidthMeters() : 1.0;
    double tex_height_m = skin? skin->getTextureHeightMeters() : 1.0;

    //Adjust the texture height so it is a multiple of the extrusion height
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

    osg::Vec3Array* normals = new osg::Vec3Array( num_verts );
    walls->setNormalArray( normals );
    walls->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );

    osg::Vec3Array* top_verts = NULL;
    osg::Vec4Array* top_colors = NULL;
    if ( top_cap )
    {
        top_verts = new osg::Vec3Array( point_count );
        top_cap->setVertexArray( top_verts );

        top_colors = new osg::Vec4Array( point_count );
        top_cap->setColorArray( top_colors );
        top_cap->setColorBinding( osg::Geometry::BIND_PER_VERTEX );
    }

    osg::Vec3Array* bottom_verts = NULL;
    if ( bottom_cap )
    {
        bottom_verts = new osg::Vec3Array( point_count );
        bottom_cap->setVertexArray( bottom_verts );
    }

    int wall_vert_ptr = 0;
    int top_vert_ptr = 0;
    int bottom_vert_ptr = 0;

    GLenum prim_type = shape.getShapeType() == GeoShape::TYPE_POINT?
        osg::PrimitiveSet::LINES :
        osg::PrimitiveSet::TRIANGLE_STRIP;

    double target_len = -DBL_MAX;
    osg::Vec3d min_loc(DBL_MAX, DBL_MAX, DBL_MAX);

    // first calculate the minimum Z across all parts:
    for( GeoPartList::const_iterator k = shape.getParts().begin(); k != shape.getParts().end(); k++ )
    {
        const GeoPointList& part = *k;

        for( GeoPointList::const_iterator m = part.begin(); m != part.end(); m++ )
        {
            osg::Vec3d m_world = *m * srs->getInverseReferenceFrame();
            if ( srs && srs->isGeocentric() )
            {
                osg::Vec3d p_vec = m_world;
                osg::Vec3d e_vec = p_vec;
                e_vec.normalize();
                p_vec = p_vec + (e_vec * height);

                if (m_world.length() < min_loc.length())
                {
                    min_loc = m_world;
                }

                double p_ex_len = p_vec.length();
                if ( p_ex_len > target_len )
                {
                    target_len = p_ex_len;
                }
            }
            else
            {
                if ( m_world.z() + height > target_len )
                {
                    target_len = m_world.z() + height;
                }

                if (m_world.z() < min_loc.z())
                {
                    min_loc = m_world;
                }                     
            }
        }
    }

    // now generate the extruded geometry.
    for( GeoPartList::const_iterator k = shape.getParts().begin(); k != shape.getParts().end(); k++ )
    {
        //double max_height = -DBL_MAX;
        double tex_height_m_adj = tex_height_m;

        const GeoPointList& part = *k;
        unsigned int wall_part_ptr = wall_vert_ptr;
        unsigned int top_part_ptr = top_vert_ptr;
        unsigned int bottom_part_ptr = bottom_vert_ptr;
        double part_len = 0.0;

        double max_height = 0;
        if (srs && srs->isGeocentric())
        {
            max_height = target_len - min_loc.length();
        }
        else
        {
            max_height = target_len - min_loc.z();
        }

        //Adjust the texture height so it is a multiple of the maximum height
        double div = osg::round(max_height / tex_height_m);
        //Prevent divide by zero
        if (div == 0) div = 1;
        tex_height_m_adj = max_height / div;

        for( GeoPointList::const_iterator m = part.begin(); m != part.end(); m++ )
        {
            osg::Vec3d extrude_vec;

            if ( srs )
            {
                osg::Vec3d m_world = *m * srs->getInverseReferenceFrame();
                if ( srs->isGeocentric() )
                {
                    osg::Vec3d p_vec = m_world;
                    
                    if ( uniform_height )
                    {
                        double p_len = p_vec.length();
                        double ratio = target_len/p_len;
                        p_vec *= ratio;
                    }
                    else
                    {
                        osg::Vec3d unit_vec = p_vec; 
                        unit_vec.normalize();
                        p_vec = p_vec + unit_vec*height;
                    }

                    extrude_vec = p_vec * srs->getReferenceFrame();
                }
                else
                {
                    if ( uniform_height )
                    {
                        extrude_vec.set( m_world.x(), m_world.y(), target_len );
                    }
                    else
                    {
                        extrude_vec.set( m_world.x(), m_world.y(), m_world.z() + height );
                    }
                    extrude_vec = extrude_vec * srs->getReferenceFrame();
                }
            }
            else
            {
                extrude_vec.set( m->x(), m->y(), target_len );
            }

            if ( top_cap )
            {
                (*top_colors)[top_vert_ptr] = color;
                (*top_verts)[top_vert_ptr++] = extrude_vec;
            }
            if ( bottom_cap )
            {
                (*bottom_verts)[bottom_vert_ptr++] = *m;
            }
             
            part_len += wall_vert_ptr > wall_part_ptr?
                (extrude_vec - (*verts)[wall_vert_ptr-2]).length() :
                0.0;

            double h;
            if ( tex_repeats_y ) {
                h = -(extrude_vec - *m).length();
            }
            else {
                h = -tex_height_m_adj;
            }
            int p;

            p = wall_vert_ptr++;
            (*colors)[p] = color;
            (*verts)[p] = extrude_vec;
            (*texcoords)[p].set( part_len/tex_width_m, 0.0f );

            p = wall_vert_ptr++;
            (*colors)[p] = color;
            (*verts)[p] = *m;
            (*texcoords)[p].set( part_len/tex_width_m, h/tex_height_m_adj );
        }

        // close the wall if it's a poly:
        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON && !part.isClosed() )
        {
            part_len += wall_vert_ptr > wall_part_ptr?
                ((*verts)[wall_part_ptr] - (*verts)[wall_vert_ptr-2]).length() :
                0.0;

            int p;

            p = wall_vert_ptr++;
            (*colors)[p] = color;
            (*verts)[p] = (*verts)[wall_part_ptr];
            (*texcoords)[p].set( part_len/tex_width_m, (*texcoords)[wall_part_ptr].y() ); //h/tex_height_m );

            p = wall_vert_ptr++;
            (*colors)[p] = color;
            (*verts)[p] = (*verts)[wall_part_ptr+1];
            (*texcoords)[p].set( part_len/tex_width_m, (*texcoords)[wall_part_ptr+1].y() );
        }

        walls->addPrimitiveSet( new osg::DrawArrays(
            prim_type,
            wall_part_ptr, wall_vert_ptr - wall_part_ptr ) );

        if ( top_cap )
        {
            top_cap->addPrimitiveSet( new osg::DrawArrays(
                osg::PrimitiveSet::LINE_LOOP,
                top_part_ptr, top_vert_ptr - top_part_ptr ) );
        }
        if ( bottom_cap )
        {
            // reverse the bottom verts:
            int len = bottom_vert_ptr - bottom_part_ptr;
            for( int i=bottom_part_ptr; i<len/2; i++ )
                std::swap( (*bottom_verts)[i], (*bottom_verts)[bottom_part_ptr+(len-1)-i] );

            bottom_cap->addPrimitiveSet( new osg::DrawArrays(
                osg::PrimitiveSet::LINE_LOOP,
                bottom_part_ptr, bottom_vert_ptr - bottom_part_ptr ) );
        }
    }

    return made_geom;
}


SkinResource*
ExtrudeGeomFilter::getWallSkinForFeature( Feature* f, FilterEnv* env )
{
    SkinResource* result = NULL;
    if ( batch_wall_skin.valid() )
    {
        result = batch_wall_skin.get();
    }
    else if ( getWallSkinScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getWallSkinScript(), f, env );
        if ( r.isValid() )
            result = env->getSession()->getResources()->getSkin( r.asString() );
        else
            env->getReport()->error( r.asString() );
    }
    return result;
}

FragmentList
ExtrudeGeomFilter::process( FeatureList& input, FilterEnv* env )
{
    batch_wall_skin = NULL;
    //is_batch = input.size() > 1;
    
    if ( input.size() > 1 && getWallSkinScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getWallSkinScript(), env );
        if ( r.isValid() )
            batch_wall_skin = dynamic_cast<SkinResource*>( r.asRef() );
        else
            env->getReport()->error( r.asString() );
    }

    return BuildGeomFilter::process( input, env );
}


FragmentList
ExtrudeGeomFilter::process( Feature* input, FilterEnv* env )
{
    FragmentList output;

    // calcuate feature extent in the SRS, which we'll need for texture coordinates.
    GeoExtent abs_feature_extent(
        input->getExtent().getSouthwest().getAbsolute(),
        input->getExtent().getNortheast().getAbsolute() );

    osg::Vec4 color = getColorForFeature( input, env );

    for( GeoShapeList::iterator j = input->getShapes().begin(); j != input->getShapes().end(); j++ )
    {
        GeoShape& shape = *j;

        double height = 0.0;
        
        // first try the height expression (takes precedence)
        if ( getHeightScript() )
        {
            ScriptResult r = env->getScriptEngine()->run( getHeightScript(), input, env );
            height = r.isValid()? r.asDouble( height ) : height;
            if ( !r.isValid() )
                env->getReport()->error( r.asString() );
        }

        // establish the wall skin resource:
        SkinResource* skin = getWallSkinForFeature( input, env );

        osg::ref_ptr<osg::Geometry> walls = new osg::Geometry();
        osg::ref_ptr<osg::Geometry> rooflines = NULL;
        
        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        {
            rooflines = new osg::Geometry();

            // prep the shapes by making sure all polys are open:
            for( GeoPartList::iterator i = shape.getParts().begin(); i != shape.getParts().end(); i++ )
            {
                GeomUtils::openPolygon( *i );
            }
        }

        if ( extrudeWallsUp( shape, env->getInputSRS(), height, getUniformHeight(), walls.get(), rooflines.get(), NULL, color, skin ) )
        {      
            if ( skin )
            {
                osg::StateSet* wall_ss = env->getResourceCache()->getStateSet( skin );
                if ( wall_ss )
                {
                    walls->setStateSet( wall_ss );
                }
                //env->getSession()->getResources()->getStateSet( skin ) );
                //env->getSession()->markResourceUsed( skin );
            }
            else
            {
                //There is no skin, so disable texturing for the walls to prevent other textures from being applied to the walls
                walls->getOrCreateStateSet()->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::OFF);
            }

            // generate per-vertex normals
            generateNormals( walls.get() );

            Fragment* new_fragment = new Fragment( walls.get() );

            // tessellate and add the roofs if necessary:
            if ( rooflines.valid() )
            {
                osgUtil::Tessellator tess;
                tess.setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
                tess.setWindingType( osgUtil::Tessellator::TESS_WINDING_POSITIVE );
                tess.retessellatePolygons( *(rooflines.get()) );

                // generate/smooth the normals.. TODO: replace this maybe
                generateNormals( rooflines.get() );
                //smoother.smooth( *(rooflines.get()) );

                // texture the rooflines if necessary
                applyOverlayTexturing( rooflines.get(), input, env );

                new_fragment->addDrawable( rooflines.get() );
            }

            applyFragmentName( new_fragment, input, env );

            output.push_back( new_fragment );
        }   
    }

    return output;
}

