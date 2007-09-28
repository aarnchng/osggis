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

#include <osgGIS/CollectionFilter>
#include <osgGIS/FeatureFilter>
#include <osgGIS/DrawableFilter>
#include <osgGIS/NodeFilter>
#include <osg/Notify>

using namespace osgGIS;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( CollectionFilter );

CollectionFilter::CollectionFilter()
{
    setMetering( 0 );
}


CollectionFilter::CollectionFilter( int _metering )
{
    setMetering( _metering );
}


CollectionFilter::~CollectionFilter()
{
    //NOP
}


void 
CollectionFilter::setProperty( const Property& prop )
{
    if ( prop.getName() == "metering" )
        setMetering( prop.getIntValue( 0 ) );
    Filter::setProperty( prop );
}

Properties
CollectionFilter::getProperties() const
{
    Properties p = Filter::getProperties();
    p.push_back( Property( "metering", getMetering() ) );
    return p;
}


void
CollectionFilter::reset( osgGIS::ScriptContext* context )
{
    in_features.clear();
    in_drawables.clear();
    in_nodes.clear();
    Filter::reset( context );
}


void 
CollectionFilter::push( const FeatureList& input )
{
    in_features.insert( in_features.end(), input.begin(), input.end() );
}


void
CollectionFilter::push( Feature* input )
{
    in_features.push_back( input );
}


void 
CollectionFilter::push( const DrawableList& input )
{
    in_drawables.insert( in_drawables.end(), input.begin(), input.end() );
}


void
CollectionFilter::push( osg::Drawable* input )
{
    in_drawables.push_back( input );
}


void 
CollectionFilter::push( const osg::NodeList& input )
{
    in_nodes.insert( in_nodes.begin(), input.begin(), input.end() );
}


void
CollectionFilter::push( osg::Node* input )
{
    in_nodes.push_back( input );
}


bool 
CollectionFilter::traverse( FilterEnv* env )
{
    // just save a copy of the env for checkpoint time.
    saved_env = env->advance();
    return true;
}

template<typename A, typename B>
bool
meterData( A source, B filter, unsigned int metering, FilterEnv* env )
{
    bool ok = true;

    //osg::notify( osg::ALWAYS ) << "Metering " << source.size() << " units." << std::endl;

    if ( metering == 0 )
    {
        filter->push( source );
        ok = filter->traverse( env );
    }
    else
    {
        unsigned int batch_size = 0;
        for( typename A::iterator i = source.begin(); i < source.end() && ok; i += batch_size )
        {
            unsigned int remaining = source.end()-i;
            batch_size = std::min( remaining, metering );
            A partial;
            partial.insert( partial.end(), i, i + batch_size );
            filter->push( partial );
            ok = filter->traverse( env );

            //osg::notify( osg::ALWAYS )
            //    << "Metered: " << i-source.begin() << "/" << source.end()-source.begin()
            //    << std::endl;
        }
    }
    return ok;
}


bool 
CollectionFilter::signalCheckpoint()
{
    bool ok = true;

    Filter* next = getNextFilter();
    if ( next )
    {
        if ( dynamic_cast<FeatureFilter*>( next ) )
        {
            FeatureFilter* filter = static_cast<FeatureFilter*>( next );
            if ( in_features.size() > 0 )
                ok = meterData( in_features, filter, metering, saved_env.get() );
            else
                ok = false;
        }
        else if ( dynamic_cast<DrawableFilter*>( next ) )
        {
            DrawableFilter* filter = static_cast<DrawableFilter*>( next );
            if ( in_features.size() > 0 )
                ok = meterData( in_features, filter, metering, saved_env.get() );
            else if ( in_drawables.size() > 0 )
                ok = meterData( in_drawables, filter, metering, saved_env.get() );
            else
                ok = false;
        }
        else if ( dynamic_cast<NodeFilter*>( next ) )
        {
            NodeFilter* filter = static_cast<NodeFilter*>( next );
            if ( in_drawables.size() > 0 )
                ok = meterData( in_drawables, filter, metering, saved_env.get() );
            else if ( in_nodes.size() > 0 )
                ok = meterData( in_nodes, filter, metering, saved_env.get() );
            else
                ok = false;
        }
        else if ( dynamic_cast<CollectionFilter*>( next ) )
        {
            CollectionFilter* filter = static_cast<CollectionFilter*>( next );
            if ( in_features.size() > 0 )
                ok = meterData( in_features, filter, metering, saved_env.get() );
            else if ( in_drawables.size() > 0 )
                ok = meterData( in_drawables, filter, metering, saved_env.get() );
            else if ( in_nodes.size() > 0 )
                ok = meterData( in_nodes, filter, metering, saved_env.get() );
            else
                ok = false;
        }

        if ( ok )
        {
            ok = next->signalCheckpoint();
        }
    }

    // clean up the input:
    in_features.clear();
    in_drawables.clear();
    in_nodes.clear();
    saved_env = NULL;

    return ok;
}

