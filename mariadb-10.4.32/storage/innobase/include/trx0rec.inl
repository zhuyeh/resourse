/*****************************************************************************

Copyright (c) 1996, 2014, Oracle and/or its affiliates. All Rights Reserved.

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
@file include/trx0rec.ic
Transaction undo log record

Created 3/26/1996 Heikki Tuuri
*******************************************************/

/**********************************************************************//**
Reads from an undo log record the record type.
@return record type */
UNIV_INLINE
ulint
trx_undo_rec_get_type(
/*==================*/
	const trx_undo_rec_t*	undo_rec)	/*!< in: undo log record */
{
	return(mach_read_from_1(undo_rec + 2) & (TRX_UNDO_CMPL_INFO_MULT - 1));
}

/**********************************************************************//**
Reads the undo log record number.
@return undo no */
UNIV_INLINE
undo_no_t
trx_undo_rec_get_undo_no(
/*=====================*/
	const trx_undo_rec_t*	undo_rec)	/*!< in: undo log record */
{
	const byte*	ptr;

	ptr = undo_rec + 3;

	return(mach_u64_read_much_compressed(ptr));
}

/***********************************************************************//**
Copies the undo record to the heap.
@return own: copy of undo log record */
UNIV_INLINE
trx_undo_rec_t*
trx_undo_rec_copy(
/*==============*/
	const trx_undo_rec_t*	undo_rec,	/*!< in: undo log record */
	mem_heap_t*		heap)		/*!< in: heap where copied */
{
	ulint		len;

	len = mach_read_from_2(undo_rec)
		- ut_align_offset(undo_rec, srv_page_size);
	ut_ad(len < srv_page_size);
	trx_undo_rec_t* rec = static_cast<trx_undo_rec_t*>(
		mem_heap_dup(heap, undo_rec, len));
	mach_write_to_2(rec, len);
	return rec;
}
