/*
 * FreeEntryCache.cpp
 *
 *  Created on: Aug 26, 2013
 *      Author: dan
 */

#include "FreeEntryCache.hpp"

namespace level_generator
{

FreeEntryCache::FreeEntryCache( const LevelGeneratorConfiguration & configuration ) :
        numFreeEntries( 0 ),
        configuration_( configuration ) {
};

void FreeEntryCache::clearToDimensions( const vec2uint32 & dimensions ) {
};

void FreeEntryCache::repopulateFloodFill( const OcclusionBuffer & occlussionBuffer ) {
};

void FreeEntryCache::useFreeEntry( FreeEntryIter & fe, const vec4uint32 & useDimensions ) {
};


} /* namespace level_generator */
