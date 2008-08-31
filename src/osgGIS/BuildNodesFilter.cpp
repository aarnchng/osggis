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

#include <osgGIS/BuildNodesFilter>
#include <osgGIS/Utils>
#include <osgGIS/GeometryCleaner>
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
#include <osgSim/ShapeAttribute>
#include <osgUtil/Optimizer>
#include <sstream>
#include <iomanip>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( BuildNodesFilter );

#define DEFAULT_RASTER_OVERLAY_MAX_SIZE  0
#define DEFAULT_POINT_SIZE               0.0f
#define DEFAULT_LINE_WIDTH               0.0f
#define DEFAULT_CULL_BACKFACES           true
#define DEFAULT_APPLY_CLUSTER_CULLING    false
#define DEFAULT_DISABLE_LIGHTING         false
#define DEFAULT_OPTIMIZE                 true
#define DEFAULT_EMBED_ATTRIBUTES         false

BuildNodesFilter::BuildNodesFilter()
{
    init();
}

BuildNodesFilter::BuildNodesFilter( const BuildNodesFilter& rhs )
: NodeFilter( rhs ),
  line_width( rhs.line_width ),
  point_size( rhs.point_size ),
  draw_cluster_culling_normals( rhs.draw_cluster_culling_normals ),
  raster_overlay_script( rhs.raster_overlay_script.get() ),
  raster_overlay_max_size( rhs.raster_overlay_max_size ),
  cull_backfaces( rhs.cull_backfaces ),
  apply_cluster_culling( rhs.apply_cluster_culling ),
  disable_lighting( rhs.disable_lighting ),
  optimize( rhs.optimize ),
  embed_attrs( rhs.embed_attrs )
{
    //NOP
}

void
BuildNodesFilter::init()
{
    cull_backfaces               = DEFAULT_CULL_BACKFACES;
    apply_cluster_culling        = DEFAULT_APPLY_CLUSTER_CULLING;
    disable_lighting             = DEFAULT_DISABLE_LIGHTING;
    optimize                     = DEFAULT_OPTIMIZE;
    line_width                   = DEFAULT_LINE_WIDTH;
    point_size                   = DEFAULT_POINT_SIZE; 
    draw_cluster_culling_normals = false;
    raster_overlay_max_size      = DEFAULT_RASTER_OVERLAY_MAX_SIZE;
    embed_attrs                  = DEFAULT_EMBED_ATTRIBUTES;
}

BuildNodesFilter::~BuildNodesFilter()
{
    //NOP
}  

void 
BuildNodesFilter::setCullBackfaces( bool value )
{
    cull_backfaces = value;
}

bool
BuildNodesFilter::getCullBackfaces() const
{
    return cull_backfaces;
}

void 
BuildNodesFilter::setApplyClusterCulling( bool value )
{
    apply_cluster_culling = value;
}

bool
BuildNodesFilter::getApplyClusterCulling() const
{
    return apply_cluster_culling;
} 

void
BuildNodesFilter::setDisableLighting( bool value )
{
    disable_lighting = value;
}

bool
BuildNodesFilter::getDisableLighting() const
{
    return disable_lighting;
}

void
BuildNodesFilter::setOptimize( bool value )
{
    optimize = value;
}

bool 
BuildNodesFilter::getOptimize() const
{
    return optimize;
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
BuildNodesFilter::setRasterOverlayScript( Script* value )
{
    raster_overlay_script = value;
}
        
Script*
BuildNodesFilter::getRasterOverlayScript() const
{
    return raster_overlay_script.get();
}

void
BuildNodesFilter::setRasterOverlayMaxSize( int value )
{
    raster_overlay_max_size = value;
}

int
BuildNodesFilter::getRasterOverlayMaxSize() const
{
    return raster_overlay_max_size;
}

void
BuildNodesFilter::setEmbedAttributes( bool value )
{
    embed_attrs = value;
}

bool 
BuildNodesFilter::getEmbedAttributes() const
{
    return embed_attrs;
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
    else if ( p.getName() == "line_width" )
        setLineWidth( p.getFloatValue( getLineWidth() ) );
    else if ( p.getName() == "point_size" )
        setPointSize( p.getFloatValue( getPointSize() ) );
    else if ( p.getName() == "raster_overlay" )
        setRasterOverlayScript( new Script( p.getValue() ) );
    else if ( p.getName() == "raster_overlay_max_size" )
        setRasterOverlayMaxSize( p.getIntValue( getRasterOverlayMaxSize() ) );
    else if ( p.getName() == "embed_attributes" )
        setEmbedAttributes( p.getBoolValue( getEmbedAttributes() ) );

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
    p.push_back( Property( "line_width", getLineWidth() ) );
    p.push_back( Property( "point_size", getPointSize() ) );
    if ( getRasterOverlayScript() )
        p.push_back( Property( "raster_overlay", getRasterOverlayScript()->getCode() ) );
    if ( getRasterOverlayMaxSize() != DEFAULT_RASTER_OVERLAY_MAX_SIZE )
        p.push_back( Property( "raster_overlay_max_size", getRasterOverlayMaxSize() ) );
    if ( getEmbedAttributes() != DEFAULT_EMBED_ATTRIBUTES )
        p.push_back( Property( "embed_attributes", getEmbedAttributes() ) );
    return p;
}

AttributedNodeList
BuildNodesFilter::process( FragmentList& input, FilterEnv* env )
{
    AttributedNodeList nodes;

    osg::Geode* geode = NULL;
    for( FragmentList::const_iterator i = input.begin(); i != input.end(); i++ )
    {
        Fragment* frag = i->get();

        AttributeList frag_attrs = frag->getAttributes();

        if ( !geode )
        {
            geode = new osg::Geode();
            nodes.push_back( new AttributedNode( geode, frag_attrs ) );
        }

        for( DrawableList::const_iterator d = frag->getDrawables().begin(); d != frag->getDrawables().end(); d++ )
        {
            geode->addDrawable( d->get() );
        }

        bool retire_geode = false;

        // if a fragment name is set, apply it
        if ( frag->hasName() )
        {
            geode->addDescription( frag->getName() );
            retire_geode = true;
        }

        if ( getEmbedAttributes() )
        {
            embedAttributes( geode, frag_attrs );
            retire_geode = true;
        }

        // If required, reset the geode point so that the next fragment gets placed in a new geode.
        if ( retire_geode )
        {
            geode = NULL;
        }
    }

    // with multiple geodes or fragment names, disable geode combining to preserve the node decription.
    if ( nodes.size() > 1  )
    {
        env->getOptimizerHints().exclude( osgUtil::Optimizer::MERGE_GEODES );
    }

    return process( nodes, env );
}

AttributedNodeList
BuildNodesFilter::process( AttributedNodeList& input, FilterEnv* env )
{
    osg::ref_ptr<osg::Node> result;

    if ( input.size() > 1 )
    {
        result = new osg::Group();
        for( AttributedNodeList::iterator i = input.begin(); i != input.end(); i++ )
        {
            osg::Node* node = i->get()->getNode();
            if ( node )
            {
                if ( getEmbedAttributes() )
                    embedAttributes( node, i->get()->getAttributes() );

                result->asGroup()->addChild( node );
            }
        }
    }
    else if ( input.size() == 1 )
    {
        result = input[0]->getNode();

        if ( getEmbedAttributes() )
            embedAttributes( result.get(), input[0]->getAttributes() );
    }
    else
    {
        return AttributedNodeList();
    }

    // if there are no drawables or external refs, toss it.
    if ( !GeomUtils::hasDrawables( result.get() ) )
    {
        return AttributedNodeList();
    }

    // NEXT create a XFORM if there's a localization matrix in the SRS. This will
    // prevent jittering due to loss of precision.
    const SpatialReference* input_srs = env->getInputSRS();

    if ( env->getExtent().getArea() > 0 && !input_srs->getReferenceFrame().isIdentity() )
    {
        osg::Vec3d centroid( 0, 0, 0 );
        osg::Matrixd irf = input_srs->getInverseReferenceFrame();
        osg::Vec3d centroid_abs = centroid * irf;
        osg::MatrixTransform* xform = new osg::MatrixTransform( irf );

        xform->addChild( result.get() );
        result = xform;

        if ( getApplyClusterCulling() && input_srs->isGeocentric() )
        {    
            osg::Vec3d normal = centroid_abs;
            normal.normalize();
            
            //osg::BoundingSphere bs = result->computeBound(); // force it            
            // radius = distance from centroid inside which to disable CC altogether:
            //float radius = bs.radius();
            //osg::Vec3d control_point = bs.center();

            osg::Vec3d control_point = centroid_abs;
            GeoPoint env_cen = input_srs->transform( env->getCellExtent().getCentroid() );
            GeoPoint env_sw  = input_srs->transform( env->getCellExtent().getSouthwest() );
            float radius = (env_cen-env_sw).length();

            // dot product: 0 = orthogonal to normal, -1 = equal to normal
            float deviation = -radius/input_srs->getEllipsoid().getSemiMinorAxis();
            
            osg::ClusterCullingCallback* ccc = new osg::ClusterCullingCallback();
            ccc->set( control_point, normal, deviation, radius );

            osg::Group* cull_group = new osg::Group();
            cull_group->setCullCallback( ccc );
            cull_group->addChild( xform );
            result = cull_group;


            //osgGIS::notify(osg::NOTICE) << "CCC: radius = " << radius << ", deviation = " << deviation << std::endl;


            //if ( getDrawClusterCullingNormals() == true )
            //{
            //    //DRAW CLUSTER-CULLING NORMALS
            //    osg::Geode* geode = new osg::Geode();
            //    osg::Geometry* g = new osg::Geometry();
            //    osg::Vec3Array* v = new osg::Vec3Array(2);
            //    (*v)[0] = control_point; (*v)[1] = control_point + (normal*radius);
            //    g->setVertexArray( v );
            //    osg::Vec4Array* c = new osg::Vec4Array(1);
            //    (*c)[0] = osg::Vec4f( 0,1,0,1 );
            //    g->setColorArray( c );
            //    g->setColorBinding( osg::Geometry::BIND_OVERALL );
            //    g->addPrimitiveSet( new osg::DrawArrays( osg::PrimitiveSet::LINES, 0, 2 ) );
            //    geode->addDrawable( g );
            //    cull_group->addChild( geode );
            //}
        }
    }

    if ( getCullBackfaces() )
    {
        result->getOrCreateStateSet()->setAttributeAndModes( new osg::CullFace(), osg::StateAttribute::ON );
    }

    if ( getDisableLighting() )
    {
        result->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    }

    if ( getLineWidth() > 0.0f )
    {
        result->getOrCreateStateSet()->setAttribute( new osg::LineWidth( line_width ), osg::StateAttribute::ON );
    }

    if ( getPointSize() > 0.0f )
    {
        osg::Point* point = new osg::Point();
        point->setSize( point_size );
        result->getOrCreateStateSet()->setAttribute( point, osg::StateAttribute::ON );
    }

    if ( getRasterOverlayScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getRasterOverlayScript(), env );
        if ( r.isValid() )
        {
            RasterResource* raster = env->getSession()->getResources()->getRaster( r.asString() );
            if ( raster )
            {
                osg::Image* image = NULL;

                std::stringstream builder;

                std::string cell_id = env->getProperties().getValue( "compiler.cell_id", "" );
                if ( cell_id.length() > 0 )
                {
                    builder << "r" << cell_id << ".jpg";
                }
                else
                {
                    double x = env->getExtent().getCentroid().x();
                    double y = env->getExtent().getCentroid().y();
                    builder << std::setprecision(10) << "r" << x << "x" << y << ".jpg";
                }

                if ( raster->applyToStateSet( result->getOrCreateStateSet(), env->getExtent(), getRasterOverlayMaxSize(), &image ) )
                {
                    // Add this as a skin resource so the compiler can properly localize and deploy it.
                    image->setFileName( builder.str() );

                    env->getResourceCache()->addSkin( result->getOrCreateStateSet() );
                }
            }
        }
    }

    if ( getOptimize() )
    {
        osgUtil::Optimizer opt;
        int opt_mask = 
            osgUtil::Optimizer::DEFAULT_OPTIMIZATIONS |
            osgUtil::Optimizer::MERGE_GEODES |
            osgUtil::Optimizer::TRISTRIP_GEOMETRY |
            osgUtil::Optimizer::SPATIALIZE_GROUPS; //::ALL_OPTIMIZATIONS; //getOptimizerOptions();

        // disable texture atlases, since they mess with our shared skin resources and
        // don't work correctly during multi-threaded building
        opt_mask &= ~osgUtil::Optimizer::TEXTURE_ATLAS_BUILDER;

        // I've seen this crash the app when dealing with certain ProxyNodes.
        // TODO: investigate this later.
        opt_mask &= ~osgUtil::Optimizer::REMOVE_REDUNDANT_NODES;

        // integrate the optimizer hints:
        opt_mask |= env->getOptimizerHints().getIncludedOptions();
        opt_mask &= ~( env->getOptimizerHints().getExcludedOptions() );

        opt.optimize( result.get(), opt_mask );

        GeometryCleaner cleaner;
        cleaner.clean( result.get() );
    }

    AttributedNodeList output;
    output.push_back( new AttributedNode( result.get() ) );

    return output;
}
