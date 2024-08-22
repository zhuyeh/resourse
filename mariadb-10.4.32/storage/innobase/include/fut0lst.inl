/*****************************************************************************

Copyright (c) 1995, 2014, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2019, MariaDB Corporation.

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
@file include/fut0lst.ic
File-based list utilities

Created 11/28/1995 Heikki Tuuri
***********************************************************************/

#include "buf0buf.h"

/********************************************************************//**
Writes a file address. */
UNIV_INLINE
void
flst_write_addr(
/*============*/
	fil_faddr_t*	faddr,	/*!< in: pointer to file faddress */
	fil_addr_t	addr,	/*!< in: file address */
	mtr_t*		mtr)	/*!< in: mini-transaction handle */
{
	ut_ad(faddr && mtr);
	ut_ad(mtr_memo_contains_page_flagged(mtr, faddr,
					     MTR_MEMO_PAGE_X_FIX
					     | MTR_MEMO_PAGE_SX_FIX));
	ut_a(addr.page == FIL_NULL || addr.boffset >= FIL_PAGE_DATA);
	ut_a(ut_align_offset(faddr, srv_page_size) >= FIL_PAGE_DATA);

	mlog_write_ulint(faddr + FIL_ADDR_PAGE, addr.page, MLOG_4BYTES, mtr);
	mlog_write_ulint(faddr + FIL_ADDR_BYTE, addr.boffset,
			 MLOG_2BYTES, mtr);
}

/********************************************************************//**
Reads a file address.
@return file address */
UNIV_INLINE
fil_addr_t
flst_read_addr(
/*===========*/
	const fil_faddr_t*	faddr,	/*!< in: pointer to file faddress */
	mtr_t*			mtr)	/*!< in: mini-transaction handle */
{
	fil_addr_t	addr;

	ut_ad(faddr && mtr);

	addr.page = mach_read_from_4(faddr + FIL_ADDR_PAGE);
	addr.boffset = mach_read_from_2(faddr + FIL_ADDR_BYTE);
	ut_a(addr.page == FIL_NULL || addr.boffset >= FIL_PAGE_DATA);
	ut_a(ut_align_offset(faddr, srv_page_size) >= FIL_PAGE_DATA);
	return(addr);
}

/********************************************************************//**
Initializes a list base node. */
UNIV_INLINE
void
flst_init(
/*======*/
	flst_base_node_t*	base,	/*!< in: pointer to base node */
	mtr_t*			mtr)	/*!< in: mini-transaction handle */
{
	ut_ad(mtr_memo_contains_page_flagged(mtr, base,
					     MTR_MEMO_PAGE_X_FIX
					     | MTR_MEMO_PAGE_SX_FIX));

	if (mach_read_from_4(base + FLST_LEN)) {
		mlog_write_ulint(base + FLST_LEN, 0, MLOG_4BYTES, mtr);
	}
	flst_zero_addr(base + FLST_FIRST, mtr);
	flst_zero_addr(base + FLST_LAST, mtr);
}

/** Get the length of a list.
@param[in]	base	base node
@return length */
UNIV_INLINE
uint32_t
flst_get_len(
	const flst_base_node_t*	base)
{
	return(mach_read_from_4(base + FLST_LEN));
}

/********************************************************************//**
Gets list first node address.
@return file address */
UNIV_INLINE
fil_addr_t
flst_get_first(
/*===========*/
	const flst_base_node_t*	base,	/*!< in: pointer to base node */
	mtr_t*			mtr)	/*!< in: mini-transaction handle */
{
	return(flst_read_addr(base + FLST_FIRST, mtr));
}

/********************************************************************//**
Gets list last node address.
@return file address */
UNIV_INLINE
fil_addr_t
flst_get_last(
/*==========*/
	const flst_base_node_t*	base,	/*!< in: pointer to base node */
	mtr_t*			mtr)	/*!< in: mini-transaction handle */
{
	return(flst_read_addr(base + FLST_LAST, mtr));
}

/********************************************************************//**
Gets list next node address.
@return file address */
UNIV_INLINE
fil_addr_t
flst_get_next_addr(
/*===============*/
	const flst_node_t*	node,	/*!< in: pointer to node */
	mtr_t*			mtr)	/*!< in: mini-transaction handle */
{
	return(flst_read_addr(node + FLST_NEXT, mtr));
}

/********************************************************************//**
Gets list prev node address.
@return file address */
UNIV_INLINE
fil_addr_t
flst_get_prev_addr(
/*===============*/
	const flst_node_t*	node,	/*!< in: pointer to node */
	mtr_t*			mtr)	/*!< in: mini-transaction handle */
{
	return(flst_read_addr(node + FLST_PREV, mtr));
}
