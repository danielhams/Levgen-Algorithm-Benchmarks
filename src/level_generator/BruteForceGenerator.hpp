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

#ifndef BRUTEFORCEGENERATOR_HPP_
#define BRUTEFORCEGENERATOR_HPP_

#include "GeneratorBase.hpp"
#include "QuadTree.hpp"
#include "OcclusionBuffer.hpp"

namespace level_generator
{

template <typename RandGenerator, typename ThreadLocalGenerationHelper>
class BruteForceGenerationStrategy : public GenerationStrategyBase {
    LevelGeneratorConfiguration configuration_;
    RandGenerator randGenerator_;

    bool makeRoomSilentlyFail_( ThreadLocalGenerationHelper & threadLocalGenerationHelper, Level & l, uint32_t & gen ) {

        uint32_t w= ( (randGenerator_( gen ) % (configuration_.roomSizeVariance)) + configuration_.minroomsize);
        uint32_t h = ( (randGenerator_( gen ) % (configuration_.roomSizeVariance)) + configuration_.minroomsize );
        uint32_t x = ( (randGenerator_( gen ) % (configuration_.levelDimensionMinusOne.x - w)) + 1 );
        uint32_t y = ( (randGenerator_( gen ) % (configuration_.levelDimensionMinusOne.y - h)) + 1 );

//        log() << roomDim << " constructed - will now check" << std::endl;
        vec4uint32 roomDim { x, y, w, h };

        if( !threadLocalGenerationHelper.isCollision( l, roomDim ) ) {
//            log() << roomDim << " begin added" << std::endl;
            l.rooms.emplace_back( roomDim );
            threadLocalGenerationHelper.addRoom( &(l.rooms[ l.rooms.size() - 1 ]) );
            return true;
        }
        else {
//            log() << roomDim << " COLLIDES - won't add." << std::endl;
            return false;
        }
    };

public:
    BruteForceGenerationStrategy( LevelGeneratorConfiguration & configuration,
            RandGenerator randGenerator ) :
        configuration_( configuration ),
        randGenerator_( randGenerator ) {};

    void fillLevel( ThreadLocalGenerationHelper & threadLocalCollisionTester, Level & level, uint32_t & gen ) {
        level.rooms.reserve( configuration_.numRooms );
        uint32_t numRoomsMade( 0 );
        for( uint32_t ii = 0 ; ii < 50000 ; ++ii ) {
//            debugRooms( level );
            numRoomsMade += static_cast<uint32_t>( makeRoomSilentlyFail_( threadLocalCollisionTester, level, gen ) );
            if( numRoomsMade >= configuration_.numRooms ) {
                break;
            }
        }
        level.fillTiles();
    };
};

struct SimpleCollisionThreadLocalHelper : public GenerationThreadLocalHelperBase {
    SimpleCollisionThreadLocalHelper( const LevelGeneratorConfiguration & configuration );

    bool isCollision( Level & level, const vec4uint32 & t );

    void addRoom( vec4uint32 * r );

    void clearForNewLevel();
};

class FixedLevelQuadTreeThreadLocalHelper : public GenerationThreadLocalHelperBase {
    const LevelGeneratorConfiguration configuration_;
    QuadTree<vec4uint32> quadTree_;

public:
    FixedLevelQuadTreeThreadLocalHelper( const LevelGeneratorConfiguration & configuration, const uint32_t treeDepth = 3 );

    bool isCollision( Level & l, const vec4uint32 & roomDim );

    void addRoom( vec4uint32 * r );

    void clearForNewLevel();
};

class OcclusionThreadLocalHelper : public GenerationThreadLocalHelperBase {
    const LevelGeneratorConfiguration configuration_;
    OcclusionBuffer occlusionBuffer_;

public:
    OcclusionThreadLocalHelper( const LevelGeneratorConfiguration & configuration );

    bool isCollision( Level & l, const vec4uint32 & roomDim );

    void addRoom( vec4uint32 * r );

    void clearForNewLevel();
};

} /* namespace level_generator */
#endif /* BRUTEFORCEGENERATOR_HPP_ */
