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
        configuration_( configuration ),
        previousSplitLeftHanded_( true ) {
};

void FreeEntryCache::insertFreeEntry_( const vec4uint32 & newFreeEntry ) {

    if( newFreeEntry.w < configuration_.minroomsizeplustwo || newFreeEntry.h < configuration_.minroomsizeplustwo ) {
#ifdef FEC_DEBUG
        log() << "Attempted to insert free entry too small: " << newFreeEntry << endl;
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
        log() << "XtoY entry map is empty for width " << newFreeEntry.w << endl;
#endif
        std::pair<XToYFreeEntryMap::iterator, bool> ins = xToYFreeEntryMap.emplace( newFreeEntry.w, YToFreeEntryMap() );
        YToFreeEntryMap & yToEntryMap = (*(ins.first)).second;
        std::pair<YToFreeEntryMap::iterator, bool> yins = yToEntryMap.emplace( newFreeEntry.h, EntryList() );
        EntryList & entryList = (*(yins.first)).second;
        entryListToInsertInto = &entryList;
    }
    else {
#ifdef FEC_DEBUG
        log() << "XtoY entry map found " << wIter->first << " width - will now search height" << endl;
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
    log() << "Asked to repopulate using flood fill. BEFORE FLOOD FILL" << endl;
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
                        log() << "Examing X borders at " << xmo << ",(" << (filledRegion.y - 1) << "/" << (filledRegion.y + filledRegion.h) << ")" << endl;
#endif
                        if( currentlyMarked[ (configuration_.levelDimension.x * (filledRegion.y - 1) ) + xmo ] == 0 ||
                                currentlyMarked[ (configuration_.levelDimension.x * (filledRegion.y + filledRegion.h )) + xmo ] == 0 ) {
#ifdef FEC_DEBUG
                            log() << "One of the X borders has empty" << endl;
#endif
                            connected = true;
                            goto CONNECTIVITY_DONE;
                        }
                    }
                    // Check the Y borders
                    for( uint32_t ymo = filledRegion.y ; ymo < (filledRegion.y + filledRegion.h ) ; ++ymo ) {
#ifdef FEC_DEBUG
                        log() << "Examing Y borders at (" << (filledRegion.x - 1) << "/" << (filledRegion.x + filledRegion.w ) << ")," << ymo << endl;
#endif
                        if( currentlyMarked[ (configuration_.levelDimension.x * ymo ) + filledRegion.x - 1 ] == 0 ||
                                currentlyMarked[ (configuration_.levelDimension.x * ymo ) + (filledRegion.x + filledRegion.w) ] == 0 ) {
#ifdef FEC_DEBUG
                            log() << "One of the Y borders has empty" << endl;
#endif
                            connected = true;
                            goto CONNECTIVITY_DONE;
                        }
                    }
CONNECTIVITY_DONE:
                    if( !connected ) {
#ifdef FEC_DEBUG
                        log() << "Region " << filledRegion << " is not connected and too small. Will marked checked" << endl;
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
                        log() << "Region is still connected to free area. Leaving." << endl;
#endif
                    }
                }
            }
        }
    }
#ifdef FEC_DEBUG
    log() << "AFTER FLOOD FILL" << endl;
    printFloodBuffer( log, configuration_.levelDimension, currentlyMarked );
#endif
};

void FreeEntryCache::useFreeEntry( FreeEntryIter & fe, const vec4uint32 & spaceUsed ) {
    vec4uint32 freeEntry( *(fe.entryListIter) );

    // Now erase
    (fe.yIter)->second.erase( fe.entryListIter );
    numFreeEntries--;
#ifdef FEC_DEBUG
    log() << "Erased list entry" << endl;
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

    // Compute the four edge block sizes (not six, we'll try and preserve big spaces and
    // rotate between left or right handedness)
    // Like this:
    // ^     **
    // |   ##BB__
    // y     ++
    //     x ----->

    int32_t starHeight( spaceUsed.y - freeEntry.y );
    int32_t hashWidth( spaceUsed.x - freeEntry.x );
    int32_t usWidth( freeEntry.w - (hashWidth + spaceUsed.w) );
    int32_t plusHeight( freeEntry.h - (starHeight + spaceUsed.h) );
#ifdef FEC_DEBUG
    log() << "HW(" << hashWidth << ") PH(" << plusHeight << ") SH(" << starHeight << ") UW(" << usWidth << ")" << endl;
#endif

    if( previousSplitLeftHanded_ ) {
        // Go right handed (start extends to right etc)
        if( starHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth, freeEntry.y, freeEntry.w - hashWidth, (uint32_t)starHeight };
#ifdef FEC_DEBUG
            log() << "IFE RH ST" << endl;
#endif
            insertFreeEntry_( hash );
        }

        if( usWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth + spaceUsed.w, freeEntry.y + starHeight, (uint32_t)usWidth, spaceUsed.h + plusHeight };
#ifdef FEC_DEBUG
            log() << "IFE RH US" << endl;
#endif
            insertFreeEntry_( hash );
        }

        if( plusHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x,
                freeEntry.y + starHeight + spaceUsed.h,
                hashWidth + spaceUsed.w,
                (uint32_t)plusHeight };
#ifdef FEC_DEBUG
            log() << "IFE RH PL" << endl;
#endif
            insertFreeEntry_( hash );
        }

        if( hashWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x, freeEntry.y, (uint32_t)hashWidth, starHeight + spaceUsed.h };
#ifdef FEC_DEBUG
            log() << "IFE RH HS" << endl;
#endif
            insertFreeEntry_( hash );
        }
    }
    else {
        // Go left handed (start extends to left etc)
        if( starHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x, freeEntry.y, hashWidth + spaceUsed.w, (uint32_t)starHeight };
#ifdef FEC_DEBUG
            log() << "IFE LH ST" << endl;
#endif
            insertFreeEntry_( hash );
        }

        if( usWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth + spaceUsed.w, freeEntry.y, (uint32_t)usWidth, starHeight + spaceUsed.h };
#ifdef FEC_DEBUG
            log() << "IFE LH US" << endl;
#endif
            insertFreeEntry_( hash );
        }

        if( plusHeight > 0 ) {
            vec4uint32 hash{ freeEntry.x + hashWidth,
                freeEntry.y + starHeight + spaceUsed.h,
                spaceUsed.w + usWidth,
                (uint32_t)plusHeight };
#ifdef FEC_DEBUG
            log() << "IFE LH PL" << endl;
#endif
            insertFreeEntry_( hash );
        }

        if( hashWidth > 0 ) {
            vec4uint32 hash{ freeEntry.x, freeEntry.y + starHeight, (uint32_t)hashWidth, spaceUsed.h + plusHeight };
#ifdef FEC_DEBUG
            log() << "IFE LH HS" << endl;
#endif
            insertFreeEntry_( hash );
        }
    }

#ifdef FEC_DEBUG
    log() << "After use of free entry, cache contains:" << endl;
    debugFreeEntries();
#endif

    previousSplitLeftHanded_ = !previousSplitLeftHanded_;
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
