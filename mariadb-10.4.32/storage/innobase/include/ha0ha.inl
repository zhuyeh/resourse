/*****************************************************************************

Copyright (c) 1994, 2015, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2018, MariaDB Corporation.

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

/********************************************************************//**
@file include/ha0ha.ic
The hash table with external chains

Created 8/18/1994 Heikki Tuuri
*************************************************************************/

#ifdef BTR_CUR_HASH_ADAPT
#include "ut0rnd.h"
#include "mem0mem.h"
#include "btr0types.h"

/******************************************************************//**
Gets a hash node data.
@return pointer to the data */
UNIV_INLINE
const rec_t*
ha_node_get_data(
/*=============*/
	const ha_node_t*	node)	/*!< in: hash chain node */
{
	return(node->data);
}

/******************************************************************//**
Sets hash node data. */
UNIV_INLINE
void
ha_node_set_data_func(
/*==================*/
	ha_node_t*	node,	/*!< in: hash chain node */
#if defined UNIV_AHI_DEBUG || defined UNIV_DEBUG
	buf_block_t*	block,	/*!< in: buffer block containing the data */
#endif /* UNIV_AHI_DEBUG || UNIV_DEBUG */
	const rec_t*	data)	/*!< in: pointer to the data */
{
#if defined UNIV_AHI_DEBUG || defined UNIV_DEBUG
	node->block = block;
#endif /* UNIV_AHI_DEBUG || UNIV_DEBUG */
	node->data = data;
}

#if defined UNIV_AHI_DEBUG || defined UNIV_DEBUG
/** Sets hash node data.
@param n in: hash chain node
@param b in: buffer block containing the data
@param d in: pointer to the data */
# define ha_node_set_data(n,b,d) ha_node_set_data_func(n,b,d)
#else /* UNIV_AHI_DEBUG || UNIV_DEBUG */
/** Sets hash node data.
@param n in: hash chain node
@param b in: buffer block containing the data
@param d in: pointer to the data */
# define ha_node_set_data(n,b,d) ha_node_set_data_func(n,d)
#endif /* UNIV_AHI_DEBUG || UNIV_DEBUG */

/******************************************************************//**
Gets the next node in a hash chain.
@return next node, NULL if none */
UNIV_INLINE
ha_node_t*
ha_chain_get_next(
/*==============*/
	const ha_node_t*	node)	/*!< in: hash chain node */
{
	return(node->next);
}

/******************************************************************//**
Gets the first node in a hash chain.
@return first node, NULL if none */
UNIV_INLINE
ha_node_t*
ha_chain_get_first(
/*===============*/
	hash_table_t*	table,	/*!< in: hash table */
	ulint		fold)	/*!< in: fold value determining the chain */
{
	return((ha_node_t*)
	       hash_get_nth_cell(table, hash_calc_hash(fold, table))->node);
}

#ifdef UNIV_DEBUG
/********************************************************************//**
Assert that the synchronization object in a hash operation involving
possible change in the hash table is held.
Note that in case of mutexes we assert that mutex is owned while in case
of rw-locks we assert that it is held in exclusive mode. */
UNIV_INLINE
void
hash_assert_can_modify(
/*===================*/
	hash_table_t*	table,	/*!< in: hash table */
	ulint		fold)	/*!< in: fold value */
{
	if (table->type == HASH_TABLE_SYNC_MUTEX) {
		ut_ad(mutex_own(hash_get_mutex(table, fold)));
	} else if (table->type == HASH_TABLE_SYNC_RW_LOCK) {
# ifdef UNIV_DEBUG
		rw_lock_t* lock = hash_get_lock(table, fold);
		ut_ad(rw_lock_own(lock, RW_LOCK_X));
# endif
	} else {
		ut_ad(table->type == HASH_TABLE_SYNC_NONE);
	}
}

/********************************************************************//**
Assert that the synchronization object in a hash search operation is held.
Note that in case of mutexes we assert that mutex is owned while in case
of rw-locks we assert that it is held either in x-mode or s-mode. */
UNIV_INLINE
void
hash_assert_can_search(
/*===================*/
	hash_table_t*	table,	/*!< in: hash table */
	ulint		fold)	/*!< in: fold value */
{
	if (table->type == HASH_TABLE_SYNC_MUTEX) {
		ut_ad(mutex_own(hash_get_mutex(table, fold)));
	} else if (table->type == HASH_TABLE_SYNC_RW_LOCK) {
		ut_ad(rw_lock_own_flagged(hash_get_lock(table, fold),
					  RW_LOCK_FLAG_X | RW_LOCK_FLAG_S));
	} else {
		ut_ad(table->type == HASH_TABLE_SYNC_NONE);
	}
}
#endif /* UNIV_DEBUG */

/*************************************************************//**
Looks for an element in a hash table.
@return pointer to the data of the first hash table node in chain
having the fold number, NULL if not found */
UNIV_INLINE
const rec_t*
ha_search_and_get_data(
/*===================*/
	hash_table_t*	table,	/*!< in: hash table */
	ulint		fold)	/*!< in: folded value of the searched data */
{
	hash_assert_can_search(table, fold);
	ut_ad(btr_search_enabled);

	for (const ha_node_t* node = ha_chain_get_first(table, fold);
	     node != NULL;
	     node = ha_chain_get_next(node)) {

		if (node->fold == fold) {

			return(node->data);
		}
	}

	return(NULL);
}

/*********************************************************//**
Looks for an element when we know the pointer to the data.
@return pointer to the hash table node, NULL if not found in the table */
UNIV_INLINE
ha_node_t*
ha_search_with_data(
/*================*/
	hash_table_t*	table,	/*!< in: hash table */
	ulint		fold,	/*!< in: folded value of the searched data */
	const rec_t*	data)	/*!< in: pointer to the data */
{
	ha_node_t*	node;

	hash_assert_can_search(table, fold);

	ut_ad(btr_search_enabled);

	node = ha_chain_get_first(table, fold);

	while (node) {
		if (node->data == data) {

			return(node);
		}

		node = ha_chain_get_next(node);
	}

	return(NULL);
}

/***********************************************************//**
Deletes a hash node. */
void
ha_delete_hash_node(
/*================*/
	hash_table_t*	table,		/*!< in: hash table */
	ha_node_t*	del_node);	/*!< in: node to be deleted */

/*********************************************************//**
Looks for an element when we know the pointer to the data, and deletes
it from the hash table, if found.
@return TRUE if found */
UNIV_INLINE
ibool
ha_search_and_delete_if_found(
/*==========================*/
	hash_table_t*	table,	/*!< in: hash table */
	ulint		fold,	/*!< in: folded value of the searched data */
	const rec_t*	data)	/*!< in: pointer to the data */
{
	ha_node_t*	node;

	hash_assert_can_modify(table, fold);
	ut_ad(btr_search_enabled);

	node = ha_search_with_data(table, fold, data);

	if (node) {
		ha_delete_hash_node(table, node);

		return(TRUE);
	}

	return(FALSE);
}
#endif /* BTR_CUR_HASH_ADAPT */
