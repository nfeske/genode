#include "stub_macros.h"

static bool const debug = false;

#include "VirtualBoxErrorInfoImpl.h"

HRESULT VirtualBoxErrorInfo::init(HRESULT, const GUID &, const char *,
                                  const Utf8Str &, IVirtualBoxErrorInfo *) STOP

HRESULT VirtualBoxErrorInfo::init(com::ErrorInfo const&, IVirtualBoxErrorInfo*) STOP

HRESULT VirtualBoxErrorInfo::initEx(unsigned int, int, nsID const&,
                                    char const*, com::Utf8Str const&,
                                    IVirtualBoxErrorInfo*) TRACE(VINF_SUCCESS)

NS_IMPL_THREADSAFE_ISUPPORTS1_CI(VirtualBoxErrorInfo, IVirtualBoxErrorInfo)
