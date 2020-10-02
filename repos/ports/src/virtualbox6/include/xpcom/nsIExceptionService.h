#ifndef __gen_nsIExceptionService_h__
#define __gen_nsIExceptionService_h__

#include "nsIException.h"

struct nsIExceptionManager
{
	nsresult GetCurrentException(nsIException **);
	nsresult SetCurrentException(nsIException *);
};

struct nsIExceptionService
{
	nsresult GetCurrentExceptionManager(already_AddRefed<nsIExceptionManager>);
};

#define NS_EXCEPTIONSERVICE_CONTRACTID ns_type_trait<nsIExceptionService>::name()

#endif /* __gen_nsIExceptionService_h__ */
