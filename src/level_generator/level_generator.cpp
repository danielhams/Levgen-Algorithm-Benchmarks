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
using level_generator::XorRandGenerator;

using level_generator::NumRoomsMetric;
using level_generator::MinSpaceMetric;
using level_generator::Level;
using level_generator::LevelGeneratorConfiguration;
using level_generator::LevelGenerator;

using level_generator::OrigBruteForceGenerationStrategy;
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

NumRoomsMetric maxRoomsMetric;
MinSpaceMetric minSpaceMetric;

void logLevelStats( Log & log, const std::string & preamble, const Level & l ) {
    log() << preamble << " numRooms(" << l.rooms.size() << ") spaceCount(" << minSpaceMetric.countSpaceForLevel( l ) << ")" << std::endl;
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
        ("maxrooms", po::value<int>(), "Maximum number of rooms per level (1<=N<=200000)")
        ("maxroomattempts", po::value<int>(), "Maximum number of attempts to create a room per level (1<=N<=1000000)")
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
            if( minroomsize < 1 || minroomsize > 1000 )
            {
                showUsageSummary = true;
            }
            maxroomsize = vm["bigroom"].as<int>();
            if( maxroomsize < 1 || maxroomsize > 1000 )
            {
                showUsageSummary = true;
            }
            numLevels = vm["levels"].as<int>();
            if( numLevels < 1 || numLevels > 5000 )
            {
                showUsageSummary = true;
            }
            numRooms = vm["maxrooms"].as<int>();
            if( numRooms < 1 || numRooms > 200000 )
            {
                showUsageSummary = true;
            }
            maxRoomAttempts = vm["maxroomattempts"].as<int>();
            if( maxRoomAttempts < 1 || maxRoomAttempts > 1000000 )
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

        auto & roomsMetric( maxRoomsMetric );
//        auto & roomsMetric( minSpaceMetric );

        CRandGenerator cRandGenerator;
        XorRandGenerator xorRandGenerator;

        bool focusOnFreeList( false );

        for( uint32_t i = 0 ; i < 1 ; ++i ) {

            Timer timer;
            timer.markBoundary( "Begin" );

            /**/
            {
                timer.markBoundary( "Begin brute force simple xor" );
                BruteForceGenerationStrategy<XorRandGenerator, SimpleCollisionThreadLocalHelper> bruteForceXorGenerationStrategy( lc, xorRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<XorRandGenerator, SimpleCollisionThreadLocalHelper>, SimpleCollisionThreadLocalHelper>
                    bfxLevelGenerator( lc, bruteForceXorGenerationStrategy );
                bfxLevelGenerator.generateLevels();
                Level & bfxLevel( bfxLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce SimpleCT Xor" );
                saveLevelPpm( bfxLevel, "bruteforcesimplectxor.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "bf sc xor", bfxLevel );
            }

            {
                timer.markBoundary( "Beginning brute force simple crand" );
                BruteForceGenerationStrategy<CRandGenerator, SimpleCollisionThreadLocalHelper> bruteForceCrandGenerationStrategy( lc, cRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<CRandGenerator, SimpleCollisionThreadLocalHelper>, SimpleCollisionThreadLocalHelper > bfcLevelGenerator( lc, bruteForceCrandGenerationStrategy );
                bfcLevelGenerator.generateLevels();
                Level & bfcLevel( bfcLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce SimpleCT CRand" );
                saveLevelPpm( bfcLevel, "bruteforcesimplectcrand.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "bf sc cr", bfcLevel );
            }

            {
                timer.markBoundary( "Beginning brute force quadtree xor" );
                BruteForceGenerationStrategy<XorRandGenerator, FixedLevelQuadTreeThreadLocalHelper> bruteForceQuadTreeXorStrategy( lc, xorRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<XorRandGenerator, FixedLevelQuadTreeThreadLocalHelper>, FixedLevelQuadTreeThreadLocalHelper>
                    bfqtxLevelGenerator( lc, bruteForceQuadTreeXorStrategy );
                bfqtxLevelGenerator.generateLevels();
                Level & bfqtxLevel( bfqtxLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce QuadTree Xor" );
                saveLevelPpm( bfqtxLevel, "bruteforcefixedquadtreexor.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "bf qt xor", bfqtxLevel );
            }

            {
                timer.markBoundary( "Beginning brute force quadtree crand" );
                BruteForceGenerationStrategy<CRandGenerator, FixedLevelQuadTreeThreadLocalHelper> bruteForceQuadTreeCrandStrategy( lc, cRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<CRandGenerator, FixedLevelQuadTreeThreadLocalHelper>, FixedLevelQuadTreeThreadLocalHelper>
                    bfqtcLevelGenerator( lc, bruteForceQuadTreeCrandStrategy );
                bfqtcLevelGenerator.generateLevels();
                Level & bfqtcLevel( bfqtcLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce QuadTree CRand" );
                saveLevelPpm( bfqtcLevel, "bruteforcefixedquadtreecrand.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "bf qt cr", bfqtcLevel );
            }

            {
                timer.markBoundary( "Beginning brute force occlusion buffer xor" );
                BruteForceGenerationStrategy<XorRandGenerator, OcclusionThreadLocalHelper> bruteForceOcclusionBufferXorStrategy( lc, xorRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<XorRandGenerator, OcclusionThreadLocalHelper>, OcclusionThreadLocalHelper>
                    bfobxLevelGenerator( lc, bruteForceOcclusionBufferXorStrategy );
                bfobxLevelGenerator.generateLevels();
                Level & bfobxLevel( bfobxLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce Occlusion Buffer Xor" );
                saveLevelPpm( bfobxLevel, "bruteforceocclusionbufferxor.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "bf ob xor", bfobxLevel );
            }
            /**/

            {
                timer.markBoundary( "Beginning brute force occlusion buffer crand" );
                BruteForceGenerationStrategy<CRandGenerator, OcclusionThreadLocalHelper> bruteForceOcclusionBufferCrandStrategy( lc, cRandGenerator );
                LevelGenerator<BruteForceGenerationStrategy<CRandGenerator, OcclusionThreadLocalHelper>, OcclusionThreadLocalHelper>
                    bfobcLevelGenerator( lc, bruteForceOcclusionBufferCrandStrategy );
                bfobcLevelGenerator.generateLevels();
                Level & bfobcLevel( bfobcLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post BruteForce Occlusion Buffer CRand" );
                saveLevelPpm( bfobcLevel, "bruteforceocclusionbuffercrand.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "bf ob cr", bfobcLevel );
            }

            {
                timer.markBoundary( "Begin original brute force occlusion buffer crand" );
                OrigBruteForceGenerationStrategy<CRandGenerator, OcclusionThreadLocalHelper> obruteForceCrandGenerationStrategy( lc, cRandGenerator );
                LevelGenerator<OrigBruteForceGenerationStrategy<CRandGenerator, OcclusionThreadLocalHelper>, OcclusionThreadLocalHelper>
                    obfcLevelGenerator( lc, obruteForceCrandGenerationStrategy );
                obfcLevelGenerator.generateLevels();
                Level & obfcLevel( obfcLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Orig BruteForce Occlusion Buffer CRand" );
                saveLevelPpm( obfcLevel, "origbruteforceocclusionbuffercrand.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "obf ob cr", obfcLevel );
            }

            {
                timer.markBoundary( "Beginning free list min size xor" );
                typedef FreeListThreadLocalHelper<XorRandGenerator, FreeListMinSizeSelector> FreeListMinHelper;
                FreeListGenerationStrategy<XorRandGenerator, FreeListMinHelper> flgrStrategy( lc, xorRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<XorRandGenerator, FreeListMinHelper>, FreeListMinHelper> flgrLevelGenerator( lc, flgrStrategy );
                flgrLevelGenerator.generateLevels();
                Level & flgrLevel( flgrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List Min Size Xor" );
                saveLevelPpm( flgrLevel, "freelistminxor.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "fl minsize xor", flgrLevel );
            }

            {
                timer.markBoundary( "Beginning free list min size crand" );
                typedef FreeListThreadLocalHelper<CRandGenerator, FreeListMinSizeSelector> FreeListMinHelper;
                FreeListGenerationStrategy<CRandGenerator, FreeListMinHelper> flcrStrategy( lc, cRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<CRandGenerator, FreeListMinHelper>, FreeListMinHelper> flcrLevelGenerator( lc, flcrStrategy );
                flcrLevelGenerator.generateLevels();
                Level & flcrLevel( flcrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List Min Size CRand" );
                saveLevelPpm( flcrLevel, "freelistmincrand.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "fl minsize crand", flcrLevel );
            }

            {
                timer.markBoundary( "Beginning free list max size xor" );
                typedef FreeListThreadLocalHelper<XorRandGenerator, FreeListMaxSizeSelector> FreeListMaxHelper;
                FreeListGenerationStrategy<XorRandGenerator, FreeListMaxHelper> flgrStrategy( lc, xorRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<XorRandGenerator, FreeListMaxHelper>, FreeListMaxHelper> flgrLevelGenerator( lc, flgrStrategy );
                flgrLevelGenerator.generateLevels();
                Level & flgrLevel( flgrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List Max Size Xor" );
                saveLevelPpm( flgrLevel, "freelistmaxxor.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "fl maxsize xor", flgrLevel );
            }
            /**/

            {
                timer.markBoundary( "Beginning free list max size crand" );
                typedef FreeListThreadLocalHelper<CRandGenerator, FreeListMaxSizeSelector> FreeListMaxHelper;
                FreeListGenerationStrategy<CRandGenerator, FreeListMaxHelper> flcrStrategy( lc, cRandGenerator );
                LevelGenerator<FreeListGenerationStrategy<CRandGenerator, FreeListMaxHelper>, FreeListMaxHelper> flcrLevelGenerator( lc, flcrStrategy );
                flcrLevelGenerator.generateLevels();
                Level & flcrLevel( flcrLevelGenerator.pickLevelByCriteria( roomsMetric ) );
                timer.markBoundary( "Post Free List Max Size CRand" );
                saveLevelPpm( flcrLevel, "freelistmaxcrand.ppm" );
                timer.markBoundary( "Save ppm" );
                logLevelStats( log, "fl maxsize crand", flcrLevel );
            }

            timer.logTimes( "Time taken " );
        }

        return 0;
    }
}
