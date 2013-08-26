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

#include "GeneratorBase.hpp"
#include "OcclusionBuffer.hpp"

namespace level_generator
{

class FreeEntryCache {
    const LevelGeneratorConfiguration & configuration_;

public:
    // Entry cache _should_ be relatively sparse
    typedef std::list<vec4uint32> EntryList;
    typedef std::map<uint64_t, EntryList> FusedXYToFreeEntryMap;

    struct FreeEntryIter {
        FusedXYToFreeEntryMap::iterator fusedMapIter;
        EntryList::iterator entryListIter;
    };

    std::vector<std::vector<std::list<vec4uint32> > > freeEntryLookupTable;
    uint32_t numFreeEntries;

    FreeEntryCache( const LevelGeneratorConfiguration & configuration );

    void clearToDimensions( const vec2uint32 & dimensions );

    void repopulateFloodFill( const OcclusionBuffer & occlussionBuffer );

    void useFreeEntry( FreeEntryIter & fe, const vec4uint32 & useDimensions );
};

} /* namespace level_generator */
#endif /* FREEENTRYCACHE_HPP_ */
