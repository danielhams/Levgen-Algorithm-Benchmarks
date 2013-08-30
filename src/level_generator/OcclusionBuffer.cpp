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

#include <cstdint>
#include <sstream>
#include <vector>
#include <algorithm>

#include "OcclusionBuffer.hpp"

/* #define OC_DEBUG */

namespace level_generator
{

using std::endl;
using std::stringstream;

Log OcclusionBuffer::log("OcclusionBuffer");

OcclusionBuffer::OcclusionBuffer( const vec2uint32 & dimensions ) :
        buffer(dimensions.x * dimensions.y),
        dimensions( dimensions ) {
};

bool OcclusionBuffer::isOccluded( const vec4uint32 & toTest ){
    for( uint32_t y = toTest.y ; y < toTest.y + toTest.h ; ++y ) {
        for( uint32_t x = toTest.x ; x < toTest.x + toTest.w ; ++x ) {
            if( buffer[ (y*dimensions.x) + x ] != ' ') {
                return true;
            }
        }
    }
    return false;
}

bool OcclusionBuffer::isExpandedOccluded( const vec4uint32 & toTest ){
#ifdef OC_DEBUG
    log() << "Asked to expanded occlude " << toTest << endl;
#endif
    for( uint32_t y = toTest.y-1 ; y <= toTest.y + toTest.h ; ++y ) {
        for( uint32_t x = toTest.x-1 ; x <= toTest.x + toTest.w ; ++x ) {
            if( buffer[ (y*dimensions.x) + x ] != ' ') {
                return true;
            }
        }
    }
    return false;
}

void OcclusionBuffer::occlude( const vec4uint32 & toTest ) {
#ifdef OC_DEBUG
    log() << "Asked to occlude " << toTest << endl;
#endif
    for( uint32_t y = toTest.y ; y < toTest.y + toTest.h ; ++y ) {
        for( uint32_t x = toTest.x ; x < toTest.x + toTest.w ; ++x ) {
            buffer[ (y*dimensions.x) + x ] = 'O';
        }
    }
}

void OcclusionBuffer::occludeWithBorder( const vec4uint32 & toTest ) {
#ifdef OC_DEBUG
    log() << "Asked to occlude with border" << toTest << endl;
#endif
    occlude( toTest );
    for( uint32_t x = toTest.x - 1 ; x < toTest.x + toTest.w + 1; ++x ) {
        buffer[ ((toTest.y-1)*dimensions.x) + x ] = 'B';
        buffer[ ((toTest.y + toTest.h )*dimensions.x) + x ] = 'B';
    }
    for( uint32_t y = toTest.y ; y < toTest.y + toTest.h ; ++y ) {
        buffer[ (y*dimensions.x) + (toTest.x - 1) ] = 'B';
        buffer[ (y*dimensions.x) + (toTest.x + toTest.w) ] = 'B';
    }
}

void OcclusionBuffer::clear(){
    std::fill( buffer.begin(), buffer.end(), ' ' );
}

void OcclusionBuffer::clearWithBorders() {
    clear();
    for( uint32_t x = 0 ; x < dimensions.x ; ++x ) {
        buffer[ x ] = 'B';
        buffer[ ((dimensions.y-1)*dimensions.x) + x ] = 'B';
    }
    for( uint32_t y = 1 ; y < dimensions.y - 1 ; ++y ) {
        buffer[ (y*dimensions.x) ] = 'B';
        buffer[ (y*dimensions.x) + (dimensions.x-1) ] = 'B';
    }
}

void OcclusionBuffer::debug() const {
    stringstream ss( stringstream::out );
    ss <<  "Buffer is currently:" << std::endl;
    for( uint32_t y = 0 ; y < dimensions.y ; ++y ) {
        for( uint32_t x = 0 ; x < dimensions.x ; ++x ) {
            const char & val( buffer[ (dimensions.x * y) + x ] );
            ss << val;
        }
        ss << std::endl;
    }
    log() << ss.str();
}

} /* namespace level_generator */
