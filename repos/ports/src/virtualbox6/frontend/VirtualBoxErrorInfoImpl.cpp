#include "dummy/macros.h"

#include "VirtualBoxErrorInfoImpl.h"

HRESULT VirtualBoxErrorInfo::init(HRESULT, const GUID &, const char *,
                                  const Utf8Str &, IVirtualBoxErrorInfo *) DUMMY()

NS_IMPL_THREADSAFE_ISUPPORTS1_CI(VirtualBoxErrorInfo, IVirtualBoxErrorInfo)
