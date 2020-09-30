#include "VirtualBoxBase.h"

#include "dummy/macros.h"

#include "AutostartDb.h"

static bool debug = false;


int AutostartDb::addAutostartVM(const char *)    DUMMY()
int AutostartDb::addAutostopVM(char const*)      DUMMY()
int AutostartDb::removeAutostopVM(char const*)   DUMMY()
int AutostartDb::removeAutostartVM(char const*)  DUMMY()

AutostartDb::AutostartDb()                       TRACE()
AutostartDb::~AutostartDb() { }
int AutostartDb::setAutostartDbPath(char const*) TRACE(VINF_SUCCESS)
