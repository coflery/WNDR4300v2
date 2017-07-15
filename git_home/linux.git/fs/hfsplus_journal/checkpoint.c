/*
 * linux/fs/checkpoint.c
 * 
 * Written by Stephen C. Tweedie <sct@redhat.com>, 1999
 *
 * Copyright 1999 Red Hat Software --- All Rights Reserved
 *
 * This file is part of the Linux kernel and is made available under
 * the terms of the GNU General Public License, version 2, or at your
 * option, any later version, incorporated herein by reference.
 *
 * Checkpoint routines for the generic filesystem journaling code.  
 * Part of the ext2fs journaling system.  
 *
 * Checkpointing is the process of ensuring that a section of the log is
 * committed fully to disk, so that that portion of the log can be
 * reused.
 */

#include <linux/time.h>
#include <linux/fs.h>
#include "hfsplus_jbd.h"
#include <linux/errno.h>
#include <linux/slab.h>
#include "hfsplus_fs.h"

/*
 * Unlink a buffer from a transaction checkpoint list.
 *
 * Called with j_list_lock held.
 */
static inline void __buffer_unlink_first(struct hfsplus_jbd_head *jh)
{
	hfsplus_transaction_t *transaction = jh->b_cp_transaction;

	jh->b_cpnext->b_cpprev = jh->b_cpprev;
	jh->b_cpprev->b_cpnext = jh->b_cpnext;
	if (transaction->t_checkpoint_list == jh) {
		transaction->t_checkpoint_list = jh->b_cpnext;
		if (transaction->t_checkpoint_list == jh)
			transaction->t_checkpoint_list = NULL;
	}
}

/*
 * Unlink a buffer from a transaction checkpoint(io) list.
 *
 * Called with j_list_lock held.
 */
static inline void __buffer_unlink(struct hfsplus_jbd_head *jh)
{
	hfsplus_transaction_t *transaction = jh->b_cp_transaction;

	__buffer_unlink_first(jh);
	if (transaction->t_checkpoint_io_list == jh) {
		transaction->t_checkpoint_io_list = jh->b_cpnext;
		if (transaction->t_checkpoint_io_list == jh)
			transaction->t_checkpoint_io_list = NULL;
	}
}

/*
 * Move a buffer from the checkpoint list to the checkpoint io list
 *
 * Called with j_list_lock held
 */
static inline void __buffer_relink_io(struct hfsplus_jbd_head *jh)
{
	hfsplus_transaction_t *transaction = jh->b_cp_transaction;

	__buffer_unlink_first(jh);

	if (!transaction->t_checkpoint_io_list) {
		jh->b_cpnext = jh->b_cpprev = jh;
	} else {
		jh->b_cpnext = transaction->t_checkpoint_io_list;
		jh->b_cpprev = transaction->t_checkpoint_io_list->b_cpprev;
		jh->b_cpprev->b_cpnext = jh;
		jh->b_cpnext->b_cpprev = jh;
	}
	transaction->t_checkpoint_io_list = jh;
}

/*
 * Try to release a checkpointed buffer from its transaction.
 * Returns 1 if we released it and 2 if we also released the
 * whole transaction.
 *
 * Requires j_list_lock
 * Called under hfsplus_jbd_lock_bh_state(hfsplus_jh2bh(jh)), and drops it
 */
static int __try_to_free_cp_buf(struct hfsplus_jbd_head *jh)
{
	int ret = 0;
	struct buffer_head *bh = hfsplus_jh2bh(jh);

	if (jh->b_jlist == HFSPLUS_BJ_None && !buffer_locked(bh) &&
	    !buffer_dirty(bh) && !buffer_write_io_error(bh)) {
		HFSPLUS_JBUFFER_TRACE(jh, "remove from checkpoint list");
		ret = __hfsplus_jbd_remove_checkpoint(jh) + 1;
		hfsplus_jbd_unlock_bh_state(bh);
		hfsplus_jbd_remove_journal_head(bh);
		HFSPLUS_BUFFER_TRACE(bh, "release");
		__brelse(bh);
	} else {
		hfsplus_jbd_unlock_bh_state(bh);
	}
	return ret;
}

/*
 * __hfsplus__log_wait_for_space: wait until there is space in the journal.
 *
 * Called under j-state_lock *only*.  It will be unlocked if we have to wait
 * for a checkpoint to free up some space in the log.
 */
void __hfsplus__log_wait_for_space(hfsplus_jbd_t *journal)
{
	int nblocks, space_left;
	assert_spin_locked(&journal->j_state_lock);

	nblocks = hfsplus_jbd_space_needed(journal);
	while (__hfsplus__log_space_left(journal) < nblocks) {
		if (journal->j_flags & JFS_ABORT)
			return;
		spin_unlock(&journal->j_state_lock);
		mutex_lock(&journal->j_checkpoint_mutex);

		/*
		 * Test again, another process may have checkpointed while we
		 * were waiting for the checkpoint lock. If there are no
		 * transactions ready to be checkpointed, try to recover
		 * journal space by calling cleanup_hfsplus_jbd_tail(), and if
		 * that doesn't work, by waiting for the currently committing
		 * transaction to complete.  If there is absolutely no way
		 * to make progress, this is either a BUG or corrupted
		 * filesystem, so abort the journal and leave a stack
		 * trace for forensic evidence.
		 */
		spin_lock(&journal->j_state_lock);
		spin_lock(&journal->j_list_lock);
		nblocks = hfsplus_jbd_space_needed(journal);
		space_left = __hfsplus__log_space_left(journal);
		if (space_left < nblocks) {
			int chkpt = journal->j_checkpoint_transactions != NULL;
			hfsplus_jbd_tid_t tid = 0;

			if (journal->j_committing_transaction)
				tid = journal->j_committing_transaction->t_tid;
			spin_unlock(&journal->j_list_lock);
			spin_unlock(&journal->j_state_lock);
			if (chkpt) {
				hfsplus_jbd_log_do_checkpoint(journal);
			} else if (cleanup_hfsplus_jbd_tail(journal) == 0) {
				/* We were able to recover space; yay! */
				;
			} else if (tid) {
				hfsplus_jbd_log_wait_commit(journal, tid);
			} else {
				printk(KERN_ERR "%s: needed %d blocks and "
				       "only had %d space available\n",
				       __func__, nblocks, space_left);
				printk(KERN_ERR "%s: no way to get more "
				       "journal space\n", __func__);
				WARN_ON(1);
				hfsplus_jbd_abort(journal, 0);
			}
			spin_lock(&journal->j_state_lock);
		} else {
			spin_unlock(&journal->j_list_lock);
		}
		mutex_unlock(&journal->j_checkpoint_mutex);
	}
}

/*
 * We were unable to perform hfsplus_jbd_trylock_bh_state() inside j_list_lock.
 * The caller must restart a list walk.  Wait for someone else to run
 * hfsplus_jbd_unlock_bh_state().
 */
static void jbd_sync_bh(hfsplus_jbd_t *journal, struct buffer_head *bh)
	__releases(journal->j_list_lock)
{
	get_bh(bh);
	spin_unlock(&journal->j_list_lock);
	hfsplus_jbd_lock_bh_state(bh);
	hfsplus_jbd_unlock_bh_state(bh);
	put_bh(bh);
}

/*
 * Clean up transaction's list of buffers submitted for io.
 * We wait for any pending IO to complete and remove any clean
 * buffers. Note that we take the buffers in the opposite ordering
 * from the one in which they were submitted for IO.
 *
 * Return 0 on success, and return <0 if some buffers have failed
 * to be written out.
 *
 * Called with j_list_lock held.
 */
static int __wait_cp_io(hfsplus_jbd_t *journal, hfsplus_transaction_t *transaction)
{
	struct hfsplus_jbd_head *jh;
	struct buffer_head *bh;
	hfsplus_jbd_tid_t this_tid;
	int released = 0;
	int ret = 0;

	this_tid = transaction->t_tid;
restart:
	/* Did somebody clean up the transaction in the meanwhile? */
	if (journal->j_checkpoint_transactions != transaction ||
			transaction->t_tid != this_tid)
		return ret;
	while (!released && transaction->t_checkpoint_io_list) {
		jh = transaction->t_checkpoint_io_list;
		bh = hfsplus_jh2bh(jh);
		if (!hfsplus_jbd_trylock_bh_state(bh)) {
			jbd_sync_bh(journal, bh);
			spin_lock(&journal->j_list_lock);
			goto restart;
		}
		if (buffer_locked(bh)) {
			atomic_inc(&bh->b_count);
			spin_unlock(&journal->j_list_lock);
			hfsplus_jbd_unlock_bh_state(bh);
			wait_on_buffer(bh);
			/* the hfsplus_jbd_head may have gone by now */
			HFSPLUS_BUFFER_TRACE(bh, "brelse");
			__brelse(bh);
			spin_lock(&journal->j_list_lock);
			goto restart;
		}
		if (unlikely(buffer_write_io_error(bh)))
			ret = -EIO;

		/*
		 * Now in whatever state the buffer currently is, we know that
		 * it has been written out and so we can drop it from the list
		 */
		released = __hfsplus_jbd_remove_checkpoint(jh);
		hfsplus_jbd_unlock_bh_state(bh);
		hfsplus_jbd_remove_journal_head(bh);
		__brelse(bh);
	}

	return ret;
}

#define NR_BATCH	64

static void
__flush_batch(hfsplus_jbd_t *journal, struct buffer_head **bhs, int *batch_count)
{
	int i;

	ll_rw_block(SWRITE, *batch_count, bhs);
	for (i = 0; i < *batch_count; i++) {
		struct buffer_head *bh = bhs[i];
		clear_buffer_hfsplus_jbd_jwrite(bh);
		HFSPLUS_BUFFER_TRACE(bh, "brelse");
		__brelse(bh);
	}
	*batch_count = 0;
}

/*
 * Try to flush one buffer from the checkpoint list to disk.
 *
 * Return 1 if something happened which requires us to abort the current
 * scan of the checkpoint list.  Return <0 if the buffer has failed to
 * be written out.
 *
 * Called with j_list_lock held and drops it if 1 is returned
 * Called under hfsplus_jbd_lock_bh_state(hfsplus_jh2bh(jh)), and drops it
 */
static int __process_buffer(hfsplus_jbd_t *journal, struct hfsplus_jbd_head *jh, struct buffer_head **bhs, int *batch_count)
{
	struct buffer_head *bh = hfsplus_jh2bh(jh);
	int ret = 0;

	if (buffer_locked(bh)) {
		atomic_inc(&bh->b_count);
		spin_unlock(&journal->j_list_lock);
		hfsplus_jbd_unlock_bh_state(bh);
		wait_on_buffer(bh);
		/* the journal_head may have gone by now */
		HFSPLUS_BUFFER_TRACE(bh, "brelse");
		__brelse(bh);
		ret = 1;
	} else if (jh->b_transaction != NULL) {
		hfsplus_transaction_t *t = jh->b_transaction;
		hfsplus_jbd_tid_t tid = t->t_tid;

		spin_unlock(&journal->j_list_lock);
		hfsplus_jbd_unlock_bh_state(bh);
		hfsplus_log_start_commit(journal, tid);
		hfsplus_jbd_log_wait_commit(journal, tid);
		ret = 1;
	} else if (!buffer_dirty(bh)) {
		ret = 1;
		if (unlikely(buffer_write_io_error(bh)))
			ret = -EIO;
		HFSPLUS_J_ASSERT_JH(jh, !buffer_hfsplus_jbddirty(bh));
		HFSPLUS_BUFFER_TRACE(bh, "remove from checkpoint");
		__hfsplus_jbd_remove_checkpoint(jh);
		spin_unlock(&journal->j_list_lock);
		hfsplus_jbd_unlock_bh_state(bh);
		hfsplus_jbd_remove_journal_head(bh);
		__brelse(bh);
	} else {
		/*
		 * Important: we are about to write the buffer, and
		 * possibly block, while still holding the journal lock.
		 * We cannot afford to let the transaction logic start
		 * messing around with this buffer before we write it to
		 * disk, as that would break recoverability.  
		 */
		HFSPLUS_BUFFER_TRACE(bh, "queue");
		get_bh(bh);
		HFSPLUS_J_ASSERT_BH(bh, !buffer_hfsplus_jbd_jwrite(bh));
		set_buffer_hfsplus_jbd_jwrite(bh);
		bhs[*batch_count] = bh;
		__buffer_relink_io(jh);
		hfsplus_jbd_unlock_bh_state(bh);
		(*batch_count)++;
		if (*batch_count == NR_BATCH) {
			spin_unlock(&journal->j_list_lock);
			__flush_batch(journal, bhs, batch_count);
			ret = 1;
		}
	}
	return ret;
}

/*
 * Perform an actual checkpoint. We take the first transaction on the
 * list of transactions to be checkpointed and send all its buffers
 * to disk. We submit larger chunks of data at once.
 *
 * The journal should be locked before calling this function.
 * Called with j_checkpoint_mutex held.
 */
int hfsplus_jbd_log_do_checkpoint(hfsplus_jbd_t *journal)
{
	hfsplus_transaction_t *transaction;
	hfsplus_jbd_tid_t this_tid;
	int result;

	dprint(DBG_JCHKPT, "Start checkpoint\n");

	/*
	 * First thing: if there are any transactions in the log which
	 * don't need checkpointing, just eliminate them from the
	 * journal straight away.  
	 */
	result = cleanup_hfsplus_jbd_tail(journal);
	dprint(DBG_JCHKPT, "cleanup_hfsplus_jbd_tail returned %d\n", result);
	if (result <= 0)
		return result;

	/*
	 * OK, we need to start writing disk blocks.  Take one transaction
	 * and write it.
	 */
	result = 0;
	spin_lock(&journal->j_list_lock);
	if (!journal->j_checkpoint_transactions)
		goto out;
	transaction = journal->j_checkpoint_transactions;
	this_tid = transaction->t_tid;
restart:
	/*
	 * If someone cleaned up this transaction while we slept, we're
	 * done (maybe it's a new transaction, but it fell at the same
	 * address).
	 */
	if (journal->j_checkpoint_transactions == transaction &&
			transaction->t_tid == this_tid) {
		int batch_count = 0;
		struct buffer_head *bhs[NR_BATCH];
		struct hfsplus_jbd_head *jh;
		int retry = 0, err;

		while (!retry && transaction->t_checkpoint_list) {
			struct buffer_head *bh;

			jh = transaction->t_checkpoint_list;
			bh = hfsplus_jh2bh(jh);
			if (!hfsplus_jbd_trylock_bh_state(bh)) {
				jbd_sync_bh(journal, bh);
				retry = 1;
				break;
			}
			retry = __process_buffer(journal, jh, bhs,&batch_count);
			if (retry < 0 && !result)
				result = retry;
			if (!retry && (need_resched() ||
				spin_needbreak(&journal->j_list_lock))) {
				spin_unlock(&journal->j_list_lock);
				retry = 1;
				break;
			}
		}

		if (batch_count) {
			if (!retry) {
				spin_unlock(&journal->j_list_lock);
				retry = 1;
			}
			__flush_batch(journal, bhs, &batch_count);
		}

		if (retry) {
			spin_lock(&journal->j_list_lock);
			goto restart;
		}
		/*
		 * Now we have cleaned up the first transaction's checkpoint
		 * list. Let's clean up the second one
		 */
		err = __wait_cp_io(journal, transaction);
		if (!result)
			result = err;
	}
out:
	spin_unlock(&journal->j_list_lock);
	if (result < 0)
		hfsplus_jbd_abort(journal, result);
	else
		result = cleanup_hfsplus_jbd_tail(journal);

	return (result < 0) ? result : 0;
}

/*
 * Check the list of checkpoint transactions for the journal to see if
 * we have already got rid of any since the last update of the log tail
 * in the journal superblock.  If so, we can instantly roll the
 * superblock forward to remove those transactions from the log.
 * 
 * Return <0 on error, 0 on success, 1 if there was nothing to clean up.
 * 
 * Called with the journal lock held.
 *
 * This is the only part of the journaling code which really needs to be
 * aware of transaction aborts.  Checkpointing involves writing to the
 * main filesystem area rather than to the journal, so it can proceed
 * even in abort state, but we must not update the journal superblock if
 * checkpointing may have failed.  Otherwise, we would lose some metadata
 * buffers which should be written-back to the filesystem.
 */

int cleanup_hfsplus_jbd_tail(hfsplus_jbd_t *journal)
{
	hfsplus_transaction_t * transaction;
	hfsplus_jbd_tid_t		first_tid;
	unsigned int	blocknr, freed;

	if (is_hfsplus_jbd_aborted(journal))
		return 1;

	/* OK, work out the oldest transaction remaining in the log, and
	 * the log block it starts at.
	 *
	 * If the log is now empty, we need to work out which is the
	 * next transaction ID we will write, and where it will
	 * start. */

	spin_lock(&journal->j_state_lock);
	spin_lock(&journal->j_list_lock);
	transaction = journal->j_checkpoint_transactions;
	if (transaction) {
		first_tid = transaction->t_tid;
		blocknr = transaction->t_log_start;
	} else if ((transaction = journal->j_committing_transaction) != NULL) {
		first_tid = transaction->t_tid;
		blocknr = transaction->t_log_start;
	} else if ((transaction = journal->j_running_transaction) != NULL) {
		first_tid = transaction->t_tid;
		blocknr = journal->j_head;
	} else {
		first_tid = journal->j_transaction_sequence;
		blocknr = journal->j_head;
	}
	spin_unlock(&journal->j_list_lock);
	HFSPLUS_J_ASSERT(blocknr != 0);

	/* If the oldest pinned transaction is at the tail of the log
           already then there's not much we can do right now. */
	if (journal->j_tail_sequence == first_tid) {
		spin_unlock(&journal->j_state_lock);
		return 1;
	}

	/* OK, update the superblock to recover the freed space.
	 * Physical blocks come first: have we wrapped beyond the end of
	 * the log?  */
	freed = blocknr - journal->j_tail;
	if (blocknr < journal->j_tail)
		freed = freed + journal->j_last - journal->j_first;

#ifdef HFSPLUS_JOURNAL_MAC_COMPATIBLE
	hfsplus_journal_header_start_update(journal, freed);
#endif
	dprint(DBG_JCHKPT,
		  "Cleaning journal tail from %d to %d (offset %u), "
		  "freeing %u\n",
		  journal->j_tail_sequence, first_tid, blocknr, freed);

	journal->j_free += freed;
	journal->j_tail_sequence = first_tid;
	journal->j_tail = blocknr;
	spin_unlock(&journal->j_state_lock);
	if (!(journal->j_flags & JFS_ABORT))
		hfsplus_jbd_update_superblock(journal, 1);
	return 0;
}


/* Checkpoint list management */

/*
 * hfsplus_jbd_clean_checkpoint_list
 *
 * Find all the written-back checkpoint buffers in the journal and release them.
 *
 * Called with the journal locked.
 * Called with j_list_lock held.
 * Returns number of bufers reaped (for debug)
 */

static int journal_clean_one_cp_list(struct hfsplus_jbd_head *jh, int *released)
{
	struct hfsplus_jbd_head *last_jh;
	struct hfsplus_jbd_head *next_jh = jh;
	int ret, freed = 0;

	*released = 0;
	if (!jh)
		return 0;

	last_jh = jh->b_cpprev;
	do {
		jh = next_jh;
		next_jh = jh->b_cpnext;
		/* Use trylock because of the ranking */
		if (hfsplus_jbd_trylock_bh_state(hfsplus_jh2bh(jh))) {
			ret = __try_to_free_cp_buf(jh);
			if (ret) {
				freed++;
				if (ret == 2) {
					*released = 1;
					return freed;
				}
			}
		}
		/*
		 * This function only frees up some memory
		 * if possible so we dont have an obligation
		 * to finish processing. Bail out if preemption
		 * requested:
		 */
		if (need_resched())
			return freed;
	} while (jh != last_jh);

	return freed;
}

/*
 * journal_clean_checkpoint_list
 *
 * Find all the written-back checkpoint buffers in the journal and release them.
 *
 * Called with the journal locked.
 * Called with j_list_lock held.
 * Returns number of buffers reaped (for debug)
 */

int __hfsplus_jbd_clean_checkpoint_list(hfsplus_jbd_t *journal)
{
	hfsplus_transaction_t *transaction, *last_transaction, *next_transaction;
	int ret = 0;
	int released;

	transaction = journal->j_checkpoint_transactions;
	if (!transaction)
		goto out;

	last_transaction = transaction->t_cpprev;
	next_transaction = transaction;
	do {
		transaction = next_transaction;
		next_transaction = transaction->t_cpnext;
		ret += journal_clean_one_cp_list(transaction->
				t_checkpoint_list, &released);
		/*
		 * This function only frees up some memory if possible so we
		 * dont have an obligation to finish processing. Bail out if
		 * preemption requested:
		 */
		if (need_resched())
			goto out;
		if (released)
			continue;
		/*
		 * It is essential that we are as careful as in the case of
		 * t_checkpoint_list with removing the buffer from the list as
		 * we can possibly see not yet submitted buffers on io_list
		 */
		ret += journal_clean_one_cp_list(transaction->
				t_checkpoint_io_list, &released);
		if (need_resched())
			goto out;
	} while (transaction != last_transaction);
out:
	return ret;
}

/* 
 * hfsplus_jbd_remove_checkpoint: called after a buffer has been committed
 * to disk (either by being write-back flushed to disk, or being
 * committed to the log).
 *
 * We cannot safely clean a transaction out of the log until all of the
 * buffer updates committed in that transaction have safely been stored
 * elsewhere on disk.  To achieve this, all of the buffers in a
 * transaction need to be maintained on the transaction's checkpoint
 * list until they have been rewritten, at which point this function is
 * called to remove the buffer from the existing transaction's
 * checkpoint list.  
 *
 * The function returns 1 if it frees the transaction, 0 otherwise.
 *
 * This function is called with the journal locked.
 * This function is called with j_list_lock held.
 * This function is called with jbd_lock_bh_state(jh2bh(jh))
 */

int __hfsplus_jbd_remove_checkpoint(struct hfsplus_jbd_head *jh)
{
	hfsplus_transaction_t *transaction;
	hfsplus_jbd_t *journal;
	int ret = 0;

	HFSPLUS_JBUFFER_TRACE(jh, "entry");

	if ((transaction = jh->b_cp_transaction) == NULL) {
		HFSPLUS_JBUFFER_TRACE(jh, "not on transaction");
		goto out;
	}
	journal = transaction->t_journal;

	__buffer_unlink(jh);
	jh->b_cp_transaction = NULL;

	if (transaction->t_checkpoint_list != NULL ||
	    transaction->t_checkpoint_io_list != NULL)
		goto out;
	HFSPLUS_JBUFFER_TRACE(jh, "transaction has no more buffers");

	/*
	 * There is one special case to worry about: if we have just pulled the
	 * buffer off a running or committing transaction's checkpoing list,
	 * then even if the checkpoint list is empty, the transaction obviously
	 * cannot be dropped!
	 *
	 * The locking here around t_state is a bit sleazy.
	 * See the comment at the end of journal_commit_transaction().
	 */
	if (transaction->t_state != HFSPLUS_T_FINISHED) {
		HFSPLUS_JBUFFER_TRACE(jh, "belongs to committing transaction");
		goto out;
	}

	/* OK, that was the last buffer for the transaction: we can now
	   safely remove this transaction from the log */

	__hfsplus_jbd_drop_transaction(journal, transaction);

	/* Just in case anybody was waiting for more transactions to be
           checkpointed... */
	wake_up(&journal->j_wait_logspace);
	ret = 1;
out:
	HFSPLUS_JBUFFER_TRACE(jh, "exit");
	return ret;
}

/*
 * hfsplus_jbd_insert_checkpoint: put a committed buffer onto a checkpoint
 * list so that we know when it is safe to clean the transaction out of
 * the log.
 *
 * Called with the journal locked.
 * Called with j_list_lock held.
 */
void __hfsplus_jbd_insert_checkpoint(struct hfsplus_jbd_head *jh, 
			       hfsplus_transaction_t *transaction)
{
	HFSPLUS_JBUFFER_TRACE(jh, "entry");
	HFSPLUS_J_ASSERT_JH(jh, buffer_dirty(hfsplus_jh2bh(jh)) || buffer_hfsplus_jbddirty(hfsplus_jh2bh(jh)));
	HFSPLUS_J_ASSERT_JH(jh, jh->b_cp_transaction == NULL);

	jh->b_cp_transaction = transaction;

	if (!transaction->t_checkpoint_list) {
		jh->b_cpnext = jh->b_cpprev = jh;
	} else {
		jh->b_cpnext = transaction->t_checkpoint_list;
		jh->b_cpprev = transaction->t_checkpoint_list->b_cpprev;
		jh->b_cpprev->b_cpnext = jh;
		jh->b_cpnext->b_cpprev = jh;
	}
	transaction->t_checkpoint_list = jh;
}

/*
 * We've finished with this transaction structure: adios...
 * 
 * The transaction must have no links except for the checkpoint by this
 * point.
 *
 * Called with the journal locked.
 * Called with j_list_lock held.
 */

void __hfsplus_jbd_drop_transaction(hfsplus_jbd_t *journal, hfsplus_transaction_t *transaction)
{
	assert_spin_locked(&journal->j_list_lock);
	if (transaction->t_cpnext) {
		transaction->t_cpnext->t_cpprev = transaction->t_cpprev;
		transaction->t_cpprev->t_cpnext = transaction->t_cpnext;
		if (journal->j_checkpoint_transactions == transaction)
			journal->j_checkpoint_transactions =
				transaction->t_cpnext;
		if (journal->j_checkpoint_transactions == transaction)
			journal->j_checkpoint_transactions = NULL;
	}

	HFSPLUS_J_ASSERT(transaction->t_state == HFSPLUS_T_FINISHED);
	HFSPLUS_J_ASSERT(transaction->t_buffers == NULL);
	HFSPLUS_J_ASSERT(transaction->t_sync_datalist == NULL);
	HFSPLUS_J_ASSERT(transaction->t_forget == NULL);
	HFSPLUS_J_ASSERT(transaction->t_iobuf_list == NULL);
	HFSPLUS_J_ASSERT(transaction->t_shadow_list == NULL);
	HFSPLUS_J_ASSERT(transaction->t_log_list == NULL);
	HFSPLUS_J_ASSERT(transaction->t_checkpoint_list == NULL);
	HFSPLUS_J_ASSERT(transaction->t_checkpoint_io_list == NULL);
	HFSPLUS_J_ASSERT(transaction->t_updates == 0);
	HFSPLUS_J_ASSERT(journal->j_committing_transaction != transaction);
	HFSPLUS_J_ASSERT(journal->j_running_transaction != transaction);

	dprint(DBG_JCHKPT, "Dropping transaction %d, all done\n", transaction->t_tid);
	kfree(transaction);
}