/*
 * OcclusionBuffer.cpp
 *
 *  Created on: Aug 26, 2013
 *      Author: dan
 */

#include <cstdint>
#include <vector>
#include <algorithm>

#include "OcclusionBuffer.hpp"

namespace level_generator
{

OcclusionBuffer::OcclusionBuffer(int w, int h)
    :buffer(w*h), width(w), height(h){
};

bool OcclusionBuffer::isOccluded(int minx, int miny, int maxx, int maxy){
    int      line   = maxx-minx+1;
    int      lines  = maxy-miny+1;
    int      stride = width-line;
    uint8_t* p      = buffer.data()+minx+width*miny;

    // test region if any point is marked
    while(lines--){
        int templine = line;
        while(templine--){
            if (*p)
                return true;
            p++;
        }
        p+=stride;
    }
    return false;
}

void OcclusionBuffer::occlude(int minx, int miny, int maxx, int maxy){
    int      line   = maxx-minx+1;
    int      lines  = maxy-miny+1;
    int      stride = width-line;
    uint8_t* p      = buffer.data()+minx+width*miny;

    //mark region
    while(lines--){
        int templine = line;
        while(templine--){
            *p = 1;
            p++;
        }
        p+=stride;
    }
}

void OcclusionBuffer::clear(){
    std::fill( buffer.begin(), buffer.end(), 0 );
}

} /* namespace level_generator */
