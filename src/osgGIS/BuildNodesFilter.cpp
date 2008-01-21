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

#include <osgGIS/BuildNodesFilter>
#include <osg/Geode>
#include <osg/Depth>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/CullFace>
#include <osg/MatrixTransform>
#include <osg/ClusterCullingCallback>
#include <osg/CullSettings>
#include <osg/PolygonOffset>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/Geometry>
#include <osg/TriangleFunctor>
#include <osgUtil/Optimizer>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BuildNodesFilter );


BuildNodesFilter::BuildNodesFilter()
{
    init( OPTIMIZE );
}


BuildNodesFilter::BuildNodesFilter( int _options )
{
    init( _options );
}


void
BuildNodesFilter::init( int _options )
{
    options = _options;
    optimizer_options = 
        osgUtil::Optimizer::ALL_OPTIMIZATIONS &
        (~osgUtil::Optimizer::TEXTURE_ATLAS_BUILDER); // no texture atlases please
    line_width = 0.0f;
    point_size = 0.0f;
    draw_cluster_culling_normals = false;
}


BuildNodesFilter::~BuildNodesFilter()
{
}  

void 
BuildNodesFilter::setCullBackfaces( bool value )
{
    options = value?
        options | CULL_BACKFACES :
        options & ~CULL_BACKFACES;
}

bool
BuildNodesFilter::getCullBackfaces() const
{
    return ( options & CULL_BACKFACES ) != 0;
}

void 
BuildNodesFilter::setApplyClusterCulling( bool value )
{
    options = value?
        options | APPLY_CLUSTER_CULLING :
        options & ~APPLY_CLUSTER_CULLING;
}

bool
BuildNodesFilter::getApplyClusterCulling() const
{
    return ( options & APPLY_CLUSTER_CULLING ) != 0;
} 

void
BuildNodesFilter::setDisableLighting( bool value )
{
    options = value?
        options | DISABLE_LIGHTING :
        options & ~DISABLE_LIGHTING;
}

bool
BuildNodesFilter::getDisableLighting() const
{
    return ( options & DISABLE_LIGHTING ) != 0;
}

void
BuildNodesFilter::setOptimize( bool value )
{
    options = value?
        options | OPTIMIZE :
        options & ~OPTIMIZE;
}

bool 
BuildNodesFilter::getOptimize() const
{
    return ( options & OPTIMIZE ) != 0;
}      

void
BuildNodesFilter::setOptimizerOptions( int value )
{
    optimizer_options = value;
}

int 
BuildNodesFilter::getOptimizerOptions() const
{
    return optimizer_options;
}

void
BuildNodesFilter::setLineWidth( float value )
{
    line_width = value;
}

float 
BuildNodesFilter::getLineWidth() const
{
    return line_width;
}

void
BuildNodesFilter::setPointSize( float value )
{
    point_size = value;
}

float
BuildNodesFilter::getPointSize() const
{
    return point_size;
}

void
BuildNodesFilter::setDrawClusterCullingNormals( bool value )
{
    draw_cluster_culling_normals = value;
}

bool
BuildNodesFilter::getDrawClusterCullingNormals() const
{
    return draw_cluster_culling_normals;
}

void
BuildNodesFilter::setProperty( const Property& p )
{
    if ( p.getName() == "optimize" )
        setOptimize( p.getBoolValue( getOptimize() ) );
    else if ( p.getName() == "cull_backfaces" )
        setCullBackfaces( p.getBoolValue( getCullBackfaces() ) );
    else if ( p.getName() == "apply_cluster_culling" )
        setApplyClusterCulling( p.getBoolValue( getApplyClusterCulling() ) );
    else if ( p.getName() == "disable_lighting" )
        setDisableLighting( p.getBoolValue( getDisableLighting() ) );
    else if ( p.getName() == "optimizer_options" )
        setOptimizerOptions( p.getIntValue( getOptimizerOptions() ) );
    else if ( p.getName() == "line_width" )
        setLineWidth( p.getFloatValue( getLineWidth() ) );
    else if ( p.getName() == "point_size" )
        setPointSize( p.getFloatValue( getPointSize() ) );
    else if ( p.getName() == "draw_cluster_culling_normals" )
        setDrawClusterCullingNormals( p.getBoolValue( getDrawClusterCullingNormals() ) );

    NodeFilter::setProperty( p );
}


Properties
BuildNodesFilter::getProperties() const
{
    Properties p = NodeFilter::getProperties();
    p.push_back( Property( "optimize", getOptimize() ) );
    p.push_back( Property( "cull_backfaces", getCullBackfaces() ) );
    p.push_back( Property( "apply_cluster_culling", getApplyClusterCulling() ) );
    p.push_back( Property( "disable_lighting", getDisableLighting() ) );
    p.push_back( Property( "optimizer_options", getOptimizerOptions() ) );
    p.push_back( Property( "line_width", getLineWidth() ) );
    p.push_back( Property( "point_size", getPointSize() ) );
    p.push_back( Property( "draw_cluster_culling_normals", getDrawClusterCullingNormals() ) );
    return p;
}


osg::NodeList
BuildNodesFilter::process( DrawableList& input, FilterEnv* env )
{
    osg::Node* result = NULL;

    osg::Geode* geode = new osg::Geode();
    for( DrawableList::iterator i = input.begin(); i != input.end(); i++ )
    {
        geode->addDrawable( i->get() );
    }

    result = geode;

    // NEXT create a XFORM if there's a localization matrix in the SRS:
    const SpatialReference* input_srs = env->getInputSRS();

    if ( env->getExtent().getArea() > 0 &&
         //input_srs->isGeocentric() &&
         !input_srs->getReferenceFrame().isIdentity() )
    {
        osg::Vec3d centroid( 0, 0, 0 );
        osg::Matrixd irf = input_srs->getInverseReferenceFrame();
        osg::Vec3d centroid_abs = centroid * irf;
        osg::MatrixTransform* xform = new osg::MatrixTransform( irf );
        xform->addChild( geode );
        result = xform;

        if ( getApplyClusterCulling() && input_srs->isGeocentric() )
        {    
            osg::Vec3d control_point = centroid_abs;
            osg::Vec3d normal = centroid_abs;
            normal.normalize();
            osg::BoundingSphere bs = geode->computeBound(); // force it
            float radius = bs.radius();
            float deviation = 
                (float) -atan( radius / input_srs->getBasisEllipsoid().getSemiMajorAxis() );

            osg::ClusterCullingCallback* ccc = new osg::ClusterCullingCallback();
            ccc->set(
                control_point,
                normal,
                deviation,
                radius );            

            osg::Group* cull_group = new osg::Group();
            cull_group->setCullCallback( ccc );
            cull_group->addChild( xform );
            result = cull_group;

            if ( getDrawClusterCullingNormals() == true )
            {
                //DRAW CLUSTER-CULLING NORMALS
                osg::Geometry* g = new osg::Geometry();
                osg::Vec3Array* v = new osg::Vec3Array(2);
                (*v)[0] = centroid; (*v)[1] = (centroid_abs + (normal*radius)) * input_srs->getReferenceFrame();
                g->setVertexArray( v );
                osg::Vec4Array* c = new osg::Vec4Array(1);
                (*c)[0] = osg::Vec4f( 0,1,0,1 );
                g->setColorArray( c );
                g->setColorBinding( osg::Geometry::BIND_OVERALL );
                g->addPrimitiveSet( new osg::DrawArrays( osg::PrimitiveSet::LINES, 0, 2 ) );
                geode->addDrawable( g );
            }
        }
    }

    if ( getCullBackfaces() )
    {
        geode->getOrCreateStateSet()->setAttributeAndModes(
            new osg::CullFace(),
            osg::StateAttribute::ON );
    }

    if ( getDisableLighting() )
    {
        geode->getOrCreateStateSet()->setMode(
            GL_LIGHTING,
            osg::StateAttribute::OFF );
    }

    if ( getLineWidth() > 0.0f )
    {
        geode->getOrCreateStateSet()->setAttribute(
            new osg::LineWidth( line_width ),
            osg::StateAttribute::ON );
    }

    if ( getPointSize() > 0.0f )
    {
        osg::Point* point = new osg::Point();
        point->setSize( point_size );
        geode->getOrCreateStateSet()->setAttribute(
            point,
            osg::StateAttribute::ON );
    }

    if ( getOptimize() )
    {
        osgUtil::Optimizer opt;
        // disable texture atlases, since they mess with our shared skin resources and
        // don't work correctly during multi-threaded building
        int opt_mask = getOptimizerOptions() & (~osgUtil::Optimizer::TEXTURE_ATLAS_BUILDER);
        opt.optimize( result, opt_mask );
    }

    osg::NodeList output;
    output.push_back( result );
    return output;
}
