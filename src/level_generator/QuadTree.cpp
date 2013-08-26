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

#include "QuadTree.hpp"

namespace level_generator
{

AABB::AABB() : ex( 0 ), ey( 0 ), doublemidx( 0 ), doublemidy( 0 ) {
};

AABB::AABB( vec4uint32 dimensions ) :
    dimensions( dimensions ),
    ex( dimensions.x + dimensions.w ),
    ey( dimensions.y + dimensions.h ),
    doublemidx( (2 * dimensions.x) + dimensions.w ),
    doublemidy( (2 * dimensions.y) + dimensions.h ) {
}

bool AABB::intersects( const AABB & c ) const {
    // We use 2*vals so we can stick with integer arith
    // (since we could have things where width = 1 or height = 1)
    int32_t xdd( c.doublemidx - doublemidx );
    int32_t ydd( c.doublemidy - doublemidy );

    return std::abs(xdd) <= (dimensions.w + c.dimensions.w) && std::abs(ydd) <= (dimensions.h + c.dimensions.h);
}

bool AABB::intersects( const vec4uint32 & r ) const {
    int32_t xdd( ((2*r.x)+r.w) - doublemidx );
    int32_t ydd( ((2*r.y)+r.h) - doublemidy );

    return std::abs(xdd) <= (dimensions.w + r.w) && std::abs(ydd) <= (dimensions.h + r.h);
}

Log QuadTreeNodeBase::log;

Log QuadTreeBase::log;

} /* namespace level_generator */
