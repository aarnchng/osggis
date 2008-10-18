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

#include <osgGIS/Report>
#include <sstream>

using namespace osgGIS;

Report::Report() :
state( STATE_OK ),
first_start_time( 0 ),
start_time( 0 ),
end_time( 0 )
{
    //NOP
}

Report::Report( const Report& rhs )
: name( rhs.name ),
state( rhs.state ),
first_start_time( rhs.first_start_time ),
start_time( rhs.start_time ),
end_time( rhs.end_time ),
durations( rhs.durations ),
sub_reports( rhs.sub_reports ),
messages( rhs.messages ),
properties( rhs.properties )
{
    //NOP
}

Report::~Report()
{
    //NOP
}

void
Report::setName( const std::string& value )
{
    name = value;
}

const std::string&
Report::getName() const
{
    return name;
}

void
Report::setState( State new_state, bool force_upgrade )
{
    if ( new_state > state || force_upgrade )
        state = new_state;
}

Report::State
Report::getState() const
{
    return state;
}

void
Report::markStartTime()
{
    start_time = osg::Timer::instance()->tick();
    if ( first_start_time == 0 )
        first_start_time = start_time;
}

void
Report::markEndTime()
{
    end_time = osg::Timer::instance()->tick();
    durations.push_back( osg::Timer::instance()->delta_s( start_time, end_time ) );
}

double
Report::getDuration() const
{
    double total = 0.0;
    for( std::list<double>::const_iterator i = durations.begin(); i != durations.end(); i++ )
        total += *i;
    return total;
}

double
Report::getAverageDuration() const
{
    return durations.size() > 0? getDuration() / durations.size() : 0.0;
}

double
Report::getMaxDuration() const
{
    double most = 0.0;
    for( std::list<double>::const_iterator i = durations.begin(); i != durations.end(); i++ )
        if ( *i > most ) most = *i;
    return most;
}

double
Report::getMinDuration() const
{
    double least = 0.0;
    for( std::list<double>::const_iterator i = durations.begin(); i != durations.end(); i++ )
        if ( least == 0.0 || *i < least ) least = *i;
    return least;
}

const ReportList&
Report::getSubReports() const
{
    return sub_reports;
}

void
Report::addSubReport( Report* sub_report )
{
    sub_reports.push_back( sub_report );
}

void
Report::setProperty( const Property& p )
{
    properties.set( p );
}

const Properties&
Report::getProperties() const
{
    return properties;
}


void
Report::notice( const std::string& msg )
{
    std::stringstream buf;
    buf << "NOTICE: " << msg;
    messages.push_back( msg );
}

void 
Report::warning( const std::string& msg )
{
    std::stringstream buf;
    buf << "WARNING: " << msg;
    messages.push_back( msg );
    setState( STATE_WARNING );
}

void 
Report::error( const std::string& msg )
{
    std::stringstream buf;
    buf << "ERROR: " << msg;
    messages.push_back( msg );
    setState( STATE_ERROR );
}

const std::list<std::string>&
Report::getMessages() const
{
    return messages;
}
