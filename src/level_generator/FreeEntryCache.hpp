/*
 * FreeEntryCache.hpp
 *
 *  Created on: Aug 26, 2013
 *      Author: dan
 */

#ifndef FREEENTRYCACHE_HPP_
#define FREEENTRYCACHE_HPP_

#include <vector>
#include <list>
#include <map>

#include "GeneratorBase.hpp"
#include "OcclusionBuffer.hpp"
#include "Utils.hpp"

namespace level_generator
{

class FreeEntryCache {
    const LevelGeneratorConfiguration & configuration_;
    static Log log;

    void insertFreeEntry_( const vec4uint32 & newFreeEntry );
    void floodFillRegion_( std::vector<uint8_t> & currentlyMarked, vec4uint32 & regionToFlood );

public:
    // Entry cache _should_ be relatively sparse
    typedef std::list<vec4uint32> EntryList;
    struct YToEntryListCompare {
        bool operator()( std::pair< const uint32_t, EntryList > & a,
                const uint32_t & b ) {
            return a.first < b;
        }
        bool operator()( const uint32_t & a, const uint32_t & b ) {
            return a < b;
        }
    };

    typedef std::map<uint32_t, EntryList, YToEntryListCompare> YToFreeEntryMap;

    struct XToYCompare {
        bool operator()( std::pair<const uint32_t, YToFreeEntryMap> & a,
                const uint32_t & b ) {
            return a.first < b;
        }

        bool operator()( const uint32_t & a, const uint32_t & b ) {
            return a < b;
        }
    };

    typedef std::map<uint32_t, YToFreeEntryMap, XToYCompare> XToYFreeEntryMap;

    struct FreeEntryIter {
        XToYFreeEntryMap::iterator xToYIter;
        YToFreeEntryMap::iterator yIter;
        EntryList::iterator entryListIter;
    };

    XToYFreeEntryMap xToYFreeEntryMap;
    XToYCompare xToYCompare;
    YToEntryListCompare yToEntryListCompare;
    uint32_t numFreeEntries;

    FreeEntryCache( const LevelGeneratorConfiguration & configuration );

    void clearToDimensions( const vec4uint32 & freeArea );

    void repopulateFloodFill( const OcclusionBuffer & occlussionBuffer );

    void useFreeEntry( FreeEntryIter & fe, const vec4uint32 & useDimensions );

    void debugFreeEntries();
};

} /* namespace level_generator */
#endif /* FREEENTRYCACHE_HPP_ */
