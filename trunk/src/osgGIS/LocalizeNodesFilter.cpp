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

#include <osgGIS/LocalizeNodesFilter>
#include <osgGIS/Registry>
#include <osg/NodeVisitor>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osg/ClusterCullingCallback>
#include <osg/TriangleFunctor>
#include <osgText/Text>
#include <osgUtil/Optimizer>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( LocalizeNodesFilter );




LocalizeNodesFilter::LocalizeNodesFilter()
{
    //NOP
}

LocalizeNodesFilter::LocalizeNodesFilter( const LocalizeNodesFilter& rhs )
: NodeFilter( rhs )
{
    //NOP
}

LocalizeNodesFilter::~LocalizeNodesFilter()
{
    //NOP
}


// copied from osg::ClusterCullingCallback.cpp
struct ComputeDeviationFunctor
{
    ComputeDeviationFunctor():
        _deviation(1.0),
        _radius2(0.0) {}
        
    void set(const osg::Vec3& center,const osg::Vec3& normal)
    {
        _center = center;
        _normal = normal;
    }

    inline void operator() ( const osg::Vec3 &v1, const osg::Vec3 &v2, const osg::Vec3 &v3, bool)
    {
        // calc orientation of triangle.
        osg::Vec3 normal = (v2-v1)^(v3-v1);
        if (normal.normalize()!=0.0f)
        {
            _deviation = osg::minimum(_normal*normal,_deviation);
        }
        _radius2 = osg::maximum((v1-_center).length2(),_radius2);
        _radius2 = osg::maximum((v2-_center).length2(),_radius2);
        _radius2 = osg::maximum((v3-_center).length2(),_radius2);

    }
    osg::Vec3 _center;
    osg::Vec3 _normal;
    float _deviation;
    float _radius2;
}; 

class ComputeDeviationVisitor : public osg::NodeVisitor
{
public:
    ComputeDeviationVisitor( const osg::Vec3& centroid, const osg::Vec3& normal )
        : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN )
    {
        cdf.set( centroid, normal );
    }

    void apply( osg::Geode& geode )
    {
        for( unsigned int i=0; i<geode.getNumDrawables(); i++ )
        {
            osg::Drawable* d = geode.getDrawable( i );
            geode.getDrawable( i )->accept( cdf );
        }
    }

    void get( double& out_deviation, double& out_radius )
    {
        float angle = acosf(cdf._deviation)+osg::PI*0.5f;
        if (angle<osg::PI) out_deviation = cosf(angle);
        else out_deviation = -1.0f;
        out_radius = sqrtf( cdf._radius2 );
    }

    osg::TriangleFunctor<ComputeDeviationFunctor> cdf;
};


class LocalizeVisitor : public osg::NodeVisitor
{
public:
    LocalizeVisitor( const osg::Vec3& _origin )
        : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ),
          origin( _origin ) { }

    void apply( osg::Geode& geode )
    {
        for( unsigned int i=0; i<geode.getNumDrawables(); i++ )
        {
            osg::Drawable* d = geode.getDrawable( i );
            
            osg::Geometry* geom = dynamic_cast<osg::Geometry*>( d );
            if ( geom )
            {
                osg::Array* verts = geom->getVertexArray();
                if ( dynamic_cast<osg::Vec3Array*>( verts ) )
                {
                    osg::Vec3Array* v = static_cast<osg::Vec3Array*>( verts );
                    for( osg::Vec3Array::iterator i = v->begin(); i != v->end(); i++ )
                    {
                        (*i) -= origin;
                    }
                }
                else
                {
                    //TODO...
                }
                continue;
            }

            osgText::Text* text = dynamic_cast<osgText::Text*>( d );
            if ( text )
            {
                text->setPosition( text->getPosition() - origin );
                continue;
            }

            osg::ShapeDrawable* sd = dynamic_cast<osg::ShapeDrawable*>( d );
            if ( sd )
            {
            }
            //TODO... etc... other osg::Drawable subclasses
        }

        traverse( geode );
    }

private:
    osg::Vec3 origin;
};


class ApplyClusterCullersVisitor : public osg::NodeVisitor
{
public:
    ApplyClusterCullersVisitor() :
      osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN ) { }

      void apply( osg::Geode& geode )
      {
          for( unsigned int i=0; i<geode.getNumDrawables(); i++ )
          {
              osg::Drawable* d = geode.getDrawable( i );
              d->setCullCallback( new osg::ClusterCullingCallback( d ) );
          }
      }
};


osg::NodeList
LocalizeNodesFilter::process( osg::Node* input, FilterEnv* env )
{
    osg::NodeList output;

    const GeoExtent& extent = env->getExtent();

    if ( extent.isValid() && !extent.isInfinite() )
    {
        GeoPoint centroid;
        osg::Matrixd matrix;
        bool apply_cluster_culling = false;

        if ( env->getInputSRS()->isGeocentric() )
        {
            centroid = env->getInputSRS()->transform( extent.getCentroid() );
            apply_cluster_culling = true;
        }
        else if ( env->getInputSRS()->isGeographic() )
        {
            // get a GEOC SRS
            osg::ref_ptr<SpatialReference> geoc_srs = 
                Registry::instance()->getSRSFactory()->createGeocentricSRS( 
                    extent.getSRS() );

            if ( geoc_srs.valid() )
            {
                centroid = geoc_srs->transform( extent.getCentroid() );
                apply_cluster_culling = true;
            }
        }
        else if ( env->getInputSRS()->isProjected() )
        {
            centroid = env->getInputSRS()->transform(
                env->getExtent().getCentroid() );
            
            // no cluster culling necessary for a flat-earth dataset:
            apply_cluster_culling = false;
        }

        if ( centroid.isValid() )
        {
            LocalizeVisitor visitor( centroid );
            input->accept( visitor );
            matrix = osg::Matrixd::translate( centroid );
        }

        osg::MatrixTransform* xform = new osg::MatrixTransform( matrix );
        xform->addChild( input );

        if ( apply_cluster_culling )
        {
            double deviation, radius;
            osg::Vec3 normal = centroid;
            normal.normalize();

            //ComputeDeviationVisitor cdv( centroid, normal );
            //input->accept( cdv );
            //cdv.get( deviation, radius );

            osg::ClusterCullingCallback* ccc = new osg::ClusterCullingCallback(
                centroid,
                normal,
                -0.75 ); //deviation );

            //ccc->setRadius( radius );
    
            xform->setCullCallback( ccc );
        }

        output.push_back( xform );
    }
    else
    {
        output.push_back( input );
    }

    return output;
}
