/*
 * FreeListGenerator.hpp
 *
 *  Created on: Aug 26, 2013
 *      Author: dan
 */

#ifndef FREELISTGENERATOR_HPP_
#define FREELISTGENERATOR_HPP_

#include <algorithm>

#include "GeneratorBase.hpp"
#include "FreeEntryCache.hpp"
#include "OcclusionBuffer.hpp"

/* #define FL_DEBUG */

namespace level_generator
{

struct FreeListSelectorBase {
    static Log log;
};

struct FreeListMinSizeSelector : FreeListSelectorBase {
    std::pair<bool, FreeEntryCache::FreeEntryIter> findFreeEntryOfDimensions( FreeEntryCache & freeEntryCache,
            const vec2uint32 & dimensionsToFind ) {
#ifdef FL_DEBUG
        log() << "Asked to find " << dimensionsToFind << std::endl;
#endif
        // std::lower bound is binary search to lowest insertion point
        FreeEntryCache::XToYFreeEntryMap::iterator wIter =
                std::lower_bound( freeEntryCache.xToYFreeEntryMap.begin(),
                        freeEntryCache.xToYFreeEntryMap.end(),
                        dimensionsToFind.x,
                        freeEntryCache.xToYCompare ),
                end = freeEntryCache.xToYFreeEntryMap.end();

        if( wIter == freeEntryCache.xToYFreeEntryMap.end() ) {
#ifdef FL_DEBUG
            log() << "XtoY entry map is empty" << std::endl;
#endif
            return std::make_pair( false, FreeEntryCache::FreeEntryIter { freeEntryCache.xToYFreeEntryMap.end() } );
        }
        else {
#ifdef FL_DEBUG
            log() << "XtoY entry map found " << wIter->first << " width - will now search height" << std::endl;
#endif
            while( wIter != end ) {
                // Look for first matching
                FreeEntryCache::YToFreeEntryMap::iterator hIter =
                        std::lower_bound( wIter->second.begin(),
                                wIter->second.end(),
                                dimensionsToFind.y,
                                freeEntryCache.yToEntryListCompare ),
                        end = wIter->second.end();

                while( hIter != end ) {
#ifdef FL_DEBUG
                    log() << "YToList found " << hIter->first << " height - checking list" << std::endl;
#endif
                    FreeEntryCache::EntryList & entryList = hIter->second;
                    if( entryList.size() > 0 ) {
                        vec4uint32 & front( entryList.front() );
#ifdef FL_DEBUG
                        log() << "List is non-zero with first entry: " << front << std::endl;
#endif
                        if( front.w >= dimensionsToFind.x && front.h >= dimensionsToFind.y ) {
                            return std::make_pair( true, FreeEntryCache::FreeEntryIter{ wIter, hIter, entryList.begin() } );
                        }
                    }
                    hIter++;
#ifdef FL_DEBUG
                    log() << "Going up the height" << std::endl;
#endif
                }
                wIter++;
#ifdef FL_DEBUG
                log() << "Going up the width" << std::endl;
#endif
            }
        }
        return std::make_pair( false, FreeEntryCache::FreeEntryIter { freeEntryCache.xToYFreeEntryMap.end() } );
    };
};

struct FreeListMaxSizeSelector : FreeListSelectorBase {
    std::pair<bool, FreeEntryCache::FreeEntryIter> findFreeEntryOfDimensions( FreeEntryCache & freeEntryCache,
            const vec2uint32 & dimensionsToFind ) {

#ifdef FL_DEBUG
        log() << "Asked to find " << dimensionsToFind << std::endl;
#endif
        // Starting from the largest walk backwards till we hit something too small and return
        // the largest we found during the walk.
        bool foundOne( false );
        vec2uint32 currentLargestSize { 0, 0 };
        FreeEntryCache::FreeEntryIter currentLargest;

        FreeEntryCache::XToYFreeEntryMap::iterator wIter = freeEntryCache.xToYFreeEntryMap.end();
        if( wIter == freeEntryCache.xToYFreeEntryMap.begin() ) {
#ifdef FL_DEBUG
            log() << "No entries found at all." << std::endl;
#endif
            // No entries
            currentLargest.xToYIter = wIter;
            return std::make_pair( foundOne, currentLargest );
        }

        while( wIter != freeEntryCache.xToYFreeEntryMap.begin() ) {
            wIter--;
            const uint32_t & w( wIter->first );
#ifdef FL_DEBUG
            log() << "Examining entries with width " << w << std::endl;
#endif

            if( w < dimensionsToFind.x ) {
#ifdef FL_DEBUG
                    log() << "Hit dimensions smaller than we are interested in. Leaving loop." << std::endl;
#endif
                goto DONE;
            }
            FreeEntryCache::YToFreeEntryMap::iterator hIter = wIter->second.end();

            while( hIter != wIter->second.begin() ) {
                hIter--;
                const uint32_t & h( hIter->first );
#ifdef FL_DEBUG
                log() << "Examining entries with height " << h << std::endl;
#endif

                if( h < dimensionsToFind.y ) {
#ifdef FL_DEBUG
                    log() << "Hit height dimension smaller than we are interested in. Skipping down to next width." << std::endl;
#endif
                    break;
                }

                if( (w > currentLargestSize.x && h >= currentLargestSize.y ) ||
                        (w >= currentLargestSize.x && h > currentLargestSize.y ) ) {
#ifdef FL_DEBUG
                    if( hIter->second.size() == 0 ) {
                        log() << "ERROR - Shouldn't find empty free element lists!" << std::endl;
                    }
                    else {
                        log() << "Using entry list begin() here." << std::endl;
#endif
                        FreeEntryCache::EntryList & el( hIter->second );
                        currentLargest.xToYIter = wIter;
                        currentLargest.yIter = hIter;
                        currentLargest.entryListIter = el.begin();

                        currentLargestSize.x = w;
                        currentLargestSize.y = h;

                        foundOne = true;
#ifdef FL_DEBUG
                    }
#endif
                }
#ifdef FL_DEBUG
                log() << "Going around height" << std::endl;
#endif
            }
#ifdef FL_DEBUG
            log() << "Going around width" << std::endl;
#endif
        }

DONE:

#ifdef FL_DEBUG
        log() << "All done - " << (foundOne ? "found one" : "not found" ) << std::endl;
#endif

        return std::make_pair( foundOne, currentLargest );
    };
};

template <typename RandGenerator, typename FreeEntrySelector>
class FreeListThreadLocalHelper : public GenerationThreadLocalHelperBase {
    const LevelGeneratorConfiguration configuration_;
    RandGenerator randGenerator_;
    FreeEntryCache freeEntryCache_;
    FreeEntrySelector freeEntrySelector_;
    OcclusionBuffer occlusionBuffer_;

public:
    FreeListThreadLocalHelper( const LevelGeneratorConfiguration & configuration,
            RandGenerator randGenerator ) :
        configuration_( configuration ),
        randGenerator_( randGenerator ),
        freeEntryCache_( configuration ),
        occlusionBuffer_( configuration_.levelDimension ) {
    }

    void resetFreeEntryCache() {
#ifdef FL_DEBUG
        log() << "Resetting free entry cache" << std::endl;
#endif

        // Clear the allocation cache and set one large free entry the size of the level
        freeEntryCache_.clearToDimensions( vec4uint32 { 0, 0, configuration_.levelDimension.x, configuration_.levelDimension.y } );

        occlusionBuffer_.clear();
        // And mark the borders "occluded"
        // The X borders
        occlusionBuffer_.occlude( vec4uint32 { 0, 0, configuration_.levelDimension.x, 1 } );
        occlusionBuffer_.occlude( vec4uint32 { 0, configuration_.levelDimensionMinusOne.y, configuration_.levelDimension.x, 1 } );
        // The Y borders
        occlusionBuffer_.occlude( vec4uint32 { 0, 0, 1, configuration_.levelDimension.y } );
        occlusionBuffer_.occlude( vec4uint32 { configuration_.levelDimensionMinusOne.x, 0, 1, configuration_.levelDimension.y } );
    }

    bool computeFreeLists( Level & level ) {
#ifdef FL_DEBUG
        log() << "Computing free lists" << std::endl;
#endif
        // Clear the allocation cache and re-populate using flood fill
        freeEntryCache_.repopulateFloodFill( occlusionBuffer_ );
        return freeEntryCache_.numFreeEntries;
    }

    bool insertRoom( Level & level, vec4uint32 & newRoom, uint32_t & seed ) {
#ifdef FL_DEBUG
        log() << "Attempting room insert of " << newRoom << std::endl;
#endif

        vec2uint32 expandedRoomDimensions { newRoom.w + 2, newRoom.h + 2 };
        std::pair<bool, FreeEntryCache::FreeEntryIter> ce = freeEntrySelector_.findFreeEntryOfDimensions( freeEntryCache_,
                expandedRoomDimensions  );

        if( ce.first ) {
            FreeEntryCache::FreeEntryIter & fe( ce.second );
            const vec4uint32 & foundSegment( *(fe.entryListIter) );
#ifdef FL_DEBUG
            log() << "Found free entry: " << foundSegment << std::endl;
#endif

            // Choose a random x and y offset inside this segment based on how wide and high it is
            int32_t restrictedX( foundSegment.w - (expandedRoomDimensions.x ) );
            int32_t restrictedY( foundSegment.h - (expandedRoomDimensions.y ) );
#ifdef FL_DEBUG
            log() << "Restricted X/Y is " << restrictedX << " " << restrictedY << std::endl;
#endif
            uint32_t xRand( randGenerator_( seed ) );
            uint32_t yRand( randGenerator_( seed ) );
            uint32_t randXOffset( ( restrictedX > 0 ? ( xRand % restrictedX) : 0 ) );
            uint32_t randYOffset( ( restrictedY > 0 ? ( yRand % restrictedY) : 0 ) );
#ifdef FL_DEBUG
            log() << "Randoffsets are " << randXOffset << " " << randYOffset << std::endl;
#endif
            newRoom.x = foundSegment.x + randXOffset + 1;
            newRoom.y = foundSegment.y + randYOffset + 1;
#ifdef FL_DEBUG
            log() << "Room X and Y are " << newRoom.x << " " << newRoom.y << std::endl;
#endif

            freeEntryCache_.useFreeEntry( fe, newRoom );
            vec4uint32 expandedRoom { newRoom.x - 1, newRoom.y - 1, newRoom.w + 2, newRoom.h + 2 };
            // We make the free space computations easier if we occlude an expanded room
            occlusionBuffer_.occlude( expandedRoom );
            return true;
        }
        else {
#ifdef FL_DEBUG
            log() << "Unable to find free entry" << std::endl;
#endif
            return false;
        }
    }

    bool hasFreeSpace() {
#ifdef FL_DEBUG
        log() << "Checking if free entry cache has free space:" << std::endl;
        freeEntryCache_.debugFreeEntries();
#endif
        return freeEntryCache_.numFreeEntries > 0;
    }

    bool computeFreeLists() {
#ifdef FL_DEBUG
        log() << "Asking free entry cache to repopulate free list using flood fill and occlusion buffer" << std::endl;
#endif
        freeEntryCache_.repopulateFloodFill( occlusionBuffer_ );
        return hasFreeSpace();
    }

    void debugFreeLists() {
        occlusionBuffer_.debug();
        freeEntryCache_.debugFreeEntries();
    }
};

template <typename RandGenerator, typename FreeListHelper>
class FreeListGenerationStrategy : public GenerationStrategyBase {
    const LevelGeneratorConfiguration configuration_;
    RandGenerator randGenerator_;

    vec4uint32 makePositionlessRoom_( uint32_t & seed ) {
        uint32_t w = ( (randGenerator_( seed ) % (configuration_.roomSizeVariance)) + configuration_.minroomsize);
        uint32_t h = ( (randGenerator_( seed ) % (configuration_.roomSizeVariance)) + configuration_.minroomsize );
        vec4uint32 newRoom { 0, 0, w, h };
#ifdef FL_DEBUG
        log() << "Made room " << newRoom << std::endl;
#endif
        return newRoom;
    }

public:
    FreeListGenerationStrategy( const LevelGeneratorConfiguration & configuration,
            RandGenerator randGenerator ) :
            configuration_( configuration ),
            randGenerator_( randGenerator ) {
    }

    void fillLevel( FreeListHelper & helper, Level & level, uint32_t & seed ) {
        level.rooms.reserve( configuration_.numRooms );
#ifdef FL_DEBUG
        log() << "Attempting to fill level." << std::endl;
#endif
        helper.resetFreeEntryCache();

        bool spaceAvailable( true );
        uint32_t roomsCreated( 0 );
        uint32_t attempts( 0 );

        while( spaceAvailable ) {
            while( spaceAvailable ) {
                vec4uint32 newRoom = makePositionlessRoom_( seed );
                if( helper.insertRoom( level, newRoom, seed ) ) {
                    roomsCreated++;
                    level.rooms.push_back( newRoom );
                    if( roomsCreated >= configuration_.numRooms ) {
#ifdef FL_DEBUG
                        log() << "Number of rooms satisfied" << std::endl;
#endif
                        goto DONE;
                    }
                }
                else {
                    attempts++;
                    if( attempts >= configuration_.maxRoomAttempts ) {
#ifdef FL_DEBUG
                        log() << "Exceeded number of room creation attempts" << std::endl;
#endif
                        goto DONE;
                    }
                }
                spaceAvailable = helper.hasFreeSpace();
            }
            // Exhausted current free lists, compact free space using flood fill
            spaceAvailable = helper.computeFreeLists();
        }
#ifdef FL_DEBUG
        log() << "All possible space taken." << std::endl;
#endif
DONE:
#ifdef FL_DEBUG
        helper.debugFreeLists();
#endif
        level.fillTiles();
    }

    FreeListHelper newThreadLocalHelper() {
        return FreeListHelper( configuration_, randGenerator_ );
    }

};

} /* namespace level_generator */
#endif /* FREELISTGENERATOR_HPP_ */
