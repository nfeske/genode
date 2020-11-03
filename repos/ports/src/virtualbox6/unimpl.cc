/*
 * \brief  Dummy implementations of symbols needed by VirtualBox
 * \author Norman Feske
 * \date   2013-08-22
 */

/*
 * Copyright (C) 2013-2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#include <base/log.h>

#include <iprt/assert.h>

extern "C" {

#define DUMMY(name) \
void name(void) { \
	Genode::error(__func__, ": " #name " called, not implemented, eip=", \
	              __builtin_return_address(0)); \
	for (;;); \
}

DUMMY(DBGFR3CoreWrite)
DUMMY(DBGFR3LogModifyDestinations)
DUMMY(DBGFR3LogModifyFlags)
DUMMY(DBGFR3LogModifyGroups)
DUMMY(DBGFR3PagingDumpEx)
DUMMY(DBGFR3ReportBugCheck)
DUMMY(DBGFR3StackWalkBegin)
DUMMY(DBGFR3StackWalkBeginEx)
DUMMY(DBGFR3StackWalkEnd)
DUMMY(DBGFR3StackWalkNext)
DUMMY(drvHostBaseDestructOs)
DUMMY(drvHostBaseDoLockOs)
DUMMY(drvHostBaseEjectOs)
DUMMY(drvHostBaseFlushOs)
DUMMY(drvHostBaseGetMediaSizeOs)
DUMMY(drvHostBaseInitOs)
DUMMY(drvHostBaseIsMediaPollingRequiredOs)
DUMMY(drvHostBaseMediaRefreshOs)
DUMMY(drvHostBaseOpenOs)
DUMMY(drvHostBaseQueryMediaStatusOs)
DUMMY(drvHostBaseReadOs)
DUMMY(drvHostBaseScsiCmdGetBufLimitOs)
DUMMY(drvHostBaseScsiCmdOs)
DUMMY(drvHostBaseWriteOs)
DUMMY(PDMCritSectBothFF)
DUMMY(PDMNsAllocateBandwidth)
DUMMY(PDMR3LdrEnumModules)
DUMMY(PDMR3LdrGetInterfaceSymbols)
DUMMY(PDMR3LdrLoadR0)
DUMMY(PGMR3DbgR3Ptr2GCPhys)
DUMMY(PGMR3DbgReadGCPtr)
DUMMY(PGMR3MappingsDisable)
DUMMY(PGMR3MappingsFix)
DUMMY(PGMR3MappingsSize)
DUMMY(PGMR3MappingsUnfix)
DUMMY(PGMR3SharedModuleCheckAll)
DUMMY(PGMR3SharedModuleRegister)
DUMMY(PGMR3SharedModuleUnregister)
DUMMY(RTDbgAsCreate)
DUMMY(RTDbgAsLineByAddr)
DUMMY(RTDbgAsLockExcl)
DUMMY(RTDbgAsModuleByIndex)
DUMMY(RTDbgAsModuleCount)
DUMMY(RTDbgAsModuleLink)
DUMMY(RTDbgAsModuleQueryMapByIndex)
DUMMY(RTDbgAsModuleUnlink)
DUMMY(RTDbgAsName)
DUMMY(RTDbgAsRelease)
DUMMY(RTDbgAsRetain)
DUMMY(RTDbgAsSymbolByAddr)
DUMMY(RTDbgAsUnlockExcl)
DUMMY(RTDbgCfgChangeString)
DUMMY(RTDbgCfgChangeUInt)
DUMMY(RTDbgCfgCreate)
DUMMY(RTDbgCfgRelease)
DUMMY(RTDbgLineDup)
DUMMY(RTDbgLineFree)
DUMMY(RTDbgModCreateFromImage)
DUMMY(RTDbgModLineByAddr)
DUMMY(RTDbgModName)
DUMMY(RTDbgModRelease)
DUMMY(RTDbgModSegmentRva)
DUMMY(RTDbgModSymbolByAddr)
DUMMY(RTDbgModUnwindFrame)
DUMMY(RTDbgSymbolDup)
DUMMY(RTDbgSymbolFree)
DUMMY(RTFileQueryFsSizes)
DUMMY(RTFsIsoMakerCmdEx)
DUMMY(RTLdrLoadAppPriv)
DUMMY(RTLdrLoadEx)
DUMMY(RTProcCreate)
DUMMY(RTSystemQueryAvailableRam)
DUMMY(RTZipXarFsStreamFromIoStream)
DUMMY(SELMR3GetSelectorInfo)
DUMMY(SUPGetCpuHzFromGipForAsyncMode)
DUMMY(SUPReadTscWithDelta)
DUMMY(USBFilterClone)
DUMMY(VDIfTcpNetInstDefaultDestroy)

/* xpcom */
DUMMY(_MD_CreateUnixProcess)
DUMMY(_MD_CreateUnixProcessDetached)
DUMMY(_MD_KillUnixProcess)
DUMMY(_MD_WaitUnixProcess)
DUMMY(PR_FindSymbol)
DUMMY(PR_LoadLibrary)
DUMMY(PR_LoadLibraryWithFlags)
DUMMY(_PR_MapOptionName)
DUMMY(PR_UnloadLibrary)

} /* extern "C" */
