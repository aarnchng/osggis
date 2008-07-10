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

#include <osgGIS/WriteFeaturesFilter>
#include <OpenThreads/ScopedLock>

using namespace osgGIS;
using namespace OpenThreads;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( WriteFeaturesFilter );


WriteFeaturesFilter::WriteFeaturesFilter()
{
    //NOP
}

WriteFeaturesFilter::WriteFeaturesFilter( const WriteFeaturesFilter& rhs )
: FeatureFilter( rhs ),
  output_uri( rhs.output_uri )
{
    //NOP
}

WriteFeaturesFilter::~WriteFeaturesFilter()
{
    //NOP
}

void
WriteFeaturesFilter::setOutputURI( const std::string& value )
{
    output_uri = value;
}

const std::string&
WriteFeaturesFilter::getOutputURI() const
{
    return output_uri;
}

void
WriteFeaturesFilter::setProperty( const Property& p )
{
    if ( p.getName() == "output_uri" )
        setOutputURI( p.getValue() );
    else
        FeatureFilter::setProperty( p );
}

Properties
WriteFeaturesFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getOutputURI().length() > 0 )
        p.push_back( Property( "output_uri", getOutputURI() ) );
    return p;
}

#define PROP_FEATURE_STORE "WriteFeaturesFilter:store"

FeatureList
WriteFeaturesFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;

    if ( getOutputURI().length() > 0 && input->hasShapeData() )
    {
        // open the feature store as a SESSION property so that all compilers can share it
        FeatureStore* store = NULL;

        // lock the session while fetching/creating the feature store:
        {
            ScopedLock<ReentrantMutex> session_lock( env->getSession()->getSessionMutex() );
            Property p = env->getSession()->getProperty( PROP_FEATURE_STORE );
            store = dynamic_cast<FeatureStore*>( p.getRefValue() );

            if ( !store )
            {
                store = Registry::instance()->getFeatureStoreFactory()->createFeatureStore(
                    getOutputURI(),
                    input->getShapeType(),
                    input->getAttributeSchemas(),
                    input->getShapeDim(),
                    env->getInputSRS(),
                    Properties() );

                if ( store )
                {
                    env->getSession()->setProperty( Property( PROP_FEATURE_STORE, store ) );
                    osg::notify(osg::NOTICE) << "WriteFeatures: created feature store \"" << getOutputURI() << "\"" << std::endl;
                }
            }
        }

        if ( store )
        {
            store->insertFeature( input );
        }
    }
    
    output.push_back( input );
    return output;
}

