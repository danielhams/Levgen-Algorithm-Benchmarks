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

#ifndef GENERATORBASE_HPP_
#define GENERATORBASE_HPP_

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>

#include "Utils.hpp"

namespace level_generator
{

struct vec2uint32 {
    uint32_t x, y;
};

std::ostream & operator<<( std::ostream & out, const vec2uint32 & vec2);

struct vec4uint32 {
    uint32_t x, y, w, h;
};

inline bool intersects( const vec4uint32 & a, const vec4uint32 & t ) {
    if( !( ((a.x + a.w ) < t.x || a.x > (t.x + t.w )) ||
           ((a.y + a.h ) < t.y || a.y > (t.y + t.h )) ) ) {
        return true;
    }
    else {
        return false;
    }
};

std::ostream & operator<<( std::ostream & out, const vec4uint32 & vec4);

struct LevelGeneratorConfiguration {
    const uint32_t seed;
    const vec2uint32 levelDimension;
    const vec2uint32 levelDimensionMinusOne;
    const uint32_t minroomsize;
    const uint32_t minroomsizeplustwo;
    const uint32_t maxroomsize;
    const uint32_t roomSizeVariance;
    const uint32_t numLevels;
    const uint32_t numRooms;
    const uint32_t maxRoomAttempts;
    const uint32_t numThreads;
};

std::ostream & operator<<( std::ostream & out, const LevelGeneratorConfiguration & lc );

struct Room : public vec4uint32 {
    Room( uint32_t & randGen, const vec4uint32 & bounds, const uint8_t rgbi[3] );
    uint8_t rgb[3];
};

class Level
{
public:
    Level( const vec2uint32 dimension, const uint32_t numRooms );
    uint32_t xy2i( const uint32_t x, const uint32_t y ) const;

    void fillTiles();

    vec2uint32 dimension;
    std::vector<Room> rooms;
    std::vector<Room*> tiles;
};

class LevelGeneratorBase {
    static Log log;
};

template <typename GenerationStrategy, typename ThreadLocalGenerationHelper>
class LevelGenerator : LevelGeneratorBase
{
    const LevelGeneratorConfiguration configuration_;
    GenerationStrategy generationStrategy_;
    std::vector<Level> levels_;

    void generateLevelsForPartition_( uint32_t seed, const uint32_t partitionStartIndex, const uint32_t partitionEndIndex ) {
        ThreadLocalGenerationHelper threadLocalGenerationHelper( generationStrategy_.newThreadLocalHelper() );

//        log() << "Would generate with seed " << seed << " levels(" << partitionStartIndex << "->" << partitionEndIndex << ")" << std::endl;
        for( uint32_t i = partitionStartIndex ; i < partitionEndIndex ; i++ ) {
            Level & l( levels_[i] );
            generationStrategy_.fillLevel( threadLocalGenerationHelper, l, seed );
        }
    }

public:
    LevelGenerator( const LevelGeneratorConfiguration & configuration,
            GenerationStrategy generationStrategy ) :
                configuration_( configuration ),
                generationStrategy_( generationStrategy ) {};

    void generateLevels() {
        levels_.clear();
        levels_.resize( configuration_.numLevels, Level( configuration_.levelDimension, configuration_.numRooms ) );
        std::vector<std::thread> threads;
        std::vector<uint32_t> numLevelsPerThread( configuration_.numThreads, 0 );
        // Distribute the level work among the threads favouring the earlier threads
        // for any extra work
        uint32_t curThreadNum( 0 );
        for( uint32_t ln = 0 ; ln < configuration_.numLevels ; ++ln ) {
            numLevelsPerThread[ curThreadNum ]++;
            curThreadNum++;
            if( curThreadNum >= configuration_.numThreads ) {
                curThreadNum = 0;
            }
        }

        uint32_t partitionStartIndex( 0 );

        for( uint32_t i = 0 ; i < configuration_.numThreads ; ++i ) {
            uint32_t threadSeed( configuration_.seed * ( (i+1)*(i+1)) );
            uint32_t numLevelsThisThread = numLevelsPerThread[ i ];
            uint32_t partitionEndIndex( partitionStartIndex + numLevelsThisThread );

            threads.emplace_back(
                    std::bind( &LevelGenerator<GenerationStrategy,ThreadLocalGenerationHelper>::generateLevelsForPartition_,
                        this,
                        threadSeed,
                        partitionStartIndex,
                        partitionStartIndex + numLevelsThisThread )
            );

            partitionStartIndex += numLevelsThisThread;
        }

        for( std::thread & t : threads ) {
            t.join();
        }
    }

    template <typename LevelMetric>
    Level & pickLevelByCriteria( LevelMetric levelMetric ) {
        auto lIter = levels_.begin(), lEnd = levels_.end();
        Level * result( &(*lIter) );
        lIter++;
        for( ; lIter != lEnd ; ++lIter ) {
            if( levelMetric.compare( *result, *lIter ) ) result = &(*lIter);
        }
        return *result;
    }
};

class GenerationStrategyBase {
    static Log log;
public:
    void debugRooms( Level & l );
};

class GenerationThreadLocalHelperBase {
protected:
    static Log log;
};


struct NumRoomsMetric {
    inline bool compare( const Level & x, const Level & y ) {
        return y.rooms.size() > x.rooms.size();
    }
};

struct MinSpaceMetric {
    inline uint32_t countSpaceForLevel( const Level & l ) {
        uint32_t spaceCount( 0 );
        for( uint32_t i ; i < l.tiles.size() ; ++i ) {
            if( l.tiles[i] == nullptr ) {
                spaceCount++;
            }
        }
        return spaceCount;
    }

    inline bool compare( const Level & x, const Level & y ) {
        uint32_t ySpaceCount( countSpaceForLevel( y ) );
        uint32_t xSpaceCount( countSpaceForLevel( x ) );
        return ySpaceCount < xSpaceCount;
    }
};

void saveLevelPpm( const Level & level, const std::string & filename );

} /* namespace level_generator */
#endif /* GENERATORBASE_HPP_ */
