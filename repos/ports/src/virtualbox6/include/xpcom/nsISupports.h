#ifndef __gen_nsISupports_h__
#define __gen_nsISupports_h__

#ifndef nsError_h__
#include "nsError.h"
#endif

#ifndef nsISupportsImpl_h__
#include "nsISupportsImpl.h"
#endif

#include "nsID.h"

#include <base/log.h>

typedef unsigned long nsrefcnt;

struct nsISupports
{
	/* make the class polymorphic, so it can be used with 'dynamic_cast' */
	virtual ~nsISupports() { }

	class Not_implemented { };

	nsrefcnt AddRef()
	{
		Genode::warning(__PRETTY_FUNCTION__, " not implemented, returning 1");
		return 1;
	}

	void Release()
	{
		Genode::warning(__PRETTY_FUNCTION__, " not implemented");
	}

	void QueryInterface(nsIID aIID, void **aInstancePtr)
	{
		Genode::log(__PRETTY_FUNCTION__, " called");
		*aInstancePtr = this;
	}
};

#endif /* __gen_nsISupports_h__ */
