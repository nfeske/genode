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
#include <sup.h>
#include <sup_gmm.h>

static bool const debug = true;

using namespace Genode;


namespace Sup { struct Nem; }

struct Sup::Nem
{
	Gmm &_gmm;

	typedef Sup::Gmm::Protection Protection;

	struct Range
	{
		addr_t     first_byte { 0 };
		addr_t     last_byte  { 0 };
		Protection prot       { false, false };

		size_t size() const { return last_byte ? last_byte - first_byte + 1 : 0; }

		bool extend(Range const &other)
		{
			/* ignore empty-range extension */
			if (!other.size())
				return true;

			if (!(prot == other.prot))
				return false;

			/* initialize if uninitialized */
			if (!size()) {
				first_byte = other.first_byte;
				last_byte  = other.last_byte;
				prot       = other.prot;

				return true;
			}

			/* prepend */
			if (first_byte == other.last_byte + 1) {
				first_byte = other.first_byte;

				return true;
			}

			/* append */
			if (last_byte + 1 == other.first_byte) {
				last_byte = other.last_byte;

				return true;
			}

			/* not contiguous (which includes overlaps) */
			return false;
		}

		void print(Output &o) const
		{
			using Genode::print;

			print(o, "r");
			print(o, prot.writeable  ? "w" : "-");
			print(o, prot.executable ? "x" : "-");
			print(o, ":", Hex_range(first_byte, size()));
		}
	};

	Range host_range  { };
	Range guest_range { };

	void commit_range()
	{
		/* ignore commit of empty ranges */
		if (!host_range.size())
			return;

		log(__PRETTY_FUNCTION__, " host=", host_range , " guest=", guest_range);

		/* commit the current range to GMM */
		_gmm.map_to_guest(Gmm::Vmm_addr   { host_range.first_byte },
		                  Gmm::Guest_addr { guest_range.first_byte },
		                  Gmm::Pages      { host_range.size() >> PAGE_SHIFT },
		                  host_range.prot);

		/* reset ranges */
		host_range  = { };
		guest_range = { };
	}

	void map_page_to_guest(addr_t host_addr, addr_t guest_addr, Protection prot)
	{
		Range new_host_range  { host_addr,  host_addr  + (PAGE_SIZE - 1), prot };
		Range new_guest_range { guest_addr, guest_addr + (PAGE_SIZE - 1), prot };

		/* new page just extends the current ranges */
		if (new_host_range.extend(host_range)
		 && new_guest_range.extend(guest_range)) {

			host_range  = new_host_range;
			guest_range = new_guest_range;

			return;
		}

		/* new page starts a new range */
		commit_range();

		/* start over with new page */
		host_range  = { host_addr,  host_addr  + (PAGE_SIZE - 1), prot };
		guest_range = { guest_addr, guest_addr + (PAGE_SIZE - 1), prot };
	}

	Nem(Gmm &gmm) : _gmm(gmm) { }
};


Sup::Nem * nem_ptr;

void Sup::nem_init(Gmm &gmm)
{
	nem_ptr = new Nem(gmm);
}


VMM_INT_DECL(int) NEMImportStateOnDemand(PVMCPUCC pVCpu, ::uint64_t fWhat) STOP


VMM_INT_DECL(int) NEMHCQueryCpuTick(PVMCPUCC pVCpu, ::uint64_t *pcTicks,
                                    ::uint32_t *puAux) STOP


VMM_INT_DECL(int) NEMHCResumeCpuTickOnAll(PVMCC pVM, PVMCPUCC pVCpu,
                                          ::uint64_t uPausedTscValue) STOP


void nemHCNativeNotifyHandlerPhysicalRegister(PVMCC pVM,
                                              PGMPHYSHANDLERKIND enmKind,
                                              RTGCPHYS GCPhys, RTGCPHYS cb)
{
	nem_ptr->commit_range();
}


int nemR3NativeInit(PVM pVM, bool fFallback, bool fForced)
{
	VM_SET_MAIN_EXECUTION_ENGINE(pVM, VM_EXEC_ENGINE_NATIVE_API);

	return VINF_SUCCESS;
}


int nemR3NativeInitAfterCPUM(PVM pVM) TRACE(VINF_SUCCESS)


int nemR3NativeInitCompleted(PVM pVM, VMINITCOMPLETED enmWhat) TRACE(VINF_SUCCESS)


int nemR3NativeTerm(PVM pVM) STOP


void nemR3NativeReset(PVM pVM) STOP


void nemR3NativeResetCpu(PVMCPU pVCpu, bool fInitIpi) STOP


VBOXSTRICTRC nemR3NativeRunGC(PVM pVM, PVMCPU pVCpu)
{
	warning(__PRETTY_FUNCTION__, " pVM=", pVM, " pVCpu=", pVCpu);

	/* handle interrupt injection */
	warning(__PRETTY_FUNCTION__, " 1 VMCPU_FF_IS_ANY_SET=",
	        VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_UPDATE_APIC | VMCPU_FF_INTERRUPT_PIC
                                     | VMCPU_FF_INTERRUPT_NMI  | VMCPU_FF_INTERRUPT_SMI));
	warning(__PRETTY_FUNCTION__, " 2 VM_FF_IS_ANY_SET=",
	        VM_FF_IS_ANY_SET(pVM, VM_FF_EMT_RENDEZVOUS | VM_FF_TM_VIRTUAL_SYNC),
	        " VMCPU_FF_IS_ANY_SET=",
	        VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HM_TO_R3_MASK));

	return VINF_SUCCESS;
}


bool nemR3NativeCanExecuteGuest(PVM pVM, PVMCPU pVCpu) TRACE(true)


bool nemR3NativeSetSingleInstruction(PVM pVM, PVMCPU pVCpu, bool fEnable) TRACE(false)


void nemR3NativeNotifyFF(PVM pVM, PVMCPU pVCpu, ::uint32_t fFlags) STOP


int nemR3NativeNotifyPhysRamRegister(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb)
{
	warning(__PRETTY_FUNCTION__, " GCPhys=", Hex(GCPhys), " cb=", Hex(cb));

	return VINF_SUCCESS;
}


int nemR3NativeNotifyPhysMmioExMap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb,
                                   ::uint32_t fFlags, void *pvMmio2) STOP


int nemR3NativeNotifyPhysMmioExUnmap(PVM pVM, RTGCPHYS GCPhys, RTGCPHYS cb,
                                     ::uint32_t fFlags) STOP


int nemR3NativeNotifyPhysRomRegisterEarly(PVM pVM, RTGCPHYS GCPhys,
                                          RTGCPHYS cb, ::uint32_t fFlags)
{
	return VINF_SUCCESS;
}


int nemR3NativeNotifyPhysRomRegisterLate(PVM pVM, RTGCPHYS GCPhys,
                                         RTGCPHYS cb, ::uint32_t fFlags)
{
	nem_ptr->commit_range();

	return VINF_SUCCESS;
}


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
                                       ::uint8_t *pu2State)
{
	warning(__PRETTY_FUNCTION__, " GCPhys=", (void *)GCPhys,
	        " HCPhys=", (void *)HCPhys, " fPageProt=", Hex(fPageProt));

	return VINF_SUCCESS;
}


void nemHCNativeNotifyPhysPageProtChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhys,
                                          ::uint32_t fPageProt, PGMPAGETYPE enmType,
                                          ::uint8_t *pu2State)
{
	bool const r { fPageProt & NEM_PAGE_PROT_READ };
	bool const w { fPageProt & NEM_PAGE_PROT_WRITE };
	bool const x { fPageProt & NEM_PAGE_PROT_EXECUTE };

	if (!r)
		warning(__PRETTY_FUNCTION__, ": unreadable mapping requested");

	nem_ptr->map_page_to_guest(HCPhys, GCPhys, Sup::Nem::Protection { x, w });
}


void nemHCNativeNotifyPhysPageChanged(PVMCC pVM, RTGCPHYS GCPhys, RTHCPHYS HCPhysPrev,
                                      RTHCPHYS HCPhysNew, ::uint32_t fPageProt,
                                      PGMPAGETYPE enmType, ::uint8_t *pu2State) STOP

