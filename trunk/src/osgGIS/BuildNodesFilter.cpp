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
#include <osgUtil/Optimizer>

using namespace osgGIS;


BuildNodesFilter::BuildNodesFilter()
{
    options = (Options)0;
    optimizer_options = osgUtil::Optimizer::ALL_OPTIMIZATIONS;
}


BuildNodesFilter::BuildNodesFilter( int _options )
{
    options = _options;
    optimizer_options = osgUtil::Optimizer::ALL_OPTIMIZATIONS;
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
    return p;
}


osg::NodeList
BuildNodesFilter::process( DrawableList& input, FilterEnv* env )
{
    osg::Node* result = NULL;

    osg::Geode* geode = new osg::Geode();
    for( DrawableList::iterator& i = input.begin(); i != input.end(); i++ )
    {
        geode->addDrawable( i->get() );
    }

    result = geode;

    // NEXT create a XFORM if there's a localization matrix in the SRS:
    const SpatialReference* input_srs = env->getInputSRS();

    if ( env->getExtent().getArea() > 0 &&
         input_srs->isGeocentric() &&
         !input_srs->getReferenceFrame().isIdentity() )
    {
        osg::Vec3d centroid( 0, 0, 0 );
        osg::Matrixd irf = input_srs->getInverseReferenceFrame();
        osg::Vec3d centroid_abs = centroid * irf;
        osg::MatrixTransform* xform = new osg::MatrixTransform( irf );
        xform->addChild( geode );

        if ( options & APPLY_CLUSTER_CULLING )
        {
            // add a cluster culler:
            double deviation = -0.65;
            osg::Vec3 normal = centroid_abs;
            normal.normalize();

            osg::ClusterCullingCallback* ccc = new osg::ClusterCullingCallback(
                centroid,
                normal,
                deviation );
            ccc->setRadius( geode->getBound().radius() ); // why is this -1?

            xform->setCullCallback( ccc );
        }
        
        result = xform;
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

    if ( getOptimize() )
    {
        osgUtil::Optimizer opt;
        opt.optimize( result, getOptimizerOptions() );
    }

    osg::NodeList output;
    output.push_back( result );
    return output;
}
