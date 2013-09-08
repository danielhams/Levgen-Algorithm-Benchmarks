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
    bool previousSplitLeftHanded_;

    void insertFreeEntry_( const vec4uint32 & newFreeEntry );
    void floodFillRegion_( std::vector<char> & currentlyMarked, vec4uint32 & regionToFlood );
    bool floodFillRegionLargeEnough_( std::vector<char> & currentlyMarked, vec4uint32 & foundRegion );
    bool floodFillRegionConnected_( std::vector<char> & currentlyMarked, vec4uint32 & foundRegion );

public:
    // Entry cache _should_ be relatively sparse
    typedef std::list<vec4uint32> EntryList;
    struct YToEntryListCompare {
        bool operator()( std::pair< const uint32_t, EntryList > & a,
                const uint32_t & b ) const {
            return a.first < b;
        }
        bool operator()( const uint32_t & a, const uint32_t & b ) const {
            return a < b;
        }
    };

    typedef std::map<uint32_t, EntryList, YToEntryListCompare> YToFreeEntryMap;

    struct XToYCompare {
        bool operator()( std::pair<const uint32_t, YToFreeEntryMap> & a,
                const uint32_t & b ) const {
            return a.first < b;
        }

        bool operator()( const uint32_t & a, const uint32_t & b ) const {
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

    void clear();

//    void repopulateFloodFill( OcclusionBuffer & occlussionBuffer );
    void repopulateFloodFillNew( OcclusionBuffer & occlussionBuffer );

    void useFreeEntry( FreeEntryIter & fe, const vec4uint32 & useDimensions );

    void debugFreeEntries();
};

} /* namespace level_generator */
#endif /* FREEENTRYCACHE_HPP_ */
