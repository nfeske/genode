#ifndef nsISupportsImpl_h__
#define nsISupportsImpl_h__

#ifndef nscore_h___
#include "nscore.h"
#endif

#include <base/log.h>

#define NS_DECL_ISUPPORTS \
public: \
  \
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr); \
  \
  NS_IMETHOD_(nsrefcnt) AddRef(void) \
  { \
    Genode::error(__PRETTY_FUNCTION__, " not implemented"); \
    class Not_implemented { }; \
    throw Not_implemented(); \
  } \
  \
  NS_IMETHOD_(nsrefcnt) Release(void) \
  { \
    Genode::error(__PRETTY_FUNCTION__, " not implemented"); \
    class Not_implemented { }; \
    throw Not_implemented(); \
  }

#define NS_DECL_CLASSINFO(x)
#define NS_IMPL_THREADSAFE_ADDREF(x)
#define NS_IMPL_THREADSAFE_RELEASE(x)

#define NS_IMPL_QUERY_INTERFACE1_CI(a, b) \
  nsresult a::QueryInterface(REFNSIID aIID, void** aInstancePtr) \
  { \
    if (b *ptr = dynamic_cast<b *>(this)) { *aInstancePtr = ptr; } \
    \
    return *aInstancePtr ? NS_OK : NS_NOINTERFACE; \
  }

#define NS_IMPL_QUERY_INTERFACE2_CI(a, b, c) \
  nsresult a::QueryInterface(REFNSIID aIID, void** aInstancePtr) \
  { \
    if      (b *ptr = dynamic_cast<b *>(this)) { *aInstancePtr = ptr; } \
    else if (c *ptr = dynamic_cast<c *>(this)) { *aInstancePtr = ptr; } \
    \
    return *aInstancePtr ? NS_OK : NS_NOINTERFACE; \
  }

#define NS_IMPL_QUERY_INTERFACE3_CI(a, b, c, d) \
  nsresult a::QueryInterface(REFNSIID aIID, void** aInstancePtr) \
  { \
    if      (b *ptr = dynamic_cast<b *>(this)) { *aInstancePtr = ptr; } \
    else if (c *ptr = dynamic_cast<c *>(this)) { *aInstancePtr = ptr; } \
    else if (d *ptr = dynamic_cast<d *>(this)) { *aInstancePtr = ptr; } \
    \
    return *aInstancePtr ? NS_OK : NS_NOINTERFACE; \
  }

#define NS_IMPL_QUERY_INTERFACE4_CI(a, b, c, d, e)
#define NS_IMPL_QUERY_INTERFACE5_CI(a, b, c, d, e, f)
#define NS_IMPL_CI_INTERFACE_GETTER1(a, b)
#define NS_IMPL_CI_INTERFACE_GETTER2(a, b, c)
#define NS_IMPL_CI_INTERFACE_GETTER3(a, b, c, d)
#define NS_IMPL_CI_INTERFACE_GETTER4(a, b, c, d, e)
#define NS_IMPL_CI_INTERFACE_GETTER5(a, b, c, d, e, f)

#endif
