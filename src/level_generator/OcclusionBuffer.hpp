/*
 * OcclusionBuffer.hpp
 *
 *  Created on: Aug 26, 2013
 *      Author: dan
 */

#ifndef OCCLUSIONBUFFER_HPP_
#define OCCLUSIONBUFFER_HPP_

namespace level_generator
{

class OcclusionBuffer{
    std::vector<uint8_t> buffer;
    int width;
    int height;
public:
    OcclusionBuffer(int w, int h);

    bool isOccluded(int minx, int miny, int maxx, int maxy);

    void occlude(int minx, int miny, int maxx, int maxy);

    void clear();
};

} /* namespace level_generator */
#endif /* OCCLUSIONBUFFER_HPP_ */
