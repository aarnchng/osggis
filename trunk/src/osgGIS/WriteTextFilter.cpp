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

#include <osgGIS/WriteTextFilter>
#include <osg/Notify>
#include <fstream>
#include <iostream>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>

using namespace osgGIS;
using namespace OpenThreads;

#include <osgGIS/Registry>
OSGGIS_DEFINE_FILTER( WriteTextFilter );


#define DEFAULT_APPEND false
#define PROP_FILE "WriteTextFilter:file"


class RefFile : public osg::Referenced
{
public:
    RefFile( const std::string& filename )
        : out( filename.c_str() ) { }

    void write( const std::string& str )
    {
        ScopedLock<Mutex> sl( mutex );
        out << str;
    }

protected:
    virtual ~RefFile() {
        out.close();
    }
    std::ofstream out;
    Mutex mutex;
};


WriteTextFilter::WriteTextFilter()
{
    append = DEFAULT_APPEND;
}

WriteTextFilter::WriteTextFilter( const WriteTextFilter& rhs )
: FeatureFilter( rhs ),
  append( rhs.append ),
  output_path_resource_name_script( rhs.output_path_resource_name_script.get() ),
  text_script( rhs.text_script.get() )
{
    //NOP
}

void
WriteTextFilter::setOutputPathResourceNameScript( Script* value )
{
    output_path_resource_name_script = value;
}

Script*
WriteTextFilter::getOutputPathResourceNameScript() const
{
    return output_path_resource_name_script.get();
}

void
WriteTextFilter::setTextScript( Script* value )
{
    text_script = value;
}

Script*
WriteTextFilter::getTextScript() const
{
    return text_script.get();
}

void
WriteTextFilter::setAppend( bool value )
{
    append = value;
}

bool
WriteTextFilter::getAppend() const
{
    return append;
}
void
WriteTextFilter::setProperty( const Property& p )
{
    if ( p.getName() == "output" )
        setOutputPathResourceNameScript( new Script( p.getValue() ) );
    else if ( p.getName() == "text" )
        setTextScript( new Script( p.getValue() ) );
    else if ( p.getName() == "append" )
        setAppend( p.getBoolValue( getAppend() ) );
    FeatureFilter::setProperty( p );
}

Properties
WriteTextFilter::getProperties() const
{
    Properties p = FeatureFilter::getProperties();
    if ( getOutputPathResourceNameScript() )
        p.push_back( Property( "output", getOutputPathResourceNameScript()->getCode() ) );
    if ( getTextScript() )
        p.push_back( Property ( "text", getTextScript()->getCode() ) );
    if ( getAppend() != DEFAULT_APPEND )
        p.push_back( Property( "append", getAppend() ) );
    return p;
}

FeatureList
WriteTextFilter::process( Feature* input, FilterEnv* env )
{
    FeatureList output;
    output.push_back( input );

    if ( getTextScript() && getOutputPathResourceNameScript() )
    {
        // open the file as a SESSION property so that all compilers can share it
        Property p = env->getSession()->getProperty( PROP_FILE );
        RefFile* file = dynamic_cast<RefFile*>( p.getRefValue() );
        if ( !file ) {
            ScopedLock<ReentrantMutex> create_file_lock( env->getSession()->getSessionMutex() );

            // check AGAIN just in case another thread created the file before we acquired the lock:
            file = dynamic_cast<RefFile*>( p.getRefValue() );
            if ( !file )
            {
                ScriptResult r = env->getScriptEngine()->run( getOutputPathResourceNameScript(), input, env );
                if ( r.isValid() )
                {
                    const std::string output_path = env->getSession()->getResources()->getPath( r.asString() );
                    if ( output_path.length() > 0 )
                    {
                        file = new RefFile( output_path );
                        env->getSession()->setProperty( Property( PROP_FILE, file ) );
                        osg::notify(osg::NOTICE) << "WriteText: writing to " << output_path << std::endl;
                    }
                }
            }
        }
        
        ScriptResult r = env->getScriptEngine()->run( getTextScript(), input, env );
        if ( r.isValid() )
        {
            file->write( r.asString() );
        }
    }

    return output;
}