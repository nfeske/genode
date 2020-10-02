#include "stub_macros.h"

static bool const debug = false;

#include "VirtualBoxErrorInfoImpl.h"

HRESULT VirtualBoxErrorInfo::init(HRESULT, const GUID &, const char *,
                                  const Utf8Str &, IVirtualBoxErrorInfo *) STOP

NS_IMPL_THREADSAFE_ISUPPORTS1_CI(VirtualBoxErrorInfo, IVirtualBoxErrorInfo)
