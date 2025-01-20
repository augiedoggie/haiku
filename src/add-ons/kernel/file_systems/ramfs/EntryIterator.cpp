/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Directory.h"
#include "Entry.h"
#include "EntryIterator.h"
#include "Volume.h"

#include <util/AutoLock.h>


EntryIterator::EntryIterator(Directory *directory)
	:
	fDirectory(directory),
	fEntry(NULL),
	fSuspended(false),
	fIsNext(false),
	fDone(false)
{
}


EntryIterator::~EntryIterator()
{
	Unset();
}


status_t
EntryIterator::SetTo(Directory *directory)
{
	Unset();

	if (directory == NULL)
		return B_BAD_VALUE;

	fDirectory = directory;
	fEntry = NULL;
	fSuspended = false;
	fIsNext = false;
	fDone = false;

	return B_OK;
}


void
EntryIterator::Unset()
{
	if (fDirectory != NULL && fSuspended)
		Resume();

	fDirectory = NULL;
	fEntry = NULL;
	fSuspended = false;
	fIsNext = false;
	fDone = false;
}


status_t
EntryIterator::Suspend()
{
	if (fDirectory == NULL || fSuspended)
		return B_ERROR;

	if (fEntry != NULL) {
		RecursiveLocker locker(fDirectory->GetVolume()->GetIteratorLock());
		if (!locker.IsLocked())
			return B_ERROR;
		fEntry->AttachEntryIterator(this);
	}

	fSuspended = true;
	return B_OK;
}


status_t
EntryIterator::Resume()
{
	if (fDirectory == NULL)
		return B_ERROR;

	if (fSuspended && fEntry != NULL) {
		RecursiveLocker locker(fDirectory->GetVolume()->GetIteratorLock());
		if (!locker.IsLocked())
			return B_ERROR;
		fEntry->DetachEntryIterator(this);
	}

	fSuspended = false;
	return B_OK;
}


status_t
EntryIterator::GetNext(Entry **entry)
{
	if (fDirectory == NULL || entry == NULL)
		return B_BAD_VALUE;

	status_t error = B_ENTRY_NOT_FOUND;
	if (!fDone) {
		if (fIsNext) {
			fIsNext = false;
			if (fEntry)
				error = B_OK;
		} else
			error = fDirectory->GetNextEntry(&fEntry);
		*entry = fEntry;
	}
	fDone = (error != B_OK);
	return error;
}


status_t
EntryIterator::Rewind()
{
	if (fDirectory == NULL)
		return B_ERROR;

	RecursiveLocker locker(fDirectory->GetVolume()->GetIteratorLock());
	if (!locker.IsLocked())
		return B_ERROR;

	if (fSuspended && fEntry != NULL)
		fEntry->DetachEntryIterator(this);

	fEntry = NULL;
	fIsNext = false;
	fDone = false;

	return B_OK;
}


void
EntryIterator::SetCurrent(Entry *entry, bool isNext)
{
	fIsNext = isNext;
	fEntry = entry;
	fDone = !fEntry;
}
