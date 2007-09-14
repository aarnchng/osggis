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

#include <osgGIS/TransformFilter>
#include <osg/CoordinateSystemNode>
#include <osgGIS/Ellipsoid>
#include <osgGIS/Registry>

using namespace osgGIS;

TransformFilter::TransformFilter()
{
    xform_matrix = osg::Matrix::identity();
    options = (Options)0;
}


TransformFilter::TransformFilter( const SpatialReference* _srs )
{
    srs = (SpatialReference*)_srs;
    options = (Options)0;
}


TransformFilter::TransformFilter(const SpatialReference* _srs,
                                 const int&              _options)
{
    srs = (SpatialReference*)_srs;
    options = _options;
}


TransformFilter::TransformFilter( const osg::Matrix& _matrix )
{
    xform_matrix = _matrix;
    options = (Options)0;
}


TransformFilter::~TransformFilter()
{
}


void
TransformFilter::setMatrix( const osg::Matrix& _matrix )
{
    xform_matrix = _matrix;
}


const osg::Matrix&
TransformFilter::getMatrix() const
{
    return xform_matrix;
}


void
TransformFilter::setLocalize( bool enabled )
{
    if ( enabled )
        options |= LOCALIZE;
    else
        options &= ~LOCALIZE;
}


bool
TransformFilter::getLocalize() const
{
    return options & LOCALIZE;
}


void
TransformFilter::setSRS( const SpatialReference* _srs )
{
    srs = (SpatialReference*)_srs;
}


const SpatialReference*
TransformFilter::getSRS() const
{
    return srs.get();
}


void
TransformFilter::setProperty( const Property& p )
{
    if ( p.getName() == "localize" )
        setLocalize( p.getBoolValue( getLocalize() ) );
    else if ( p.getName() == "matrix" )
        setMatrix( p.getMatrixValue() );
    else if ( p.getName() == "srs" )
        setSRS( Registry::instance()->getSRSFactory()->createSRSfromWKT( p.getValue() ) );
    FeatureFilter::setProperty( p );
}


Properties
TransformFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    p.push_back( Property( "localize", getLocalize() ) );
    p.push_back( Property( "matrix", getMatrix() ) );
    if ( getSRS() )
        p.push_back( Property( "srs", getSRS()->getWKT() ) );
    return p;
}


FeatureList
TransformFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;
    
    osg::ref_ptr<SpatialReference> out_srs = srs.get();
    if ( out_srs.valid() )
    {
        // LOCALIZE points around a local origin (the working extent's centroid)
        if ( options & TransformFilter::LOCALIZE && env->getExtent().getArea() > 0.0 )
        {
            if ( env->getExtent().getSRS()->isGeographic() &&
                 env->getExtent().getWidth() > 179.0 )
            {
                //NOP - no localization for big geog extent
            }
            else
            {
                GeoPoint centroid = out_srs->transform( env->getExtent().getCentroid() );
                osg::Matrixd localizer;

                // For geocentric datasets, we need a special localizer matrix
                if ( out_srs->isGeocentric() )
                {                    
                    localizer = out_srs->getBasisEllipsoid().createGeocentricInvRefFrame(
                        centroid );
                    localizer = osg::Matrixd::inverse( localizer );
                }
                else
                {
                    localizer = osg::Matrixd::translate( -centroid );
                }
                out_srs = out_srs->cloneWithNewReferenceFrame( localizer );
            }
        }
        
        env->setOutputSRS( out_srs.get() );
    }

    for( GeoShapeList::iterator& shape = input->getShapes().begin(); 
         shape!= input->getShapes().end();
         shape++ )
    {
        if ( out_srs.valid() )
        {
            out_srs->transformInPlace( *shape );
        }
        else if ( xform_matrix.valid() )
        {
            struct XformVisitor : public GeoPointVisitor {
                osg::Matrixd mat;
                bool visitPoint( GeoPoint& p ) {
                    p.set( p * mat );
                    return true;
                }
            };

            XformVisitor visitor;
            visitor.mat = xform_matrix;
            shape->accept( visitor );
        }
    }

    output.push_back( input );
    return output;
}

