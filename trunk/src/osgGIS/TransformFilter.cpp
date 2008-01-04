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

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( TransformFilter );


TransformFilter::TransformFilter()
{
    xform_matrix = osg::Matrix::identity();
    options = (Options)0;
}


TransformFilter::TransformFilter( const int& _options )
{
    xform_matrix = osg::Matrix::identity();
    options = _options;
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
TransformFilter::setUseTerrainSRS( bool value )
{
    options = value? 
        options | USE_TERRAIN_SRS :
        options & ~USE_TERRAIN_SRS;
}


bool
TransformFilter::getUseTerrainSRS() const
{
    return ( options & USE_TERRAIN_SRS ) != 0;
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
    //else if ( p.getName() == "matrix" )
    //    setMatrix( p.getMatrixValue() );
    else if ( p.getName() == "translate" )
        translate_expr = p.getValue();
        //setMatrix( osg::Matrix::translate( p.getVec3Value() ) );
    else if ( p.getName() == "use_terrain_srs" )
        setUseTerrainSRS( p.getBoolValue( getUseTerrainSRS() ) );
    else if ( p.getName() == "srs" )
        setSRS( Registry::instance()->getSRSFactory()->createSRSfromWKT( p.getValue() ) );
    FeatureFilter::setProperty( p );
}


Properties
TransformFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    p.push_back( Property( "localize", getLocalize() ) );
    //p.push_back( Property( "matrix", getMatrix() ) );
    if ( translate_expr.length() > 0 )
        p.push_back( Property( "translate", translate_expr ) );
    if ( getUseTerrainSRS() )
        p.push_back( Property( "use_terrain_srs", getUseTerrainSRS() ) );
    if ( getSRS() )
        p.push_back( Property( "srs", getSRS()->getWKT() ) );
    return p;
}


FeatureList
TransformFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;
    
    osg::ref_ptr<SpatialReference> new_out_srs = getUseTerrainSRS()? env->getTerrainSRS() : NULL;
    if ( !new_out_srs.valid() )
        new_out_srs = srs.get();

    // resolve the xlate shortcut
    osg::Matrix working_matrix = xform_matrix;

    if ( translate_expr.length() > 0 )
    {
        ScriptResult r = env->getScriptEngine()->run( new Script( translate_expr ), input, env );
        if ( r.isValid() )
            working_matrix = osg::Matrix::translate( r.asVec3() );
    }

    if ( new_out_srs.valid() )
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
                GeoPoint centroid = new_out_srs->transform( env->getExtent().getCentroid() );
                osg::Matrixd localizer;

                // For geocentric datasets, we need a special localizer matrix
                if ( new_out_srs->isGeocentric() )
                {                    
                    localizer = new_out_srs->getBasisEllipsoid().createGeocentricInvRefFrame(
                        centroid );
                    localizer = osg::Matrixd::inverse( localizer );
                }
                else
                {
                    localizer = osg::Matrixd::translate( -centroid );
                }
                new_out_srs = new_out_srs->cloneWithNewReferenceFrame( localizer );
            }
        }
        
        env->setOutputSRS( new_out_srs.get() );
    }

    if ( new_out_srs.valid() || ( working_matrix.valid() && !working_matrix.isIdentity() ) )
    {
        for( GeoShapeList::iterator shape = input->getShapes().begin(); 
             shape!= input->getShapes().end();
             shape++ )
        {
            if ( new_out_srs.valid() )
            {
                new_out_srs->transformInPlace( *shape );
            }
            else if ( working_matrix.valid() && !working_matrix.isIdentity() )
            {
                struct XformVisitor : public GeoPointVisitor {
                    osg::Matrixd mat;
                    bool visitPoint( GeoPoint& p ) {
                        p.set( p * mat );
                        p.setDim( 3 );
                        return true;
                    }
                };

                XformVisitor visitor;
                visitor.mat = working_matrix;
                shape->accept( visitor );
            }
        }
    }

    output.push_back( input );
    return output;
}

