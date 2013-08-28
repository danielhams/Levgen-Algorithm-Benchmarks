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

#ifndef RANDGENERATORS_HPP_
#define RANDGENERATORS_HPP_

#include <cstdint>
#include <random>

namespace level_generator
{

struct CRandGenerator
{
    uint32_t operator()( uint32_t & gen ) {
        return rand_r( & gen );
    }
};

struct XorRandGenerator
{
    uint32_t operator()( uint32_t & gen ) {
        gen += gen;
        gen ^= 1;
        int32_t tgen=gen;
        if ( tgen < 0) {
            gen ^= 0x88888eef;
        }
        return gen;
    }
};

} /* namespace level_generator */
#endif /* RANDGENERATORS_HPP_ */
