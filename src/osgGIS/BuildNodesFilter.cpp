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
BuildNodesFilter::setOptimizerOptions( int _value )
{
    optimizer_options = _value;
}


int
BuildNodesFilter::getOptimizerOptions() const
{
    return optimizer_options;
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

    if ( state_set.valid() )
    {
        geode->setStateSet( state_set.get() );
    }

    if ( options & CULL_BACKFACES )
    {
        geode->getOrCreateStateSet()->setAttributeAndModes(
            new osg::CullFace(),
            osg::StateAttribute::ON );
    }

    if ( options & DISABLE_LIGHTING )
    {
        geode->getOrCreateStateSet()->setMode(
            GL_LIGHTING,
            osg::StateAttribute::OFF );
    }

    if ( options & OPTIMIZE )
    {
        osgUtil::Optimizer opt;
        opt.optimize( result, optimizer_options );
    }

    osg::NodeList output;
    output.push_back( result );
    return output;
}
