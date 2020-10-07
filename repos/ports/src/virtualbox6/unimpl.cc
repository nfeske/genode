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
	Genode::warning(__func__, ": " #name " called, not implemented, eip=", \
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
DUMMY(MMR3HyperRealloc)
DUMMY(MMR3LockCall)
DUMMY(MMR3PageDummyHCPhys)
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
DUMMY(RTAvloU32Get)
DUMMY(RTAvloU32Insert)
DUMMY(RTAvlrFileOffsetGet)
DUMMY(RTAvlrFileOffsetGetBestFit)
DUMMY(RTAvlrFileOffsetInsert)
DUMMY(RTAvlrFileOffsetRangeGet)
DUMMY(RTAvlrFileOffsetRemove)
DUMMY(RTAvlrU64Destroy)
DUMMY(RTAvlrU64DoWithAll)
DUMMY(RTAvlrU64GetBestFit)
DUMMY(RTAvlrU64Insert)
DUMMY(RTAvlrU64RangeGet)
DUMMY(RTAvlrU64RangeRemove)
DUMMY(RTAvlrU64Remove)
DUMMY(RTAvlrUIntPtrDestroy)
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
DUMMY(RTLdrGetSuff)
DUMMY(RTLdrLoadAppPriv)
DUMMY(RTLdrLoadEx)
DUMMY(RTProcCreate)
DUMMY(RTSystemQueryAvailableRam)
DUMMY(RTUdpCreateClientSocket)
DUMMY(RTZipXarFsStreamFromIoStream)
DUMMY(SELMR3GetSelectorInfo)
DUMMY(SUPGetCpuHzFromGipForAsyncMode)
DUMMY(SUPR3ContAlloc)
DUMMY(SUPR3ContFree)
DUMMY(SUPR3GetHwvirtMsrs)
DUMMY(SUPR3HardenedLdrLoadPlugIn)
DUMMY(SUPR3HardenedVerifyPlugIn)
DUMMY(SUPR3PageAlloc)
DUMMY(SUPR3PageFree)
DUMMY(SUPR3ReadTsc)
DUMMY(SUPR3SetVMForFastIOCtl)
DUMMY(SUPReadTscWithDelta)
DUMMY(SUPSemEventGetResolution)
DUMMY(SUPSemEventMultiWaitNsAbsIntr)
DUMMY(SUPSemEventMultiWaitNsRelIntr)
DUMMY(SUPSemEventWaitNsAbsIntr)
DUMMY(SUPSemEventWaitNsRelIntr)
DUMMY(VDIfTcpNetInstDefaultDestroy)

} /* extern "C" */
