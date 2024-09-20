#pragma once

#include <Windows.h>

/*-------RAII--------
	  LockGuard
-------------------*/

class SRWLockGuard
{
public:
	SRWLockGuard(SRWLOCK* pLock) : _pLock(pLock) { AcquireSRWLockExclusive(_pLock); }
	~SRWLockGuard() { ReleaseSRWLockExclusive(_pLock); }

	SRWLOCK* _pLock;
};
