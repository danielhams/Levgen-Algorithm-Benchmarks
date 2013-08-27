/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS HEADER.
 *
 * Copyright 2013 Daniel Hams
 *
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this work; if not, see http://www.gnu.org/licenses/
 *
 */

#include <vector>
#include <iostream>
#include <sstream>

#include <mutex>

#include <boost/format.hpp>
#include <boost/thread.hpp>

#include "Utils.hpp"

#define MICROS_PER_SECOND ( 1000 * 1000 )

namespace level_generator
{

using std::string;
using std::stringstream;
using std::endl;
using std::ostream;
using std::streambuf;
using std::vector;
using std::cout;

using std::mutex;
using std::lock_guard;

using boost::format;
using boost::thread_specific_ptr;

class ThreadOutputStreamBuffer : public streambuf {
public:
    ThreadOutputStreamBuffer( size_t bufferSize = 512 ) :
        charBuffer_( bufferSize + 1 ) {
        char * charBufferBase( &(charBuffer_[0]) );
        setp( charBufferBase, charBufferBase + charBuffer_.size() - 1 );
    }

    virtual ~ThreadOutputStreamBuffer() {
    }

protected:
    bool pushAndFlush() {
        std::ptrdiff_t n( pptr() - pbase() );
        pbump( -n );
        lock_guard<mutex> lock( osMutex_ );
        bool success = cout.write( pbase(), n );
        cout.flush();
        return success;
    }

private:
    virtual ThreadOutputStreamBuffer::int_type overflow( ThreadOutputStreamBuffer::int_type ch = traits_type::eof() ) {
        if( ch != traits_type::eof() ) {
            *pptr() = ch;
            pbump(1);
            if( pushAndFlush() ) {
                return ch;
            }
        }
        return traits_type::eof();
    }

    virtual int sync() {
        return pushAndFlush() ? 0 : -1;
    }

    vector<char> charBuffer_;

    static mutex osMutex_;
};

mutex ThreadOutputStreamBuffer::osMutex_;

class ThreadOutputStream : public ostream {
public:
    ThreadOutputStream() :
        ostream( &buffer_ ) {};
    virtual ~ThreadOutputStream() {};

private:
    ThreadOutputStreamBuffer buffer_;
};

class LogPrivateImpl {
    const string & name_;
    static thread_specific_ptr<ThreadOutputStream> threadOutputStream_;
public:
    LogPrivateImpl( const string & name ) : name_( name ) {};

    ostream & operator()() {
        ThreadOutputStream * tos( threadOutputStream_.get() );
        if( tos == nullptr ) {
            tos = new ThreadOutputStream();
            threadOutputStream_.reset( tos );
        }
        return (*tos) << boost::posix_time::microsec_clock::universal_time() <<
                ' ' << format("%014s") % boost::this_thread::get_id() <<
                " [ " << format("%-20.20s") % name_ << " ] ";
    };
};

thread_specific_ptr<ThreadOutputStream> LogPrivateImpl::threadOutputStream_;

Log::Log( const std::string & name ) :
        name_( name ),
        pimpl_( std::auto_ptr<LogPrivateImpl>( new LogPrivateImpl( name_ ) ) ) {
}

std::ostream & Log::operator()() {
    return (*(pimpl_.get()))();
}

Log Timer::log("Timer");

void Timer::markBoundary( const string & sectorName )
{
    boost::posix_time::ptime newTimestamp( boost::posix_time::microsec_clock::universal_time() );
    timestamps_.push_back( newTimestamp );
    sectorNames_.push_back( sectorName );
}

void Timer::logTimes( const string & linePrefix, bool logSectors )
{
    // Will be used to reset and spit out the times we want
    boost::posix_time::ptime firstTimestamp, previousTimestamp;

    uint32_t sectorNum( 0 );

    for( boost::posix_time::ptime & tp : timestamps_ )
    {
        if( sectorNum != 0 )
        {
            string sectorName( sectorNames_[sectorNum] );
            boost::posix_time::time_duration sectorDuration( tp - previousTimestamp );

            if( logSectors )
            {
                long sms( sectorDuration.total_microseconds() );
                double inSeconds( (double)sms / MICROS_PER_SECOND );
                log() << linePrefix << "sector " << format("%02i") % sectorNum << ": " << format("%09f") % inSeconds << " seconds (" << sectorName << ")" << endl;
            }
        }
        else
        {
            firstTimestamp = tp;
        }

        previousTimestamp = tp;
        ++sectorNum;

    }
    boost::posix_time::time_duration totalDuration( previousTimestamp - firstTimestamp );
    long tms( totalDuration.total_microseconds() );
    double inSeconds( (double)tms / MICROS_PER_SECOND );
    log() << linePrefix << "total " << format("%09i") % inSeconds << " seconds" << endl;
}

} /* namespace level_generator */
