/**
 * NatFeats (Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#ifndef _NF_BASE_H
#define _NF_BASE_H

#include "natfeats.h"	/* nf_getparameter is defined there */

#ifdef USE_JIT
extern int in_handler;
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); in_handler = 0; longjmp(excep_env, 2); }
#else
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); longjmp(excep_env, 2); }
#endif

class NF_Base
{
protected:
	/**
	 * Host<->Atari mem & str functions
	 **/
	void a2fmemcpy( char *dest, memptr source, size_t count );
	void a2fstrcpy( char *dest, memptr source );
	void f2amemcpy( memptr dest, char *source, size_t count );
	void f2astrcpy( memptr dest, char *source );

public:
	virtual bool init() { return true; }
	virtual void exit() { }
	virtual char *name() = 0;
	virtual bool isSuperOnly() = 0;
	virtual int32 dispatch(uint32 fncode) = 0;
	uint32 getParameter(int i) { return nf_getparameter(i); }
};

#endif /* _NF_BASE_H */
