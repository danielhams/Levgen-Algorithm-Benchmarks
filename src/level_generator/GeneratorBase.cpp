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

#include "GeneratorBase.hpp"

namespace level_generator
{

using std::string;
using std::ostream;
using std::ofstream;

Log LevelGeneratorBase::log;

ostream & operator<<( ostream & out, const vec2uint32 & vec2) {
    out << "(" << vec2.x << ", " << vec2.y << ")";
    return out;
}

ostream & operator<<( ostream & out, const vec4uint32 & vec4) {
    out << "(" << vec4.x << ", " << vec4.y << " [" << vec4.w << ", " << vec4.h << "])";
    return out;
}

ostream & operator<<( ostream & out, const LevelGeneratorConfiguration & lc ) {
    out << "seed(" << lc.seed << ") levelDimensions" << lc.levelDimension << " roomSizes(" <<
            lc.minroomsize << "->" << lc.maxroomsize << "[" << lc.roomSizeVariance << "]) numLevels(" << lc.numLevels <<
            ") numRooms(" << lc.numRooms << ") maxRoomAttempts(" << lc.maxRoomAttempts << ") numThreads(" << lc.numThreads << ")";
    return out;
}

Level::Level( const vec2uint32 dimension, const uint32_t numRooms ) :
    dimension( dimension ),
    tiles( dimension.x * dimension.y ) {
}

uint32_t Level::xy2i( const uint32_t x, const uint32_t y ) const {
    return (dimension.x * y) + x;
};

void Level::fillTiles() {
    for( vec4uint32 & r : rooms ) {
        for( uint32_t yi = r.y ; yi < ( r.y + r.h ) ; ++yi ) {
            for( uint32_t xi = r.x ; xi < ( r.x + r.w ) ; ++xi ) {
                tiles[ xy2i( xi, yi ) ] = true;
            }
        }
    }
};

Log GenerationStrategyBase::log;

void GenerationStrategyBase::debugRooms( Level & l ) {
    log() << "Debugging rooms: " << std::endl;
    for( vec4uint32 & r : l.rooms ) {
        log() << &r << "-" << r << std::endl;
    }
}

Log GenerationThreadLocalHelperBase::log;

void saveLevelPpm( const Level & level, const string & filename ) {
    ofstream ppmOut;
    ppmOut.open( filename.c_str(), std::ios_base::out | std::ios_base::binary );
    ppmOut << "P6\n" << level.dimension.x << " " << level.dimension.y << "\n255\n";
    unsigned char white[3] { 255, 255, 255}; // rgb order
    unsigned char lightGrey[3] { 192, 192, 192 };
    unsigned char darkGrey[3] { 28, 28, 28 };
    unsigned char black[3] { 0, 0, 0 };

    for( uint32_t y = 0 ; y < level.dimension.y ; ++y ) {
        for( uint32_t x = 0 ; x < level.dimension.x ; ++x ) {
            if( level.tiles[ (y*level.dimension.x) + x ] ) {
//                ppmOut.write( (const char*)white, 3 );
                ppmOut.write( (const char*)lightGrey, 3 );
            }
            else {
//                ppmOut.write( (const char*)black, 3 );
                ppmOut.write( (const char*)darkGrey, 3 );
            }
        }
    }
}

} /* namespace level_generator */
