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

#ifndef OCCLUSIONBUFFER_HPP_
#define OCCLUSIONBUFFER_HPP_

#include "GeneratorBase.hpp"

namespace level_generator
{

class OcclusionBuffer{

public:
    std::vector<uint8_t> buffer;
    const vec2uint32 dimensions;

    OcclusionBuffer( const vec2uint32 & dimensions );

    bool isOccluded( const vec4uint32 & toTest );
    bool isExpandedOccluded( const vec4uint32 & toTest );

    void occlude( const vec4uint32 & toOcclude );

    void clear();
};

} /* namespace level_generator */
#endif /* OCCLUSIONBUFFER_HPP_ */
