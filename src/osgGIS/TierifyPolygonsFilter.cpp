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

#include <osgGIS/TierifyPolygonsFilter>
#include <osgGIS/Utils>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( TierifyPolygonsFilter );



TierifyPolygonsFilter::TierifyPolygonsFilter()
{
    //NOP
}


TierifyPolygonsFilter::~TierifyPolygonsFilter()
{
    //NOP
}

void
TierifyPolygonsFilter::setHeightScript( Script* value )
{
    height_script = value;
}

Script*
TierifyPolygonsFilter::getHeightScript() const
{
    return const_cast<const TierifyPolygonsFilter*>(this)->height_script.get();
}

void
TierifyPolygonsFilter::setProperty( const Property& p )
{
    if ( p.getName() == "height" )
        setHeightScript( new Script( p.getValue() ) );
    FeatureFilter::setProperty( p );
}


Properties
TierifyPolygonsFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getHeightScript() )
        p.push_back( Property( "height", getHeightScript()->getCode() ) );
    return p;
}



typedef std::map<FeatureOID,double> FeatureHeightMap;

void
tierify( FeatureList& input, FilterEnv* env, Script* height_script )
{
    //todo
}



FeatureList
TierifyPolygonsFilter::process( FeatureList& input, FilterEnv* env )
{
    FeatureList output;

    if ( input.size() > 1 && getHeightScript() )
    {
        // first sort the input features low to high based on the height
        // simple insertion sort.
        FeatureList sorted;
        FeatureHeightMap heights;
        for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
        {
            ScriptResult ir = env->getScriptEngine()->run( getHeightScript(), i->get(), env );
            double i_h = ir.isValid()? ir.asDouble(0.0) : 0.0;
            heights[i->get()->getOID()] = i_h;

            FeatureList::iterator j = sorted.begin();
            for( ; j != sorted.end(); j++ )
            {                
                if ( i_h < heights[j->get()->getOID()] )
                    break;
            }
            sorted.insert( j, i->get() );
        }

        // next...go from bottom to top. For each poly, check whether it falls completely
        // within any polys below it. If so, transform it up to the other poly's "height".
        for( FeatureList::iterator i = sorted.begin()+1; i != sorted.end(); i++ )
        {
            GeoShapeList& shapes = i->get()->getShapes();
            if ( shapes.size() > 0 )
            {
                //TODO: consider multi-shape features.
                GeoShape& first_shape = shapes[0];
                if ( first_shape.getPartCount() > 0 )
                {
                    //TODO: consider muti-part shapes. As it stands, this is currently ignoring holes.
                    GeoPointList& first_part = first_shape.getPart( 0 );

                    // loop through each lower polygon and test each vertex. If all verts are within
                    // the lower poly, stop and tierify it (by raising its base verts up to the height
                    // of the lower poly).
                    bool done = false;
                    for( FeatureList::const_iterator j = i-1; !done && j != sorted.end(); j = j == sorted.begin()? sorted.end() : j-1 )
                    {
                        bool is_inside_lower_poly = false;
                        const GeoShapeList& lower_shapes = j->get()->getShapes();
                        if ( lower_shapes.size() > 0 )
                        {
                            const GeoShape& first_lower_shape = lower_shapes[0];
                            if ( first_lower_shape.getPartCount() > 0 )
                            {
                                const GeoPointList& first_lower_part = first_lower_shape.getPart( 0 );
                                
                                is_inside_lower_poly = true;
                                for( GeoPointList::const_iterator k = first_part.begin(); k != first_part.end() && is_inside_lower_poly; k++ )
                                {
                                    if ( !GeomUtils::isPointInPolygon( *k, first_lower_part ) )
                                        is_inside_lower_poly = false;
                                }
                            }
                        }

                        // if our poly sits inside the other poly, tierify it and move on
                        if ( is_inside_lower_poly )
                        {
                            double lower_poly_h = heights[j->get()->getOID()];
                            for( GeoPointList::iterator n = first_part.begin(); n != first_part.end(); n++ )
                            {
                                GeoPoint& p = *n;
                                if ( p.getDim() < 3 )
                                    p.z() = lower_poly_h * 3; // the *3 is for testing
                                else
                                    p.z() = p.z() + lower_poly_h * 3; // the *3 is for testing
                                p.setDim( 3 );
                            }
                            done = true;
                        }
                    }
                }
            }
        }

        output.insert( output.end(), sorted.begin(), sorted.end() );
    }
    else
    {
        // nothing to do; just copy the input to the output
        output.insert( output.begin(), input.begin(), input.end() );
    }

    return output;
}