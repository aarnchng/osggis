/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/
#include <osgGIS/Notify>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>

using namespace std;

osg::NotifySeverity g2_NotifyLevel = osg::NOTICE;

void osgGIS::setNotifyLevel(osg::NotifySeverity severity)
{
    osgGIS::initNotifyLevel();
    g2_NotifyLevel = severity;
}


osg::NotifySeverity osgGIS::getNotifyLevel()
{
    osgGIS::initNotifyLevel();
    return g2_NotifyLevel;
}


bool osgGIS::initNotifyLevel()
{
    static bool s_NotifyInit = false;

    if (s_NotifyInit) return true;
    
    // g_NotifyLevel
    // =============

    g2_NotifyLevel = osg::NOTICE; // Default value

    char* OSGNOTIFYLEVEL=getenv("OSGGIS_NOTIFY_LEVEL");
    if (!OSGNOTIFYLEVEL) OSGNOTIFYLEVEL=getenv("OSGGISNOTIFYLEVEL");
    if(OSGNOTIFYLEVEL)
    {

        std::string stringOSGNOTIFYLEVEL(OSGNOTIFYLEVEL);

        // Convert to upper case
        for(std::string::iterator i=stringOSGNOTIFYLEVEL.begin();
            i!=stringOSGNOTIFYLEVEL.end();
            ++i)
        {
            *i=toupper(*i);
        }

        if(stringOSGNOTIFYLEVEL.find("ALWAYS")!=std::string::npos)          g2_NotifyLevel=osg::ALWAYS;
        else if(stringOSGNOTIFYLEVEL.find("FATAL")!=std::string::npos)      g2_NotifyLevel=osg::FATAL;
        else if(stringOSGNOTIFYLEVEL.find("WARN")!=std::string::npos)       g2_NotifyLevel=osg::WARN;
        else if(stringOSGNOTIFYLEVEL.find("NOTICE")!=std::string::npos)     g2_NotifyLevel=osg::NOTICE;
        else if(stringOSGNOTIFYLEVEL.find("DEBUG_INFO")!=std::string::npos) g2_NotifyLevel=osg::DEBUG_INFO;
        else if(stringOSGNOTIFYLEVEL.find("DEBUG_FP")!=std::string::npos)   g2_NotifyLevel=osg::DEBUG_FP;
        else if(stringOSGNOTIFYLEVEL.find("DEBUG")!=std::string::npos)      g2_NotifyLevel=osg::DEBUG_INFO;
        else if(stringOSGNOTIFYLEVEL.find("INFO")!=std::string::npos)       g2_NotifyLevel=osg::INFO;
        else std::cout << "Warning: invalid OSG_NOTIFY_LEVEL set ("<<stringOSGNOTIFYLEVEL<<")"<<std::endl;
 
    }

    s_NotifyInit = true;

    return true;

}

bool osgGIS::isNotifyEnabled( osg::NotifySeverity severity )
{
    return severity<=g2_NotifyLevel;
}

class NullStreamBuffer : public std::streambuf
{
    private:
    
        virtual streamsize xsputn (const char_type*, streamsize n)
        {
            return n;
        }
};

std::ostream& osgGIS::notify(const osg::NotifySeverity severity)
{
    // set up global notify null stream for inline notify
    static std::ostream s2_NotifyNulStream(new NullStreamBuffer());

    static bool initialized = false;
    if (!initialized) 
    {
        std::cerr<<""; // dummy op to force construction of cerr, before a reference is passed back to calling code.
        std::cout<<""; // dummy op to force construction of cout, before a reference is passed back to calling code.
        initialized = osgGIS::initNotifyLevel();
    }

    if (severity<=g2_NotifyLevel)
    {
        std::cout << "[osgGIS] ";
        switch(severity)
        {            
        case osg::FATAL:
            std::cout << "*** ERROR *** ";
            break;
        case osg::WARN:
            std::cout << "WARN: ";
            break;
        case osg::NOTICE:
            break;
        case osg::INFO:
            std::cout << "INFO: ";
            break;
        default:
            std::cout << "DEBUG: ";
        }

        return std::cout;
    }
    return s2_NotifyNulStream;
}


std::ostream& 
osgGIS::debug() { return osgGIS::notify(osg::DEBUG_INFO); }

std::ostream& 
osgGIS::info() { return osgGIS::notify(osg::INFO); }

std::ostream&
osgGIS::notice() { return osgGIS::notify(osg::NOTICE); }

std::ostream& 
osgGIS::warn() { return osgGIS::notify(osg::WARN); }

