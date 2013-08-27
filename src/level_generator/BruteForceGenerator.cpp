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

#include "BruteForceGenerator.hpp"

namespace level_generator
{

SimpleCollisionThreadLocalHelper::SimpleCollisionThreadLocalHelper( const LevelGeneratorConfiguration & configuration ) {
};

bool SimpleCollisionThreadLocalHelper::isCollision( Level & level, const vec4uint32 & t ) {
    for( vec4uint32 & r : level.rooms ) {
        if( intersects( r, t ) ) {
//            if( !( ((r.x + r.w ) < t.x || r.x > (t.x + t.w )) ||
//                   ((r.y + r.h ) < t.y || r.y > (t.y + t.h )) ) ) {
            return true;
        }
    }
    return false;
}

void SimpleCollisionThreadLocalHelper::addRoom( vec4uint32 * r ) {
}

void SimpleCollisionThreadLocalHelper::clearForNewLevel() {
}

FixedLevelQuadTreeThreadLocalHelper::FixedLevelQuadTreeThreadLocalHelper( const LevelGeneratorConfiguration & configuration, const uint32_t treeDepth ) :
                configuration_( configuration ),
                quadTree_( treeDepth, configuration_.levelDimension.x, configuration_.levelDimension.y ) {
}

bool FixedLevelQuadTreeThreadLocalHelper::isCollision( Level & l, const vec4uint32 & roomDim ) {
    return quadTree_.checkForCollision( roomDim );
}

void FixedLevelQuadTreeThreadLocalHelper::addRoom( vec4uint32 * r ) {
#ifdef BF_DEBUG
    log() << "Adding room " << r << std::endl;
#endif
    quadTree_.insertContent( r );
}

void FixedLevelQuadTreeThreadLocalHelper::clearForNewLevel() {
    quadTree_.clear();
}

OcclusionThreadLocalHelper::OcclusionThreadLocalHelper( const LevelGeneratorConfiguration & configuration ) :
        configuration_( configuration ),
        occlusionBuffer_( configuration_.levelDimension ) {
}

bool OcclusionThreadLocalHelper::isCollision( Level & l, const vec4uint32 & roomDim ) {
    return occlusionBuffer_.isExpandedOccluded( roomDim );
}

void OcclusionThreadLocalHelper::addRoom( vec4uint32 * r ) {
    occlusionBuffer_.occlude( *r );
}

void OcclusionThreadLocalHelper::clearForNewLevel() {
    occlusionBuffer_.clear();
}

} /* namespace level_generator */
