#ifndef nsID_h__
#define nsID_h__

#ifndef nscore_h___
#include "nscore.h"
#endif

typedef struct { char x [sizeof(RTUUID)]; } nsID; /* differs from original */

typedef nsID nsCID;
typedef nsID nsIID;

#define NS_DEFINE_STATIC_IID_ACCESSOR(x)
#define REFNSIID const nsIID&

#endif
