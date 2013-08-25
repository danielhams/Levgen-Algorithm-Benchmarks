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

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <iostream>
#include <mutex>
#include <vector>
#include <memory>

#include <boost/date_time.hpp>

namespace level_generator
{

class LogPrivateImpl;

// Should be multi-threaded, but isn't and I didn't want to introduce another dependency
class Log {
    std::auto_ptr<LogPrivateImpl> pimpl_;
    Log( const Log & );
    Log & operator=(const Log &);
public:
    Log();
    std::ostream & operator()();
};

class Timer {
public:

    void markBoundary( const std::string & sectorName );
    void logTimes( const std::string & linePrefix, bool logSectors = true );

private:
    std::vector<boost::posix_time::ptime> timestamps_;
    std::vector<std::string> sectorNames_;

    static Log log;
};

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} /* namespace level_generator */
#endif /* UTILS_HPP_ */
