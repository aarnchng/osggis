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
TierifyPolygonsFilter::setOutputHeightAttribute( const std::string& value )
{
    output_height_attr = value;
}

const std::string&
TierifyPolygonsFilter::getOutputHeightAttribute() const
{
    return output_height_attr;
}

void
TierifyPolygonsFilter::setProperty( const Property& p )
{
    if ( p.getName() == "height" )
        setHeightScript( new Script( p.getValue() ) );
    else if ( p.getName() == "output_height_attribute" )
        setOutputHeightAttribute( p.getValue() );
    FeatureFilter::setProperty( p );
}


Properties
TierifyPolygonsFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getHeightScript() )
        p.push_back( Property( "height", getHeightScript()->getCode() ) );
    if ( getOutputHeightAttribute().length() > 0 )
        p.push_back( Property( "output_height_attribute", getOutputHeightAttribute() ) );
    return p;
}



typedef std::map<FeatureOID,double> FeatureHeightMap;


// sorts input features from lowest to highest based on each feature's "height".
static void
sortFeaturesByHeight( FeatureList& input, FilterEnv* env, Script* height_script, FeatureList& output, FeatureHeightMap& heights )
{
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        ScriptResult ir = env->getScriptEngine()->run( height_script, i->get(), env );
        double i_h = ir.isValid()? ir.asDouble(0.0) : 0.0;
        heights[i->get()->getOID()] = i_h;

        FeatureList::iterator j = output.begin();
        for( ; j != output.end(); j++ )
        {                
            if ( i_h < heights[j->get()->getOID()] )
                break;
        }
        output.insert( j, i->get() );
    }
}


static bool
partContains2d( const GeoPointList& part, const GeoPoint& p )
{
    for( GeoPointList::const_iterator i = part.begin(); i != part.end(); i++ )
    {
        if ( (*i).x() == p.x() && (*i).y() == p.y() )
            return true;
    }
    return false;
}


static bool
findMatchingNonHolePart( FeatureList& features, FeatureList::iterator i, const GeoPointList& hole )
{
    for( ; i != features.end(); i++ )
    {
        Feature* f = i->get();
        GeoShapeList& shapes = f->getShapes();
        for( GeoShapeList::const_iterator j = shapes.begin(); j != shapes.end(); j++ )
        {
            const GeoShape& shape = *j;
            for( GeoPartList::const_iterator k = shape.getParts().begin(); k != shape.getParts().end(); k++ )
            {
                const GeoPointList& part = *k;
                if ( part.size() > 0 && part.size() == hole.size() )
                {
                    bool diff = false;
                    for( GeoPointList::const_iterator n = part.begin(); n != part.end(); n++ )
                    {
                        const GeoPoint& p = *n;
                        if ( partContains2d( hole, p ) )
                        {
                            diff = true;
                            break;
                        }
                    }

                    // found a match:
                    if ( !diff )
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}


// 
static void
removeInsideHoles( FeatureList& input )
{
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        GeoShapeList& shapes = i->get()->getShapes();

        for( GeoShapeList::iterator j = shapes.begin(); j != shapes.end(); j++ )
        {
            GeoPartList& parts = j->getParts();

            // go thru the part list and look for holes.
            for( GeoPartList::iterator k = parts.begin(); k != parts.end(); )
            {
                bool erased = false;
                GeoPointList& part = *k;
                bool part_is_hole = !GeomUtils::isPolygonCW( part );
                if ( part_is_hole )
                {
                    // found a hole; search upwards up the feature stack looking for a non-hole with the same
                    // exact shape as this hole. If we find one, remove this hole and continue.
                    if ( findMatchingNonHolePart( input, i+1, part ) )
                    {
                        k = parts.erase( k );
                        erased = true;
                    }
                }
                if ( !erased )
                    k++;
            }
        }
    }
}


// Go from bottom to top. For each poly, check whether it falls completely
// within any polys below it. If so, transform it up to the other poly's "height".
void
trimShapes( FeatureList& input, FeatureHeightMap& heights, const std::string& output_height_attr )
{
    for( FeatureList::iterator i = input.begin()+1; i != input.end(); i++ )
    {
        Feature* upper_f = i->get();

        GeoShapeList& shapes = upper_f->getShapes();
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
                for( FeatureList::const_iterator j = i-1; !done && j != input.end(); j = j == input.begin()? input.end() : j-1 )
                {
                    Feature* lower_f = j->get();

                    bool is_inside_lower_poly = false;
                    const GeoShapeList& lower_shapes = lower_f->getShapes();
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
                        double lower_poly_h = heights[lower_f->getOID()];
                        for( GeoPointList::iterator n = first_part.begin(); n != first_part.end(); n++ )
                        {
                            GeoPoint& p = *n;
                            if ( p.getDim() < 3 )
                                p.z() = lower_poly_h; // the *3 is for testing
                            else
                                p.z() = p.z() + lower_poly_h; // the *3 is for testing

                            p.setDim( 3 );

                            double new_height = heights[upper_f->getOID()] - lower_poly_h;
                            upper_f->setAttribute( output_height_attr, new_height );
                        }
                        done = true;
                    }
                }
            }
        }
    }
}


static void
copyHeights( FeatureList& input, FilterEnv* env, Script* height_script, const std::string& output_height_attr )
{
    for( FeatureList::iterator i = input.begin(); i != input.end(); i++ )
    {
        ScriptResult ir = env->getScriptEngine()->run( height_script, i->get(), env );
        double i_h = ir.isValid()? ir.asDouble(0.0) : 0.0;
        i->get()->setAttribute( output_height_attr, i_h );
    }
}



FeatureList
TierifyPolygonsFilter::process( FeatureList& input, FilterEnv* env )
{
    FeatureList output;

    // copy old height value to output height field for all features:
    copyHeights( input, env, getHeightScript(), getOutputHeightAttribute() );

    if ( input.size() > 1 && getHeightScript() && getOutputHeightAttribute().length() > 0 )
    {
        // first sort the input features low to high based on the height.
        FeatureList sorted;
        FeatureHeightMap heights;
        sortFeaturesByHeight( input, env, getHeightScript(), sorted, heights );

        // next, optimize a bit by removing inner hole poly parts:
        removeInsideHoles( sorted );

        // next trim each shape so that it does not extend downwards past polys
        // underneath it, assuming it fits completely within them.
        trimShapes( sorted, heights, getOutputHeightAttribute() );

        output.insert( output.end(), sorted.begin(), sorted.end() );
    }
    else
    {
        // nothing to do; just copy the input to the output
        output.insert( output.begin(), input.begin(), input.end() );
    }

    return output;
}