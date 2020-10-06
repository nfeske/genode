#ifndef __gen_nsISupports_h__
#define __gen_nsISupports_h__

#ifndef nsError_h__
#include "nsError.h"
#endif

#ifndef nsISupportsImpl_h__
#include "nsISupportsImpl.h"
#endif

#include "nsID.h"

typedef unsigned long nsrefcnt;

struct nsISupports
{
	/* make the class polymorphic, so it can be used with 'dynamic_cast' */
	virtual ~nsISupports() { }

	class Not_implemented { };

	nsrefcnt AddRef() { throw Not_implemented(); }

	void Release() { throw Not_implemented();  }

	void QueryInterface(nsIID aIID, void **aInstancePtr)
	{
		*aInstancePtr = this;
	}
};

#endif /* __gen_nsISupports_h__ */
