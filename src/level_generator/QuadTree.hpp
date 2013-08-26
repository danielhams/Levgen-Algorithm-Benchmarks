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

#ifndef QUADTREE_HPP_
#define QUADTREE_HPP_

#include <memory>
#include <functional>

#include "Utils.hpp"

#include "GeneratorBase.hpp"

/* #define QUADDEBUG */

namespace level_generator
{

struct AABB {
    vec4uint32 dimensions;
    uint32_t ex, ey, doublemidx, doublemidy;

    AABB();
    explicit AABB( vec4uint32 dimensions );

    bool intersects( const AABB & c ) const;
    bool intersects( const vec4uint32 & r ) const;
    inline bool intersects( const vec4uint32 * r ) const {
        return intersects( *r );
    }
};

inline std::ostream & operator<< ( std::ostream & out, const AABB & aabb ) {
    out << "(" << aabb.dimensions.x << "," << aabb.dimensions.y << "->" << aabb.ex << "," << aabb.ey <<
            " w/h[" << aabb.dimensions.w << "," << aabb.dimensions.h << "])";
    return out;
}

class QuadTreeNodeBase {
    static Log log;
};

template <typename Content>
class QuadTreeNode : QuadTreeNodeBase {
    AABB boundary_;
    uint32_t level_;

#ifdef QUADDEBUG
    std::string indent_;
#endif

    std::unique_ptr<QuadTreeNode> leftBottomNode_;
    std::unique_ptr<QuadTreeNode> rightBottomNode_;
    std::unique_ptr<QuadTreeNode> leftTopNode_;
    std::unique_ptr<QuadTreeNode> rightTopNode_;

    std::vector<Content*> nodeContent_;

public:
    QuadTreeNode( const uint32_t depthToCreateTo, const uint32_t level,
                  const vec4uint32 dimensions ) :
        level_( level ),
#ifdef QUADDEBUG
        indent_( level, '\t'),
#endif
        boundary_( dimensions ),
        leftBottomNode_(nullptr),
        rightBottomNode_(nullptr),
        leftTopNode_(nullptr),
        rightTopNode_(nullptr) {
            if( level < depthToCreateTo ) {
                partition( depthToCreateTo, level );
            }
    };
    QuadTreeNode( const QuadTreeNode & s ) :
        level_( s.level_),
#ifdef QUADDEBUG
        indent_( s.indent_ ),
#endif
        boundary_( s.boundary_ ) {
        if( s.leftBottomNode_.get() != nullptr ) {
            leftBottomNode_ = make_unique<QuadTreeNode>( *(s.leftBottomNode_) );
            rightBottomNode_ = make_unique<QuadTreeNode>( *(s.rightBottomNode_) );
            leftTopNode_ = make_unique<QuadTreeNode>( *(s.leftTopNode_) );
            rightTopNode_ = make_unique<QuadTreeNode>( *(s.rightTopNode_) );
        }
    };
    void operator=( const QuadTreeNode & s ) {
        this->level_ = s.level_;
#ifdef QUADDEBUG
        this->indent_ = s.indent_;
#endif
        this->boundary_ = s.boundary_;
        if( s.leftBottomNode_.get() != nullptr ) {
            this->leftBottomNode_ = make_unique<QuadTreeNode>( *(s.leftBottomNode_) );
            this->rightBottomNode_ = make_unique<QuadTreeNode>( *(s.rightBottomNode_) );
            this->leftTopNode_ = make_unique<QuadTreeNode>( *(s.leftTopNode_) );
            this->rightTopNode_ = make_unique<QuadTreeNode>( *(s.rightTopNode_) );
        }
    };
    void insertContent( Content * c ) {
#ifdef QUADDEBUG
        log() << indent_ << "Attempting to insert into node level " << level_ << " QuadTreeNode" << boundary_ << " " << r << std::endl;
#endif

        if( leftBottomNode_.get() != nullptr ) {
            // We are partitioned - check for intersection with child partitions
            if( leftBottomNode_->boundary_.intersects( c ) ) {
                leftBottomNode_->insertContent( c );
            }
            if( rightBottomNode_->boundary_.intersects( c ) ) {
                rightBottomNode_->insertContent( c );
            }
            if( leftTopNode_->boundary_.intersects( c ) ) {
                leftTopNode_->insertContent( c );
            }
            if( rightTopNode_->boundary_.intersects( c ) ) {
                rightTopNode_->insertContent( c );
            }
        }
        else {
            nodeContent_.push_back( c );
#ifdef QUADDEBUG
            log() << indent_ << "Appended content to node." << std::endl;
#endif
        }
    }

    void partition( const uint32_t depthToCreateTo, const uint32_t level ) {
        uint32_t leftWidth( boundary_.dimensions.w / 2 );
        uint32_t rightWidth( boundary_.dimensions.w - leftWidth );
        uint32_t bottomHeight( boundary_.dimensions.h / 2 );
        uint32_t topHeight( boundary_.dimensions.h - bottomHeight );
        uint32_t newLevel( level + 1 );

        uint32_t rightStartX( boundary_.ex - rightWidth );
        uint32_t topStartY( boundary_.ey - topHeight );

        leftBottomNode_ = make_unique<QuadTreeNode>( QuadTreeNode( depthToCreateTo,
                                                                   newLevel,
                                                                   vec4uint32 { boundary_.dimensions.x, boundary_.dimensions.y, leftWidth, bottomHeight } ) );

        rightBottomNode_ = make_unique<QuadTreeNode>( QuadTreeNode( depthToCreateTo,
                                                                    newLevel,
                                                                    vec4uint32 { rightStartX, boundary_.dimensions.y, rightWidth, bottomHeight } ) );

        leftTopNode_ = make_unique<QuadTreeNode>( QuadTreeNode( depthToCreateTo,
                                                                    newLevel,
                                                                    vec4uint32 { boundary_.dimensions.x, topStartY, leftWidth, topHeight } ) );

        rightTopNode_ = make_unique<QuadTreeNode>( QuadTreeNode( depthToCreateTo,
                                                                     newLevel,
                                                                     vec4uint32 { rightStartX, topStartY, rightWidth, topHeight } ) );
    }

    bool checkForCollision( const vec4uint32 & dim ) const {
#ifdef QUADDEBUG
        log() << indent_ << boundary_ << " checking for collision with " << dim << std::endl;
#endif
        if( level_ == 0 || boundary_.intersects( dim ) ) {
            if( leftBottomNode_.get() != nullptr ) {
#ifdef QUADDEBUG
                log() << indent_ << "Using the boundary tests" << std::endl;
#endif
                return ( leftBottomNode_->checkForCollision( dim )
                         ||
                         rightBottomNode_->checkForCollision( dim )
                         ||
                         leftTopNode_->checkForCollision( dim )
                         ||
                         rightTopNode_->checkForCollision( dim ) );
            }
            else if( nodeContent_.size() > 0 ) {
#ifdef QUADDEBUG
                log() << indent_ << "Node " << boundary_ << " doing rooms collision check: " << dim << std::endl;
#endif
                for( const Content * c : nodeContent_ ) {
#ifdef QUADDEBUG
                    log() << indent_ << "-- checking if room " << c << " intersects with " << dim << std::endl;
#endif
                    if( intersects( *c, dim ) ) {
#ifdef QUADDEBUG
                        log() << indent_ << "-- room collision between new room " << dim << " and existing room " << *dcr << std::endl;
#endif
                        return true;
                    }
                }
            }
        }

#ifdef QUADDEBUG
        log() << indent_ << "No collision" << std::endl;
#endif
        return false;
    }

    void clear() {
        if( leftBottomNode_.get() != nullptr ) {
            leftBottomNode_->clear();
            rightBottomNode_->clear();
            leftTopNode_->clear();
            rightTopNode_->clear();
        }
        else {
            nodeContent_.clear();
        }
    }

#ifdef QUADDEBUG
    void debug() {
        log() << indent_ << "Node with boundary " << boundary_ << " at level " << level_ << std::endl;
        if( leftBottomNode_.get() != nullptr ) {
            leftBottomNode_->debug();
            rightBottomNode_->debug();
            leftTopNode_->debug();
            rightTopNode_->debug();
        }
        else {
            for( vec4uint32 * r : nodeRooms_ ) {
                log() << indent_ << "\t" << *r << std::endl;
            }
        }
    }
#endif
};

class QuadTreeBase {
    static Log log;
};

template <typename Content>
class QuadTree : QuadTreeBase {
    uint32_t treeDepth_;
    QuadTreeNode<Content> rootNode_;

public:
    QuadTree( const uint32_t treeDepth, const uint32_t width, const uint32_t height ) :
        treeDepth_(treeDepth),
        rootNode_( treeDepth, 0, vec4uint32{ 0, 0, width, height } ) {
#ifdef QUADDEBUG
        log() << "QuadTree constructed." << std::endl;
#endif
    };

    void insertContent( Content * c ) {
#ifdef QUADDEBUG
        log() << "QUADTREE Insert of " << c << std::endl;
#endif
        rootNode_.insertContent( c );
#ifdef QUADDEBUG
        log() << "QUADTREE Completed insert of " << c << std::endl;
#endif
    };

    bool checkForCollision( const Content & c ) const {
#ifdef QUADDEBUG
        log() << "QuadTree checkForCollision called" << std::endl;
#endif
        return rootNode_.checkForCollision( c );
    }

    void clear() {
        rootNode_.clear();
    }

#ifdef QUADDEBUG
    void debug() {
        rootNode_.debug();
    }
#endif
};

} /* namespace level_generator */
#endif /* QUADTREE_HPP_ */
