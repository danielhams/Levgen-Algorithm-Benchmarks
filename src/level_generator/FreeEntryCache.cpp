/*
 * FreeEntryCache.cpp
 *
 *  Created on: Aug 26, 2013
 *      Author: dan
 */

#include <sstream>

#include "FreeEntryCache.hpp"

/* #define FEC_DEBUG */

namespace level_generator
{

using std::endl;
using std::vector;
using std::stringstream;

Log FreeEntryCache::log("FreeEntryCache");

FreeEntryCache::FreeEntryCache( const LevelGeneratorConfiguration & configuration ) :
        numFreeEntries( 0 ),
        configuration_( configuration ) {
};

void FreeEntryCache::insertFreeEntry_( const vec4uint32 & newFreeEntry ) {

    if( newFreeEntry.w < configuration_.minroomsizeplustwo || newFreeEntry.h < configuration_.minroomsizeplustwo ) {
#ifdef FEC_DEBUG
        log() << "Attempted to insert free entry too small: " << newFreeEntry << std::endl;
#endif
        return;
    }
#ifdef FEC_DEBUG
    log() << "Inserting free entry: " << newFreeEntry << endl;
#endif
    // std::lower bound is binary search to lowest insertion point
    FreeEntryCache::XToYFreeEntryMap::iterator wIter =
            std::lower_bound( xToYFreeEntryMap.begin(),
                    xToYFreeEntryMap.end(),
                    newFreeEntry.w,
                    xToYCompare ),
            end = xToYFreeEntryMap.end();

    EntryList * entryListToInsertInto( nullptr );

    if( wIter == xToYFreeEntryMap.end() || wIter->first != newFreeEntry.w ) {
#ifdef FEC_DEBUG
        log() << "XtoY entry map is empty for width " << newFreeEntry.w << std::endl;
#endif
        std::pair<XToYFreeEntryMap::iterator, bool> ins = xToYFreeEntryMap.emplace( newFreeEntry.w, YToFreeEntryMap() );
        YToFreeEntryMap & yToEntryMap = (*(ins.first)).second;
        std::pair<YToFreeEntryMap::iterator, bool> yins = yToEntryMap.emplace( newFreeEntry.h, EntryList() );
        EntryList & entryList = (*(yins.first)).second;
        entryListToInsertInto = &entryList;
    }
    else {
#ifdef FEC_DEBUG
        log() << "XtoY entry map found " << wIter->first << " width - will now search height" << std::endl;
#endif

        FreeEntryCache::YToFreeEntryMap::iterator hIter =
                std::lower_bound( wIter->second.begin(),
                        wIter->second.end(),
                        newFreeEntry.h,
                        yToEntryListCompare ),
                        end = wIter->second.end();

        if( hIter == wIter->second.end() || hIter->first != newFreeEntry.h ) {
#ifdef FEC_DEBUG
            log() << "Missing height map entry." << endl;
#endif
            EntryList newEntryList;
            std::pair<YToFreeEntryMap::iterator, bool> ins = wIter->second.emplace( newFreeEntry.h, newEntryList );
            entryListToInsertInto = &(*ins.first).second;
        }
        else {
#ifdef FEC_DEBUG
            log() << "Found height map entry." << endl;
#endif
            entryListToInsertInto = &hIter->second;
        }
    }

#ifdef FEC_DEBUG
    log() << "Inserting into entry list." << endl;
#endif
    entryListToInsertInto->push_back( newFreeEntry );
    numFreeEntries++;
}

void FreeEntryCache::floodFillRegion_( vector<uint8_t> & currentlyMarked, vec4uint32 & floodRegion ) {
    bool advancingX( true );
    bool advancingY( true );
    floodRegion.w = 1;
    floodRegion.h = 1;

    while( advancingX || advancingY ) {
        if( advancingX ) {
            uint32_t curX( floodRegion.x + floodRegion.w );
            if( curX < configuration_.levelDimension.x ) {
                for( uint32_t testY = floodRegion.y ; testY < (floodRegion.y + floodRegion.h) ; ++testY ) {
#ifdef FEC_DEBUG
                    log() << "Advancing X checking " << curX << " " << testY << endl;
#endif
                    if( currentlyMarked[ (configuration_.levelDimension.x * testY) + curX ] == 1 ) {
#ifdef FEC_DEBUG
                        log() << "Advancing X hit filled point " << endl;
#endif
                        advancingX = false;
                        break;
                    }
                }
            }
            else {
#ifdef FEC_DEBUG
                log() << "X oversteps level dimension " << endl;
#endif
                advancingX = false;
                floodRegion.w--;
            }

            if( advancingX ) {
                floodRegion.w++;
            }
        }

        if( advancingY ) {
            uint32_t curY( floodRegion.y + floodRegion.h );
            if( curY < configuration_.levelDimension.y ) {
                for( uint32_t testX = floodRegion.x ; testX < (floodRegion.x + floodRegion.w) ; ++testX ) {
#ifdef FEC_DEBUG
                    log() << "Advancing Y checking " << testX << " " << curY << endl;
#endif
                    if( currentlyMarked[ (configuration_.levelDimension.x * curY) + testX ] == 1 ) {
#ifdef FEC_DEBUG
                        log() << "Advancing Y hit filled point " << endl;
#endif
                        advancingY = false;
                        break;
                    }
                }
            }
            else {
#ifdef FEC_DEBUG
                log() << "Y oversteps level dimension " << endl;
#endif
                advancingY = false;
                floodRegion.h--;
            }

            if( advancingY ) {
                floodRegion.h++;
            }
        }
#ifdef FEC_DEBUG
        log() << "After one pass, region has become " << floodRegion << endl;
#endif
    }

#ifdef FEC_DEBUG
    log() << "After flood fill, region is " << floodRegion << endl;
#endif

}

void FreeEntryCache::clearToDimensions( const vec4uint32 & freeArea ) {
#ifdef FEC_DEBUG
    log() << "Clearing to dimensions" << endl;
#endif
    xToYFreeEntryMap.clear();
    numFreeEntries = 0;
    insertFreeEntry_( freeArea );
};

void printFloodBuffer( Log & log, vec2uint32 dimensions, vector<uint8_t> & buffer ) {
    stringstream ss( stringstream::out );
    for( uint32_t y = 0 ; y < dimensions.y ; ++y ) {
        for( uint32_t x = 0 ; x < dimensions.x ; ++x ) {
            uint8_t & val( buffer[ (dimensions.x * y) + x ] );
            ss << (val == 0 ? 0 : 1 );
        }
        ss << endl;
    }
    log() << endl << ss.str() << endl;
}

void FreeEntryCache::repopulateFloodFill( OcclusionBuffer & occlussionBuffer ) {
    // Take a copy of the occlusion buffer so we can mark which regions we've extracted as free
    // and skip checking them in future
    vector<uint8_t> currentlyMarked( occlussionBuffer.buffer );

#ifdef FEC_DEBUG
    log() << "Asked to repopulate using flood fill. BEFORE FLOOD FILL" << std::endl;
    printFloodBuffer( log, configuration_.levelDimension, currentlyMarked );
#endif

    // we skip the borders anyway
    for( uint32_t y = 1 ; y < configuration_.levelDimension.y - 1 ; ++y ) {
        for( uint32_t x = 1 ; x < configuration_.levelDimension.x - 1 ; ++x ) {
            uint32_t index( (configuration_.levelDimension.x * y) + x );
#ifdef FEC_DEBUG
            log() << "Examing pixel " << x << " " << y << " at index " << index << endl;
#endif
            if( currentlyMarked[ index ] == 0 ) {
#ifdef FEC_DEBUG
                log() << "Is free, will attempt a flood fill" << endl;
#endif
                vec4uint32 filledRegion { x, y, 1, 1 };
                floodFillRegion_( currentlyMarked, filledRegion );

                if( filledRegion.w >= configuration_.minroomsize && filledRegion.h >= configuration_.minroomsize ) {
#ifdef FEC_DEBUG
                    log() << "Flood fill discovers big enough region - will use as free segment" << endl;
#endif
                    // Expand it by one in each direction
                    filledRegion.x--;
                    filledRegion.y--;
                    filledRegion.w += 2;
                    filledRegion.h += 2;

                    for( uint32_t my = filledRegion.y ; my < filledRegion.y + filledRegion.h ; ++my ) {
                        for( uint32_t mx = filledRegion.x ; mx < filledRegion.x + filledRegion.w ; ++mx ) {
                            currentlyMarked[ (configuration_.levelDimension.x * my) + mx ] = 1;
                        }
                    }

                    insertFreeEntry_( filledRegion );
#ifdef FEC_DEBUG
                    log() << "After finding free region buffer is:" << endl;
                    printFloodBuffer( log, configuration_.levelDimension, currentlyMarked );
#endif
                }
                else {
                    // If the region found is too thin or too short check if it's unconnected
                    // If it is,
#ifdef FEC_DEBUG
                    log() << "Region not large enough. Checking if unconnected" << endl;
#endif
                    bool connected( false );
                    // Check the X borders
                    for( uint32_t xmo = filledRegion.x - 1 ; xmo < (filledRegion.x + filledRegion.w + 1) ; ++xmo ) {
#ifdef FEC_DEBUG
                        log() << "Examing X borders at " << xmo << ",(" << (filledRegion.y - 1) << "/" << (filledRegion.y + filledRegion.h) << ")" << std::endl;
#endif
                        if( currentlyMarked[ (configuration_.levelDimension.x * (filledRegion.y - 1) ) + xmo ] == 0 ||
                                currentlyMarked[ (configuration_.levelDimension.x * (filledRegion.y + filledRegion.h )) + xmo ] == 0 ) {
#ifdef FEC_DEBUG
                            log() << "One of the X borders has empty" << std::endl;
#endif
                            connected = true;
                            goto CONNECTIVITY_DONE;
                        }
                    }
                    // Check the Y borders
                    for( uint32_t ymo = filledRegion.y ; ymo < (filledRegion.y + filledRegion.h ) ; ++ymo ) {
#ifdef FEC_DEBUG
                        log() << "Examing Y borders at (" << (filledRegion.x - 1) << "/" << (filledRegion.x + filledRegion.w ) << ")," << ymo << std::endl;
#endif
                        if( currentlyMarked[ (configuration_.levelDimension.x * ymo ) + filledRegion.x - 1 ] == 0 ||
                                currentlyMarked[ (configuration_.levelDimension.x * ymo ) + (filledRegion.x + filledRegion.w) ] == 0 ) {
#ifdef FEC_DEBUG
                            log() << "One of the Y borders has empty" << std::endl;
#endif
                            connected = true;
                            goto CONNECTIVITY_DONE;
                        }
                    }
CONNECTIVITY_DONE:
                    if( !connected ) {
#ifdef FEC_DEBUG
                        log() << "Region " << filledRegion << " is not connected and too small. Will marked checked" << std::endl;
#endif
                        for( uint32_t my = filledRegion.y ; my < filledRegion.y + filledRegion.h ; ++my ) {
                            for( uint32_t mx = filledRegion.x ; mx < filledRegion.x + filledRegion.w ; ++mx ) {
                                currentlyMarked[ (configuration_.levelDimension.x * my) + mx ] = 1;
                            }
                        }
                        occlussionBuffer.occlude( filledRegion );
#ifdef FEC_DEBUG
                        printFloodBuffer( log, configuration_.levelDimension, currentlyMarked );
#endif
                    }
                    else {
#ifdef FEC_DEBUG
                        log() << "Region is still connected to free area. Leaving." << std::endl;
#endif
                    }
                }
            }
        }
    }
#ifdef FEC_DEBUG
    log() << "AFTER FLOOD FILL" << std::endl;
    printFloodBuffer( log, configuration_.levelDimension, currentlyMarked );
#endif
};

void FreeEntryCache::useFreeEntry( FreeEntryIter & fe, const vec4uint32 & spaceUsed ) {
    vec4uint32 freeEntry( *(fe.entryListIter) );

    // Now erase
    (fe.yIter)->second.erase( fe.entryListIter );
    numFreeEntries--;
#ifdef FEC_DEBUG
    log() << "Erased list entry" << std::endl;
#endif

    // If the list is now empty, remove the yToFreeEntry
    if( (fe.yIter)->second.size() == 0 ) {
#ifdef FEC_DEBUG
        log() << "The YToEntry list is empty. Will erase" << endl;
#endif
        fe.xToYIter->second.erase( fe.yIter );

        if( (fe.xToYIter->second.size() == 0 ) ) {
#ifdef FEC_DEBUG
            log() << "The XToYEntry list is empty. Will erase" << endl;
#endif
            xToYFreeEntryMap.erase( fe.xToYIter );
        }
    }

    // Finally, decompose into new free entries
    // and insert into our free lists.
#ifdef FEC_DEBUG
    log() << "Decomposing " << freeEntry << " with " << spaceUsed << endl;
#endif

    // Compute the four edge block sizes (not six, we'll try and preserve big spaces as much as we can)
    // Like this:
    // ^       **
    // |   ##BB__
    // y       ++
    //     x ----->

    // First the #
    int32_t hashWidth( spaceUsed.x - freeEntry.x );
    int32_t plusHeight( spaceUsed.y - freeEntry.y );
    int32_t starHeight( freeEntry.h - (plusHeight + spaceUsed.h ) );
    int32_t usWidth( freeEntry.w - (hashWidth + spaceUsed.w) );

    // Favour a split that doesn't leave very slim regions
    // By looking at the original free entry dimensions
    bool favourSplitOnHeight( freeEntry.h > freeEntry.w );

    /**/
    // Alternate between high and wide merges
    if( favourSplitOnHeight ) {
        if( plusHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x, freeEntry.y, freeEntry.w, (uint32_t)plusHeight };
#ifdef FEC_DEBUG
            log() << "IFE W1 full width plus" << std::endl;
#endif
            insertFreeEntry_( hash );
        }

        if( starHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x, freeEntry.y + plusHeight + spaceUsed.h, freeEntry.w, (uint32_t)starHeight };
#ifdef FEC_DEBUG
            log() << "IFE W2 full width star" << std::endl;
#endif
            insertFreeEntry_( hash );
        }

        if( hashWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x, freeEntry.y + plusHeight, (uint32_t)hashWidth, spaceUsed.h };
#ifdef FEC_DEBUG
            log() << "IFE W3 small hash" << std::endl;
#endif
            insertFreeEntry_( hash );
        }
        if( usWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth + spaceUsed.w, freeEntry.y + plusHeight, (uint32_t)usWidth, spaceUsed.h };
#ifdef FEC_DEBUG
            log() << "IFE W4 small underscore" << std::endl;
#endif
            insertFreeEntry_( hash );
        }
    }
    else {
        if( hashWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x, freeEntry.y, (uint32_t)hashWidth, freeEntry.h };
#ifdef FEC_DEBUG
            log() << "IFE H1 full height hash" << std::endl;
#endif
            insertFreeEntry_( hash );
        }

        if( usWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth + spaceUsed.w, freeEntry.y, (uint32_t)usWidth, freeEntry.h };
#ifdef FEC_DEBUG
            log() << "IFE H2 full height underscore" << std::endl;
#endif
            insertFreeEntry_( hash );
        }

        if( plusHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth, freeEntry.y, spaceUsed.w, (uint32_t)plusHeight };
#ifdef FEC_DEBUG
            log() << "IFE H3 small plus" << std::endl;
#endif
            insertFreeEntry_( hash );
        }
        if( starHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth, freeEntry.y + plusHeight + spaceUsed.h, spaceUsed.w, (uint32_t)starHeight };
#ifdef FEC_DEBUG
            log() << "IFE H4 small star" << std::endl;
#endif
            insertFreeEntry_( hash );
        }
    }

#ifdef FEC_DEBUG
    log() << "After use of free entry, cache contains:" << endl;
    debugFreeEntries();
#endif
};

void FreeEntryCache::debugFreeEntries() {
    for( XToYFreeEntryMap::iterator wIter = xToYFreeEntryMap.begin(),
            wend = xToYFreeEntryMap.end() ; wIter != wend ; ++wIter ) {

        YToFreeEntryMap & yMap( wIter->second );

        for( YToFreeEntryMap::iterator hIter = yMap.begin(), hend = yMap.end() ; hIter != hend ; ++hIter ) {

            EntryList & entryList( hIter->second );

            for( EntryList::iterator eIter = entryList.begin(), eEnd = entryList.end() ; eIter != eEnd ; ++eIter ) {
                log() << "Have free entry " << *eIter << endl;
            }
        }
    }
    log() << "This makes " << numFreeEntries << " free entries" << endl;
}


} /* namespace level_generator */
