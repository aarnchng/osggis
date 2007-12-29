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
#include <osgGIS/ExpressionEvaluator>
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



#define FRAND (((double)(rand()%100))/100.0)

ExtrudeGeomFilter::ExtrudeGeomFilter()
{
    options = 0;
    min_height = 1.0;
    max_height = 2.0;
    tex_index = 0;
    randomize_facade_textures = false;
}


ExtrudeGeomFilter::ExtrudeGeomFilter( const int& _options )
{
    options = _options;
}


ExtrudeGeomFilter::ExtrudeGeomFilter(const osg::Vec4f& color,
                                     double height )
{
    overall_color  = color;
    options        = 0;
    min_height     = 1.0;
    max_height     = 2.0;
}


ExtrudeGeomFilter::~ExtrudeGeomFilter()
{
    //NOP
}

void
ExtrudeGeomFilter::setHeightExpr( const std::string& value )
{
    height_expr = value;
    height_functor = NULL;
}

const std::string&
ExtrudeGeomFilter::getHeightExpr() const
{
    return height_expr;
}

void
ExtrudeGeomFilter::setRandomizeHeights( bool value )
{
    options = value?
        options | RANDOMIZE_HEIGHTS :
        options & ~RANDOMIZE_HEIGHTS;

    if ( value )
    { 
        struct RandomHeightFunctor : FeatureFunctor<double> {
            RandomHeightFunctor( ExtrudeGeomFilter* _e ) : e( _e ) { }
            double get( Feature* f ) {
                return e->getMinHeight() + FRAND * ( e->getMaxHeight() - e->getMinHeight() );
            }
            ExtrudeGeomFilter* e;
        };

        height_functor = new RandomHeightFunctor( this );
    }
    else
    {
        height_functor = NULL;
    }
}

bool
ExtrudeGeomFilter::getRandomizeHeights() const
{
    return ( options & RANDOMIZE_HEIGHTS ) != 0;
}

void
ExtrudeGeomFilter::setMinHeight( double value )
{
    min_height = value;
}

double
ExtrudeGeomFilter::getMinHeight() const
{
    return min_height;
}

void
ExtrudeGeomFilter::setMaxHeight( double value )
{
    max_height = value;
}

double
ExtrudeGeomFilter::getMaxHeight() const
{
    return max_height;
}

void
ExtrudeGeomFilter::setRandomizeFacadeTextures( bool value )
{
    randomize_facade_textures = value;
}

bool
ExtrudeGeomFilter::getRandomizeFacadeTextures() const
{
    return randomize_facade_textures;
}

void
ExtrudeGeomFilter::setProperty( const Property& p )
{
    if ( p.getName() == "height" )
        setHeightExpr( p.getValue() );
    else if ( p.getName() == "min_height" )
        setMinHeight( p.getDoubleValue( getMinHeight() ) );
    else if ( p.getName() == "max_height" )
        setMaxHeight( p.getDoubleValue( getMaxHeight() ) );
    else if ( p.getName() == "randomize_heights" )
        setRandomizeHeights( p.getBoolValue( getRandomizeHeights() ) );
    else if ( p.getName() == "randomize_facade_textures" )
        setRandomizeFacadeTextures( p.getBoolValue( getRandomizeFacadeTextures() ) );
    BuildGeomFilter::setProperty( p );
}


Properties
ExtrudeGeomFilter::getProperties() const
{
    Properties p = BuildGeomFilter::getProperties();
    p.push_back( Property( "height", getHeightExpr() ) );
    p.push_back( Property( "randomize_heights", getRandomizeHeights() ) );
    p.push_back( Property( "min_height", getMinHeight() ) );
    p.push_back( Property( "max_height", getMaxHeight() ) );
    p.push_back( Property( "randomize_facade_textures", getRandomizeFacadeTextures() ) );
    return p;
}

#define TEX_WIDTH_M 3
#define TEX_HEIGHT_M 4

static bool
extrudeWallsUp(const GeoShape&         shape, 
               const SpatialReference* srs, 
               double                  height, 
               osg::Geometry*          walls,
               osg::Geometry*          rooflines,
               const osg::Vec4&        color )
{
    bool made_geom = true;

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

                    double h = (extrude_vec - *m).length();
                    int p;

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = extrude_vec;
                    //(*texcoords)[p].set( part_len/TEX_WIDTH_M, 1.0f );
                    (*texcoords)[p].set( part_len/TEX_WIDTH_M, h/TEX_HEIGHT_M );

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = *m;
                    (*texcoords)[p].set( part_len/TEX_WIDTH_M, 0.0f );
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

                    double h = ((*verts)[wall_part_ptr] - (*verts)[wall_part_ptr+1]).length();

                    int p;

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = (*verts)[wall_part_ptr];
                    //(*texcoords)[p].set( part_len/TEX_WIDTH_M, 1.0f );
                    (*texcoords)[p].set( part_len/TEX_WIDTH_M, h );

                    p = wall_vert_ptr++;
                    (*colors)[p] = color;
                    (*verts)[p] = (*verts)[wall_part_ptr+1];
                    (*texcoords)[p].set( part_len/TEX_WIDTH_M, 0.0f );
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


DrawableList
ExtrudeGeomFilter::process( FeatureList& input, FilterEnv* env )
{
    if ( getRandomizeFacadeTextures() )
    {
        // TEST: texturing
        int k = tex_index++ % 4;
        osg::Image* image = new osg::Image();
        image->setFileName( 
            k == 0? "g:/data/ttools/terrasim/large_win_tan_border.rgb" :
            k == 1? "g:/data/ttools/terrasim/blue_glass.rgb" :
            k == 2? "g:/data/ttools/terrasim/red_tan_brick_storefront.rgb" :
                    "g:/data/ttools/terrasim/cement_3x3win.rgb" );
        //osg::Texture2D* tex = new osg::Texture2D( image );
        active_tex = new osg::Texture2D( image );
        active_tex->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT );
        active_tex->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT );

        //osg::TexEnv* texenv = new osg::TexEnv();
        active_texenv = new osg::TexEnv();
        active_texenv->setMode( osg::TexEnv::MODULATE );

        //active_state = new osg::StateSet();
        //active_state->setTextureAttributeAndModes( 0, tex );
        //active_state->setTextureAttribute( 0, texenv );
    }

    return BuildGeomFilter::process( input, env );
}


DrawableList
ExtrudeGeomFilter::process( Feature* input, FilterEnv* env )
{
    DrawableList output;

    osg::Vec4 color = getColorForFeature( input );

    for( GeoShapeList::const_iterator j = input->getShapes().begin(); j != input->getShapes().end(); j++ )
    {
        const GeoShape& shape = *j;

        double height = 0.0;
        
        // first try the height expression (takes precedence)
        if ( height_expr.length() > 0 )
        {
            ExpressionEvaluator eval( input );
            if ( !eval.eval( height_expr, /*out*/height ) )
            {
                osg::notify(osg::WARN) 
                    << "Warning, height expression \"" << height_expr << "\" has an error" << std::endl;
            }
        }

        // next try the functor.. this may eventually go away as well..
        else if ( height_functor.valid() )
        {
            height = height_functor->get( input );
        }

        osg::ref_ptr<osg::Geometry> walls = new osg::Geometry();
        osg::ref_ptr<osg::Geometry> rooflines = NULL;
        
        if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        {
            rooflines = new osg::Geometry();
        }

        if ( extrudeWallsUp( shape, env->getInputSRS(), height, walls.get(), rooflines.get(), color ) )
        {                
            if ( active_tex.valid() ) //active_state.valid() )
            {
                //walls->getOrCreateStateSet()->merge( *(active_state.get()) ); // crashed the optimizer?!
                walls->getOrCreateStateSet()->setTextureAttributeAndModes( 0, active_tex.get(), osg::StateAttribute::ON );
                walls->getOrCreateStateSet()->setTextureAttribute( 0, active_texenv.get(), osg::StateAttribute::ON );
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
                smoother.smooth( *(rooflines.get()) );

                output.push_back( rooflines.get() );
            }
        }   
    }

    return output;
}