/**
 * osgGIS - GIS Library for OpenSceneGraph
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

#include <osgGIS/TransformFilter>
#include <osgGIS/Ellipsoid>
#include <osgGIS/Utils>
#include <osg/CoordinateSystemNode>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( TransformFilter );

#define DEFAULT_LOCALIZE false
#define DEFAULT_USE_TERRAIN_SRS false


TransformFilter::TransformFilter()
{
    xform_matrix = osg::Matrix::identity();
    localize = DEFAULT_LOCALIZE;
    use_terrain_srs = DEFAULT_USE_TERRAIN_SRS;
}

TransformFilter::TransformFilter( const TransformFilter& rhs )
: FeatureFilter( rhs ),
  xform_matrix( rhs.xform_matrix ),
  localize( rhs.localize ),
  use_terrain_srs( rhs.use_terrain_srs ),
  srs( rhs.srs.get() ),
  srs_script( rhs.srs_script.get() ),
  translate_script( rhs.translate_script.get() )
{
    //NOP
}

TransformFilter::~TransformFilter()
{
    //NOP
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
TransformFilter::setLocalize( bool value )
{
    localize = value;
}

bool
TransformFilter::getLocalize() const
{
    return localize;
}

void
TransformFilter::setUseTerrainSRS( bool value )
{
    use_terrain_srs = value;
}

bool
TransformFilter::getUseTerrainSRS() const
{
    return use_terrain_srs;
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
TransformFilter::setSRSScript( Script* value )
{
    srs_script = value;
}

Script*
TransformFilter::getSRSScript() const
{
    return srs_script.get();
}

void
TransformFilter::setTranslateScript( Script* value )
{
    translate_script = value;
}

Script*
TransformFilter::getTranslateScript() const
{
    return translate_script.get();
}

void
TransformFilter::setProperty( const Property& p )
{
    if ( p.getName() == "localize" )
        setLocalize( p.getBoolValue( getLocalize() ) );
    else if ( p.getName() == "translate" )
        setTranslateScript( new Script( p.getValue() ) );
    else if ( p.getName() == "use_terrain_srs" )
        setUseTerrainSRS( p.getBoolValue( getUseTerrainSRS() ) );
    else if ( p.getName() == "srs" )
        setSRSScript( new Script( p.getValue() ) );
    FeatureFilter::setProperty( p );
}


Properties
TransformFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getLocalize() != DEFAULT_LOCALIZE )
        p.push_back( Property( "localize", getLocalize() ) );
    if ( getUseTerrainSRS() != DEFAULT_USE_TERRAIN_SRS )
        p.push_back( Property( "use_terrain_srs", getUseTerrainSRS() ) );
    if ( getTranslateScript() )
        p.push_back( Property( "translate", getTranslateScript()->getCode() ) );
    if ( getSRSScript() )
        p.push_back( Property( "srs", getSRSScript()->getCode() ) );
    return p;
}


FeatureList
TransformFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    // first try to use the terrain SRS if so directed:
    osg::ref_ptr<SpatialReference> new_out_srs = getUseTerrainSRS()? env->getTerrainSRS() : NULL;
    if ( !new_out_srs.valid() )
    {
        // failing that, see if we have an SRS in a resource:
        if ( !getSRS() && getSRSScript() )
        {
            ScriptResult r = env->getScriptEngine()->run( getSRSScript(), input, env );
            if ( r.isValid() )
            {
                setSRS( env->getSession()->getResources()->getSRS( r.asString() ) );
            }
        }

        new_out_srs = srs.get();
    }

    // resolve the xlate shortcut
    osg::Matrix working_matrix = xform_matrix;

    // TODO: this can go into process(FeatureList) instead of running for every feature..
    if ( getTranslateScript() )
    {
        ScriptResult r = env->getScriptEngine()->run( getTranslateScript(), input, env );
        if ( r.isValid() )
            working_matrix = osg::Matrix::translate( r.asVec3() );
    }

    SpatialReference* working_srs = new_out_srs.valid()? 
        new_out_srs.get() : 
        env->getInputSRS(); //NULL;

    // LOCALIZE points around a local origin (the working extent's centroid)
    if ( working_srs && getLocalize() && env->getExtent().getArea() > 0.0 )
    {
        if ( env->getExtent().getSRS()->isGeographic() &&
             env->getExtent().getWidth() > 179.0 )
        {
            //NOP - no localization for big geog extent
        }
        else
        {
            //working_srs = new_out_srs.get();

            GeoPoint centroid = new_out_srs.valid()?
                new_out_srs->transform( env->getExtent().getCentroid() ) :
                env->getExtent().getCentroid();

            osg::Matrixd localizer;


            // For geocentric datasets, we need a special localizer matrix
            if ( working_srs->isGeocentric() )
            {                    
                localizer = working_srs->getEllipsoid().createGeocentricInvRefFrame( centroid );
                localizer = osg::Matrixd::inverse( localizer );
            }
            else
            {
                localizer = osg::Matrixd::translate( -centroid );
            }
            working_srs = working_srs->cloneWithNewReferenceFrame( localizer );
        }
    }
    
    if ( working_srs )
    {
        env->setOutputSRS( working_srs );
    }

    if ( working_srs || ( working_matrix.valid() && !working_matrix.isIdentity() ) )
    {
        for( GeoShapeList::iterator shape = input->getShapes().begin(); 
             shape!= input->getShapes().end();
             shape++ )
        {
            if ( working_srs && !working_srs->equivalentTo( env->getInputSRS() ) ) //???
            {
                working_srs->transformInPlace( *shape );
            }

            if ( working_matrix.valid() && !working_matrix.isIdentity() )
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

