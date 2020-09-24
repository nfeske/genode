#ifndef ___VBox_com_com_h
#define ___VBox_com_com_h

#include <VBox/com/defs.h>

namespace com {
	int GetVBoxUserHomeDirectory(char *aDir, size_t aDirLen, bool fCreateDir = true);
	HRESULT Initialize(bool fGui = false);
	HRESULT Shutdown();

	int VBoxLogRelCreate(const char *pszEntity, const char *pszLogFile,
	                     uint32_t fFlags, const char *pszGroupSettings,
	                     const char *pszEnvVarBase, uint32_t fDestFlags,
	                     uint32_t cMaxEntriesPerGroup, uint32_t cHistory,
	                     uint32_t uHistoryFileTime, uint64_t uHistoryFileSize,
	                     PRTERRINFO pErrInfo);
}

#endif /* ___VBox_com_com_h */
