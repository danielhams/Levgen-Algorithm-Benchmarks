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

#include "level_generator.hpp"

#include "../../config.h"

#include <iostream>

#include <boost/program_options.hpp>

#include "Utils.hpp"
#include "RandGenerators.hpp"
#include "GeneratorBase.hpp"
#include "BruteForceGenerator.hpp"
#include "FreeListGenerator.hpp"

using std::cout;
using std::endl;
using std::string;
using std::stringstream;

using level_generator::vec2uint32;
using level_generator::vec4uint32;
using level_generator::Log;
using level_generator::Timer;
using level_generator::saveLevelPpm;

using level_generator::CRandGenerator;
using level_generator::GenRandGenerator;

using level_generator::NumRoomsMetric;
using level_generator::Level;
using level_generator::LevelGeneratorConfiguration;
using level_generator::LevelGenerator;

using level_generator::BruteForceGenerationStrategy;
using level_generator::SimpleCollisionThreadLocalHelper;
using level_generator::FixedLevelQuadTreeThreadLocalHelper;
using level_generator::OcclusionThreadLocalHelper;

using level_generator::FreeListMinSizeSelector;
using level_generator::FreeListMaxSizeSelector;
using level_generator::FreeListThreadLocalHelper;
using level_generator::FreeListGenerationStrategy;

namespace po = boost::program_options;

const string version( PACKAGE_VERSION );
const string abiVersion( ABI_VERSION );

namespace level_generator
{
}

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio( false );
    Log log("Main");
    log() << "level_generator beginning ver(" << version << ") abiVer(" << abiVersion << ")" << endl;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Show this help message")
        ("dimx", po::value<int>(), "Level dimension in x (10<=N<=8192)")
        ("dimy", po::value<int>(), "Level dimension in y")
        ("smallroom", po::value<int>(), "Minimum room size (> 3)")
        ("bigroom", po::value<int>(), "Maximum room size (N<=(min dim -2)")
        ("levels", po::value<int>(), "Number of levels to generate (1<=N<=5000)")
        ("maxrooms", po::value<int>(), "Maximum number of rooms per level (1<=N<=100000)")
        ("maxroomattempts", po::value<int>(), "Maximum number of attempts to create a room per level (1<=N<=100000)")
        ("threads", po::value<int>(), "Number of threads (1<=N<=4)")
        ("seed", po::value<int>(), "Set the random number seed");

    po::variables_map vm;
    po::store( po::parse_command_line( argc, argv, desc), vm );
    po::notify( vm );

    bool showUsageSummary = false;
    int32_t seed = -1;
    int32_t dimensionx = -1;
    int32_t dimensiony = -1;
    int32_t minroomsize = -1;
    int32_t maxroomsize = -1;
    int32_t numLevels = -1;
    int32_t numRooms = -1;
    int32_t maxRoomAttempts = -1;
    int32_t numThreads = -1;
    if( vm.size() == 0 || vm.count("help"))
    {
        showUsageSummary = true;
    }
    else
    {
        try {
            // Validate the parameters
            dimensionx = vm["dimx"].as<int>();
            if( dimensionx < 3 || dimensionx > 8192 )
            {
                showUsageSummary = true;
            }
            dimensiony = vm["dimy"].as<int>();
            if( dimensiony < 3 || dimensiony > 8192 )
            {
                showUsageSummary = true;
            }
            minroomsize = vm["smallroom"].as<int>();
            if( minroomsize < 1 || minroomsize > 100 )
            {
                showUsageSummary = true;
            }
            maxroomsize = vm["bigroom"].as<int>();
            if( maxroomsize < 1 || maxroomsize > 100 )
            {
                showUsageSummary = true;
            }
            numLevels = vm["levels"].as<int>();
            if( numLevels < 1 || numLevels > 5000 )
            {
                showUsageSummary = true;
            }
            numRooms = vm["maxrooms"].as<int>();
            if( numRooms < 1 || numRooms > 100000 )
            {
                showUsageSummary = true;
            }
            maxRoomAttempts = vm["maxroomattempts"].as<int>();
            if( maxRoomAttempts < 1 || maxRoomAttempts > 100000 )
            {
                showUsageSummary = true;
            }
            numThreads = vm["threads"].as<int>();
            if( numThreads < 1 || numThreads > 4 )
            {
                showUsageSummary = true;
            }
            seed = vm["seed"].as<int>();
            if( seed < 0 || seed > INT_MAX )
            {
                showUsageSummary = true;
            }
        }
        catch( boost::bad_any_cast & bbac ) {
            showUsageSummary = true;
        }

        if( numThreads > numLevels ) {
            log() << "Must have at least as many threads as there are levels!" << endl;
            showUsageSummary = true;
        }

        if( minroomsize == maxroomsize || minroomsize > maxroomsize ) {
            log() << "Room min < max" << endl;
            showUsageSummary = true;
        }

        if( maxroomsize >= dimensionx || maxroomsize >= dimensiony ) {
            log() << "Max room size >= (level dimension - 2)" << endl;
            showUsageSummary = true;
        }
    }

    if( showUsageSummary )
    {
        stringstream descss;
        descss << desc;
        string descstr( descss.str() );
        log() << descstr << endl;
        return 1;
    }
    else
    {
        srand( seed );

        LevelGeneratorConfiguration lc { (uint32_t)seed,
            vec2uint32 { (uint32_t)dimensionx, (uint32_t)dimensiony },
            vec2uint32 { (uint32_t)dimensionx - 1, (uint32_t)dimensiony - 1},
            (uint32_t)minroomsize,
            (uint32_t)(minroomsize + 2),
            (uint32_t)maxroomsize,
            (uint32_t)(maxroomsize - minroomsize),
            (uint32_t)numLevels,
            (uint32_t)numRooms,
            (uint32_t)maxRoomAttempts,
            (uint32_t)numThreads };

        log() << "LevelGeneratorConfiguration: " << lc << endl;

        NumRoomsMetric roomsMetric;

        CRandGenerator cRandGenerator;
        GenRandGenerator genRandGenerator;

        bool focusOnFreeList( false );

        for( uint32_t i = 0 ; i < 1 ; ++i ) {

            Timer timer;
            timer.markBoundary( "Begin" );

            {
                timer.markBoundary( "Begin genrand simple" );
                BruteForceGenerationStrategy<GenRandGenerator, SimpleCollisionThreadLocalHelper> bruteForceGenrandGenerationStrategy( lc, genRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<GenRandGenerator, SimpleCollisionThreadLocalHelper>, SimpleCollisionThreadLocalHelper>
                    bfgrLevelGenerator( lc, bruteForceGenrandGenerationStrategy );
                bfgrLevelGenerator.generateLevels();
                Level & bfgrLevel( bfgrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce SimpleCT GenRand" );
                saveLevelPpm( bfgrLevel, "bruteforcesimplectgenrand.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "bf gr rooms " << bfgrLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning crand simple" );
                BruteForceGenerationStrategy<CRandGenerator, SimpleCollisionThreadLocalHelper> bruteForceCrandGenerationStrategy( lc, cRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<CRandGenerator, SimpleCollisionThreadLocalHelper>, SimpleCollisionThreadLocalHelper > bfcrLevelGenerator( lc, bruteForceCrandGenerationStrategy );
                bfcrLevelGenerator.generateLevels();
                Level & bfcrLevel( bfcrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce SimpleCT CRand" );
                saveLevelPpm( bfcrLevel, "bruteforcesimplectcrand.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "bf cr rooms " << bfcrLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning genrand quadtree" );
                BruteForceGenerationStrategy<GenRandGenerator, FixedLevelQuadTreeThreadLocalHelper> bruteForceQuadTreeStrategy( lc, genRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<GenRandGenerator, FixedLevelQuadTreeThreadLocalHelper>, FixedLevelQuadTreeThreadLocalHelper>
                    bfqtLevelGenerator( lc, bruteForceQuadTreeStrategy );
                bfqtLevelGenerator.generateLevels();
                Level & bfqtLevel( bfqtLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce QuadTree GenRand" );
                saveLevelPpm( bfqtLevel, "bruteforcefixedquadtreegenrand.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "bf qt gr rooms " << bfqtLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning crand quadtree" );
                BruteForceGenerationStrategy<CRandGenerator, FixedLevelQuadTreeThreadLocalHelper> cbruteForceQuadTreeStrategy( lc, cRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<CRandGenerator, FixedLevelQuadTreeThreadLocalHelper>, FixedLevelQuadTreeThreadLocalHelper>
                    cbfqtLevelGenerator( lc, cbruteForceQuadTreeStrategy );
                cbfqtLevelGenerator.generateLevels();
                Level & cbfqtLevel( cbfqtLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce QuadTree CRand" );
                saveLevelPpm( cbfqtLevel, "bruteforcefixedquadtreecrand.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "bf qt cr rooms " << cbfqtLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning genrand occlusion buffer" );
                BruteForceGenerationStrategy<GenRandGenerator, OcclusionThreadLocalHelper> gbruteForceOcclusionBufferStrategy( lc, genRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<GenRandGenerator, OcclusionThreadLocalHelper>, OcclusionThreadLocalHelper>
                    gbfobLevelGenerator( lc, gbruteForceOcclusionBufferStrategy );
                gbfobLevelGenerator.generateLevels();
                Level & gbfobLevel( gbfobLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce Occlussion Buffer GenRand" );
                saveLevelPpm( gbfobLevel, "bruteforceocclusionbuffergenrand.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "bf ob gr rooms " << gbfobLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning crand occlusion buffer" );
                BruteForceGenerationStrategy<CRandGenerator, OcclusionThreadLocalHelper> cbruteForceOcclusionBufferStrategy( lc, cRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<CRandGenerator, OcclusionThreadLocalHelper>, OcclusionThreadLocalHelper>
                    cbfobLevelGenerator( lc, cbruteForceOcclusionBufferStrategy );
                cbfobLevelGenerator.generateLevels();
                Level & cbfobLevel( cbfobLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce Occlussion Buffer CRand" );
                saveLevelPpm( cbfobLevel, "bruteforceocclusionbuffercrand.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "bf ob cr rooms " << cbfobLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning genrand free list min size" );
                typedef FreeListThreadLocalHelper<GenRandGenerator, FreeListMinSizeSelector> FreeListMinHelper;
                FreeListGenerationStrategy<GenRandGenerator, FreeListMinHelper> flgrStrategy( lc, genRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<GenRandGenerator, FreeListMinHelper>, FreeListMinHelper> flgrLevelGenerator( lc, flgrStrategy );
                flgrLevelGenerator.generateLevels();
                Level & flgrLevel( flgrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List GenRand Min Size" );
                saveLevelPpm( flgrLevel, "freelistgenrandmin.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "fl gr minsize rooms " << flgrLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning crand free list min size" );
                typedef FreeListThreadLocalHelper<CRandGenerator, FreeListMinSizeSelector> FreeListMinHelper;
                FreeListGenerationStrategy<CRandGenerator, FreeListMinHelper> flcrStrategy( lc, cRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<CRandGenerator, FreeListMinHelper>, FreeListMinHelper> flcrLevelGenerator( lc, flcrStrategy );
                flcrLevelGenerator.generateLevels();
                Level & flcrLevel( flcrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List CRand Min Size" );
                saveLevelPpm( flcrLevel, "freelistcrandmin.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "fl cr minsize rooms " << flcrLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning genrand free list max size" );
                typedef FreeListThreadLocalHelper<GenRandGenerator, FreeListMaxSizeSelector> FreeListMaxHelper;
                FreeListGenerationStrategy<GenRandGenerator, FreeListMaxHelper> flgrStrategy( lc, genRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<GenRandGenerator, FreeListMaxHelper>, FreeListMaxHelper> flgrLevelGenerator( lc, flgrStrategy );
                flgrLevelGenerator.generateLevels();
                Level & flgrLevel( flgrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List GenRand Max Size" );
                saveLevelPpm( flgrLevel, "freelistgenrandmax.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "fl gr maxsize rooms " << flgrLevel.rooms.size() << std::endl;
            }

            {
                timer.markBoundary( "Beginning crand free list max size" );
                typedef FreeListThreadLocalHelper<CRandGenerator, FreeListMaxSizeSelector> FreeListMaxHelper;
                FreeListGenerationStrategy<CRandGenerator, FreeListMaxHelper> flcrStrategy( lc, cRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<CRandGenerator, FreeListMaxHelper>, FreeListMaxHelper> flcrLevelGenerator( lc, flcrStrategy );
                flcrLevelGenerator.generateLevels();
                Level & flcrLevel( flcrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List CRand Max Size" );
                saveLevelPpm( flcrLevel, "freelistcrandmax.ppm" );
                timer.markBoundary( "Save ppm" );
                log() << "fl cr maxsize rooms " << flcrLevel.rooms.size() << std::endl;
            }


            timer.logTimes( "Time taken " );
        }

        return 0;
    }
}
