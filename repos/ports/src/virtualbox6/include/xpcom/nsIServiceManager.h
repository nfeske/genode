#ifndef __gen_nsIServiceManager_h__
#define __gen_nsIServiceManager_h__

#include "nsCOMPtr.h"

struct nsGetServiceByContractID : nsCOMPtr_helper
{
	void *ptr;

	nsGetServiceByContractID(void *ptr) : ptr(ptr) { }

	nsresult operator()(char const *nsIID, void **out_ptr) const override
	{
		*out_ptr = ptr;
		return NS_OK;
	}
};


nsGetServiceByContractID
do_GetService(const char* aContractID, nsresult* error = 0);

#endif /* __gen_nsIServiceManager_h__ */
