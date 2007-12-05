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
}


ExtrudeGeomFilter::ExtrudeGeomFilter( const int& _options )
{
    options = _options;
    //setHeight( 1.0 );
}


ExtrudeGeomFilter::ExtrudeGeomFilter(const osg::Vec4f& color,
                                     double height )
{
    overall_color  = color;
    //overall_height = height;
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

//void
//ExtrudeGeomFilter::setHeight( double height )
//{
//    overall_height = height;
//    height_functor = NULL;
//}
//
//double 
//ExtrudeGeomFilter::getHeight() const
//{
//    return overall_height;
//}

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
    return p;
}


static bool
extrudeWallsUp(const GeoShape&         shape, 
               const SpatialReference* srs, 
               double                  height, 
               osg::Geometry*          walls,
               osg::Geometry*          rooflines )
{
    bool made_geom = true;

    int point_count = shape.getTotalPointCount();
    int num_verts = 2 * point_count;
    if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
        num_verts += 2 * shape.getPartCount();

    osg::Vec3Array* verts = new osg::Vec3Array( num_verts );
    walls->setVertexArray( verts );

    osg::Vec3Array* roof_verts = NULL;
    if ( rooflines )
    {
        roof_verts = new osg::Vec3Array( point_count );
        rooflines->setVertexArray( roof_verts );
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

            for( GeoPointList::const_iterator m = part.begin(); m != part.end(); m++ )
            {
                if ( pass == 0 )
                {
                    if ( srs && srs->isGeocentric() )
                    {
                        osg::Vec3d p_vec = *m * srs->getInverseReferenceFrame();
                        osg::Vec3d e_vec = p_vec;
                        e_vec.normalize();
                        p_vec = p_vec + (e_vec * height);
                        double p_ex_len = p_vec.length();
                        if ( p_ex_len > target_len )
                            target_len = p_ex_len;
                    }
                    else
                    {
                        if ( m->z() + height > target_len )
                        {
                            target_len = m->z() + height;
                        }
                    }
                }
                else // if ( pass == 1 )
                {
                    osg::Vec3d extrude_vec;

                    if ( srs )
                    {
                        if ( srs->isGeocentric() )
                        {
                            osg::Vec3d p_vec = *m * srs->getInverseReferenceFrame();
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
                            extrude_vec.set( m->x(), m->y(), target_len );
                            extrude_vec = extrude_vec * srs->getReferenceFrame();
                        }
                    }
                    else
                    {
                        extrude_vec.set( m->x(), m->y(), target_len );
                    }

                    if ( rooflines )
                    {
                        (*roof_verts)[roof_vert_ptr++] = extrude_vec;
                    }

                    (*verts)[wall_vert_ptr++] = extrude_vec;
                    (*verts)[wall_vert_ptr++] = *m;
                }
            }

            if ( pass == 1 )
            {
                // close the wall if it's a poly:
                if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
                {
                    (*verts)[wall_vert_ptr++] = (*verts)[wall_part_ptr];
                    (*verts)[wall_vert_ptr++] = (*verts)[wall_part_ptr+1];
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
    DrawableList output;

    for( FeatureList::const_iterator i = input.begin(); i != input.end(); i++ )
    {
        Feature* f = i->get();

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array( 1 );
        (*colors)[0] = getColorForFeature( f );

        for( GeoShapeList::const_iterator j = f->getShapes().begin(); j != f->getShapes().end(); j++ )
        {
            const GeoShape& shape = *j;
            
            //double height = 
            //    height_attr.length() > 0? f->getAttribute( height_attr ).asDouble() :
            //    height_functor.valid()? height_functor->get( f ) : 
            //    overall_height;

            //height *= height_scale;

            double height = 0.0;
            
            // first try the height expression (takes precedence)
            if ( height_expr.length() > 0 )
            {
                ExpressionEvaluator eval( f );
                if ( !eval.eval( height_expr, /*out*/height ) )
                {
                    osg::notify(osg::WARN) 
                        << "Warning, expression " << height_expr << " has an error" << std::endl;
                }
            }

            // next try the functor.. this may eventually go away as well..
            else if ( height_functor.valid() )
            {
                height = height_functor->get( f );
            }

            osg::ref_ptr<osg::Geometry> walls = new osg::Geometry();
            osg::ref_ptr<osg::Geometry> rooflines = NULL;
            
            if ( shape.getShapeType() == GeoShape::TYPE_POLYGON )
            {
                rooflines = new osg::Geometry();
            }

            if ( extrudeWallsUp( shape, env->getInputSRS(), height, walls.get(), rooflines.get() ) )
            {
                walls->setColorArray( colors.get() );
                walls->setColorBinding( osg::Geometry::BIND_OVERALL );
                
                // generate per-vertex normals:
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
                    
                    rooflines->setColorArray( colors.get() );
                    rooflines->setColorBinding( osg::Geometry::BIND_OVERALL );

                    output.push_back( rooflines.get() );
                }
            }
        }
    }

    return output;
}