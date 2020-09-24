#ifndef __gen_nsIExceptionService_h__
#define __gen_nsIExceptionService_h__

#include "nsIException.h"

struct nsIExceptionService
{
	class Not_implemented { };

	template <typename T>
	nsresult GetCurrentExceptionManager(T) { throw Not_implemented(); }
};

struct nsIExceptionManager
{
	class Not_implemented { };

	template <typename T>
	nsresult GetCurrentException(T) { throw Not_implemented(); }

	nsresult SetCurrentException(nsIException *) { throw Not_implemented(); }
};

#define NS_EXCEPTIONSERVICE_CONTRACTID "@mozilla.org/exceptionservice;1"

#endif /* __gen_nsIExceptionService_h__ */
