/*****************************************************************************

Copyright (c) 2006, 2016, Oracle and/or its affiliates. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/buf0buddy.ic
Binary buddy allocator for compressed pages

Created December 2006 by Marko Makela
*******************************************************/

#include "buf0buf.h"
#include "buf0buddy.h"

/**********************************************************************//**
Allocate a block.  The thread calling this function must hold
buf_pool->mutex and must not hold buf_pool->zip_mutex or any block->mutex.
The buf_pool_mutex may be released and reacquired.
@return allocated block, never NULL */
void*
buf_buddy_alloc_low(
/*================*/
	buf_pool_t*	buf_pool,	/*!< in/out: buffer pool instance */
	ulint		i,		/*!< in: index of buf_pool->zip_free[],
					or BUF_BUDDY_SIZES */
	bool*		lru)		/*!< in: pointer to a variable that
					will be assigned true if storage was
					allocated from the LRU list and
					buf_pool->mutex was temporarily
					released */
	MY_ATTRIBUTE((malloc, nonnull));

/**********************************************************************//**
Deallocate a block. */
void
buf_buddy_free_low(
/*===============*/
	buf_pool_t*	buf_pool,	/*!< in: buffer pool instance */
	void*		buf,		/*!< in: block to be freed, must not be
					pointed to by the buffer pool */
	ulint		i)		/*!< in: index of buf_pool->zip_free[],
					or BUF_BUDDY_SIZES */
	MY_ATTRIBUTE((nonnull));

/**********************************************************************//**
Get the index of buf_pool->zip_free[] for a given block size.
@return index of buf_pool->zip_free[], or BUF_BUDDY_SIZES */
UNIV_INLINE
ulint
buf_buddy_get_slot(
/*===============*/
	ulint	size)	/*!< in: block size */
{
	ulint	i;
	ulint	s;

	ut_ad(size >= UNIV_ZIP_SIZE_MIN);

	for (i = 0, s = BUF_BUDDY_LOW; s < size; i++, s <<= 1) {
	}

	ut_ad(i <= BUF_BUDDY_SIZES);
	return(i);
}

/**********************************************************************//**
Allocate a block.  The thread calling this function must hold
buf_pool->mutex and must not hold buf_pool->zip_mutex or any
block->mutex.  The buf_pool->mutex may be released and reacquired.
This function should only be used for allocating compressed page frames.
@return allocated block, never NULL */
UNIV_INLINE
byte*
buf_buddy_alloc(
/*============*/
	buf_pool_t*	buf_pool,	/*!< in/out: buffer pool in which
					the page resides */
	ulint		size,		/*!< in: compressed page size
					(between UNIV_ZIP_SIZE_MIN and
					srv_page_size) */
	bool*		lru)		/*!< in: pointer to a variable
					that will be assigned true if
				       	storage was allocated from the
				       	LRU list and buf_pool->mutex was
				       	temporarily released */
{
	ut_ad(buf_pool_mutex_own(buf_pool));
	ut_ad(ut_is_2pow(size));
	ut_ad(size >= UNIV_ZIP_SIZE_MIN);
	ut_ad(size <= srv_page_size);

	return((byte*) buf_buddy_alloc_low(buf_pool, buf_buddy_get_slot(size),
					   lru));
}

/**********************************************************************//**
Deallocate a block. */
UNIV_INLINE
void
buf_buddy_free(
/*===========*/
	buf_pool_t*	buf_pool,	/*!< in/out: buffer pool in which
					the block resides */
	void*		buf,		/*!< in: block to be freed, must not
					be pointed to by the buffer pool */
	ulint		size)		/*!< in: block size,
					up to srv_page_size */
{
	ut_ad(buf_pool_mutex_own(buf_pool));
	ut_ad(ut_is_2pow(size));
	ut_ad(size >= UNIV_ZIP_SIZE_MIN);
	ut_ad(size <= srv_page_size);

	buf_buddy_free_low(buf_pool, buf, buf_buddy_get_slot(size));
}
