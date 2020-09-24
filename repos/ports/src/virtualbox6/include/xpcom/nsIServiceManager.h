#ifndef __gen_nsIServiceManager_h__
#define __gen_nsIServiceManager_h__

#include "nsCOMPtr.h"

struct nsGetServiceByContractID : nsCOMPtr_helper
{
};

nsGetServiceByContractID
do_GetService(const char* aContractID, nsresult* error = 0);

#endif /* __gen_nsIServiceManager_h__ */
