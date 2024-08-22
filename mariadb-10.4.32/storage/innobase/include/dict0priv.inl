/*****************************************************************************

Copyright (c) 2010, 2014, Oracle and/or its affiliates. All Rights Reserved.

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

/******************************************************************//**
@file include/dict0priv.ic
Data dictionary system private include file

Created  Wed 13 Oct 2010 16:10:14 EST Sunny Bains
***********************************************************************/

#include "dict0dict.h"
#include "dict0load.h"

/**********************************************************************//**
Gets a table; loads it to the dictionary cache if necessary. A low-level
function.
@return table, NULL if not found */
UNIV_INLINE
dict_table_t*
dict_table_get_low(
/*===============*/
	const char*	table_name)	/*!< in: table name */
{
	dict_table_t*	table;

	ut_ad(table_name);
	ut_ad(mutex_own(&dict_sys.mutex));

	table = dict_table_check_if_in_cache_low(table_name);

	if (table && table->corrupted) {
		ib::error	error;
		error << "Table " << table->name << "is corrupted";
		if (srv_load_corrupted) {
			error << ", but innodb_force_load_corrupted is set";
		} else {
			return(NULL);
		}
	}

	if (table == NULL) {
		table = dict_load_table(table_name, DICT_ERR_IGNORE_NONE);
	}

	ut_ad(!table || table->cached);

	return(table);
}

/**********************************************************************//**
Checks if a table is in the dictionary cache.
@return table, NULL if not found */
UNIV_INLINE
dict_table_t*
dict_table_check_if_in_cache_low(
/*=============================*/
	const char*	table_name)	/*!< in: table name */
{
	dict_table_t*	table;
	ulint		table_fold;

	DBUG_ENTER("dict_table_check_if_in_cache_low");
	DBUG_PRINT("dict_table_check_if_in_cache_low",
		   ("table: '%s'", table_name));

	ut_ad(table_name);
	ut_ad(mutex_own(&dict_sys.mutex));

	/* Look for the table name in the hash table */
	table_fold = ut_fold_string(table_name);

	HASH_SEARCH(name_hash, dict_sys.table_hash, table_fold,
		    dict_table_t*, table, ut_ad(table->cached),
		    !strcmp(table->name.m_name, table_name));
	DBUG_RETURN(table);
}
