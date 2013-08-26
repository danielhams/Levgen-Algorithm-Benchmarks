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
#include <vector>
#include <algorithm>

#include "OcclusionBuffer.hpp"

namespace level_generator
{

OcclusionBuffer::OcclusionBuffer( const vec2uint32 & dimensions ) :
        buffer(dimensions.x * dimensions.y),
        dimensions( dimensions ) {
};

bool OcclusionBuffer::isOccluded( const vec4uint32 & toTest ){
    for( uint32_t y = toTest.y ; y < toTest.y + toTest.h ; ++y ) {
        for( uint32_t x = toTest.x ; x < toTest.x + toTest.w ; ++x ) {
            if( buffer[ (y*dimensions.x) + x ] ) {
                return true;
            }
        }
    }
    return false;
}

bool OcclusionBuffer::isExpandedOccluded( const vec4uint32 & toTest ){
    for( uint32_t y = toTest.y-1 ; y <= toTest.y + toTest.h ; ++y ) {
        for( uint32_t x = toTest.x-1 ; x <= toTest.x + toTest.w ; ++x ) {
            if( buffer[ (y*dimensions.x) + x ] ) {
                return true;
            }
        }
    }
    return false;
}

void OcclusionBuffer::occlude( const vec4uint32 & toTest ) {
    for( uint32_t y = toTest.y ; y < toTest.y + toTest.h ; ++y ) {
        for( uint32_t x = toTest.x ; x < toTest.x + toTest.w ; ++x ) {
            buffer[ (y*dimensions.x) + x ] = 1;
        }
    }
}

void OcclusionBuffer::clear(){
    std::fill( buffer.begin(), buffer.end(), 0 );
}

} /* namespace level_generator */
