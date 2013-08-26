/*
 * FreeListGenerator.hpp
 *
 *  Created on: Aug 26, 2013
 *      Author: dan
 */

#ifndef FREELISTGENERATOR_HPP_
#define FREELISTGENERATOR_HPP_

#include "GeneratorBase.hpp"
#include "FreeEntryCache.hpp"
#include "OcclusionBuffer.hpp"

namespace level_generator
{

struct FreeListMinSizeSelector {
    std::pair<bool, FreeEntryCache::FreeEntryIter> findFreeEntryOfDimensions( FreeEntryCache & freeEntryCache,
            vec4uint32 & dimensionsToFind ) {
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
        // Clear the allocation cache and set one large free entry the size of the level
        freeEntryCache_.clearToDimensions( configuration_.levelDimension );
        occlusionBuffer_.clear();
    }

    bool computeFreeLists( Level & level ) {
        // Clear the allocation cache and re-populate using flood fill
        freeEntryCache_.repopulateFloodFill( occlusionBuffer_ );
        return freeEntryCache_.numFreeEntries;
    }

    bool insertRoom( Level & level, vec4uint32 & newRoom, uint32_t & seed ) {
        std::pair<bool, FreeEntryCache::FreeEntryIter> ce = freeEntrySelector_.findFreeEntryOfDimensions( freeEntryCache_, newRoom );

        if( ce.first ) {
            FreeEntryCache::FreeEntryIter & fe( ce.second );
            const vec4uint32 & foundSegment( *(fe.entryListIter) );

            // Choose a random x and y offset inside this segment based on how wide and high it is
            int32_t restrictedX( foundSegment.w - (newRoom.w + 3 ) );
            int32_t restrictedY( foundSegment.h - (newRoom.h + 3 ) );
            uint32_t xRand( randGenerator_( seed ) );
            uint32_t yRand( randGenerator_( seed ) );
            newRoom.x = ( restrictedX > 0 ? ( xRand % restrictedX) + 1 : 1 );
            newRoom.y = ( restrictedY > 0 ? ( yRand % restrictedY) + 1 : 1 );

            freeEntryCache_.useFreeEntry( fe, newRoom );

            // We make the free space computations easier if we occlude an expanded room
            vec4uint32 expandedRoom { newRoom.x - 1, newRoom.y - 1, newRoom.w + 2, newRoom.h + 2 };
            occlusionBuffer_.occlude( expandedRoom );
            return true;
        }
        else {
            return false;
        }
    }

    bool hasFreeSpace() {
    }

    bool computeFreeLists() {
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

        helper.resetFreeEntryCache();

        bool spaceAvailable( true );
        uint32_t roomsCreated( 0 );
        uint32_t attempts( 0 );

        while( spaceAvailable ) {
            while( spaceAvailable ) {
                vec4uint32 newRoom = makePositionlessRoom_( seed );
                attempts++;
                if( helper.insertRoom( level, newRoom, seed ) ) {
                    roomsCreated++;
                    if( roomsCreated >= configuration_.numRooms || attempts >= configuration_.maxRoomAttempts ) {
                        goto DONE;
                    }
                }
                spaceAvailable = helper.hasFreeSpace();
            }
            // Exhausted current free lists, compact free space using flood fill
            spaceAvailable = helper.computeFreeLists();
        }
DONE:

        level.fillTiles();
    }

    FreeListHelper newThreadLocalHelper() {
        return FreeListHelper( configuration_, randGenerator_ );
    }

};

} /* namespace level_generator */
#endif /* FREELISTGENERATOR_HPP_ */
