/*
 * \brief  Genode backend for VirtualBox native execution manager
 * \author Norman Feske
 * \date   2020-11-05
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/log.h>

/* VirtualBox includes */
#include <VBox/vmm/nem.h>
#include <VBox/vmm/vmcc.h>
#include <VBox/err.h>
#include <NEMInternal.h>

/* local includes */
#include <stub_macros.h>

static bool const debug = true;

using namespace Genode;


VMM_INT_DECL(int) NEMImportStateOnDemand(PVMCPUCC pVCpu, ::uint64_t fWhat) STOP


VMM_INT_DECL(int) NEMHCQueryCpuTick(PVMCPUCC pVCpu, ::uint64_t *pcTicks,
                                    ::uint32_t *puAux) STOP


VMM_INT_DECL(int) NEMHCResumeCpuTickOnAll(PVMCC pVM, PVMCPUCC pVCpu,
                                          ::uint64_t uPausedTscValue) STOP


void nemHCNativeNotifyHandlerPhysicalRegister(PVMCC pVM,
                                              PGMPHYSHANDLERKIND enmKind,
                                              RTGCPHYS GCPhys, RTGCPHYS cb) STOP


int nemR3NativeInit(PVM pVM, bool fFallback, bool fForced)
{
	VM_SET_MAIN_EXECUTION_ENGINE(pVM, VM_EXEC_ENGINE_NATIVE_API);

	return VINF_SUCCESS;
}


int nemR3NativeInitAfterCPUM(PVM pVM) TRACE(VINF_SUCCESS)


int nemR3NativeInitCompleted(PVM pVM, VMINITCOMPLETED enmWhat) STOP


int nemR3NativeTerm(PVM pVM) STOP


void nemR3NativeReset(PVM pVM) STOP


void nemR3NativeResetCpu(PVMCPU pVCpu, bool fInitIpi) STOP


VBOXSTRICTRC nemR3NativeRunGC(PVM pVM, PVMCPU pVCpu) STOP


bool nemR3NativeCanExecuteGuest(PVM pVM, PVMCPU pVCpu) STOP


bool nemR3NativeSetSingleInstruction(PVM pVM, PVMCPU pVCpu, bool fEnable) STOP


void nemR3NativeNotifyFF(PVM pVM, PVMCPU pVCpu, ::uint32_t fFlags) STOP


int nemR3NativeNotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb)
{
	log(__PRETTY_FUNCTION__, " GCPhys=", Hex(GCPhys), " cb=", Hex(cb));

	return VINF_SUCCESS;
}


int nemR3NativeNotifyPhysMmioExMap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb,
                                   ::uint32_t fFlags, void *pvMmio2) STOP


int nemR3NativeNotifyPhysMmioExUnmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb,
                                     ::uint32_t fFlags) STOP


int nemR3NativeNotifyPhysRomRegisterEarly(PVM pVM, RTGCPHYS GCPhys,
                                          RTGCPHYS cb, ::uint32_t fFlags) STOP


int nemR3NativeNotifyPhysRomRegisterLate(PVM pVM, RTGCPHYS GCPhys,
                                         RTGCPHYS cb, ::uint32_t fFlags) STOP


void nemR3NativeNotifySetA20(PVMCPU pVCpu, bool fEnabled) STOP


void nemHCNativeNotifyHandlerPhysicalDeregister(PVMCC pVM, PGMPHYSHANDLERKIND enmKind,
                                                RTGCPHYS GCPhys, RTGCPHYS cb,
                                                int fRestoreAsRAM,
                                                bool fRestoreAsRAM2) STOP


void nemHCNativeNotifyHandlerPhysicalModify(PVMCC pVM, PGMPHYSHANDLERKIND enmKind,
                                            RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew,
                                            RTGCPHYS cb, bool fRestoreAsRAM) STOP


int nemHCNativeNotifyPhysPageAllocated(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys,
                                       ::uint32_t fPageProt, PGMPAGETYPE enmType,
                                       ::uint8_t *pu2State) STOP


void nemHCNativeNotifyPhysPageProtChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys,
                                          ::uint32_t fPageProt, PGMPAGETYPE enmType,
                                          ::uint8_t *pu2State) STOP


void nemHCNativeNotifyPhysPageChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhysPrev,
                                      RTHCPHYS HCPhysNew, ::uint32_t fPageProt,
                                      PGMPAGETYPE enmType, ::uint8_t *pu2State) STOP

