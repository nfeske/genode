/*
 * \brief  Genode backend for VirtualBox Suplib
 * \author Norman Feske
 * \date   2020-10-12
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/env.h>
#include <base/log.h>

/* VirtualBox includes */
#include <VBox/err.h>
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/gvmm.h>
#include <SUPLibInternal.h>
#include <IOMInternal.h>
#define PDMPCIDEV_INCLUDE_PRIVATE   /* needed for PDMPCIDEVINT_DECLARED */
#define VBOX_IN_VMM                 /* needed for definition of PDMTASKTYPE */
#include <PDMInternal.h>
#undef VBOX_IN_VMM
#include <SUPDrvIOC.h>

/* local includes */
#include <init.h>
#include <sup_drv.h>
#include <sup_vm.h>
#include <stub_macros.h>

static bool const debug = true;

using namespace Genode;

static Sup::Drv *sup_drv;

void Sup::init(Env &env)
{
	sup_drv = new Sup::Drv(env);

	nem_init(sup_drv->gmm());
}


static bool before_first_call_of_ioctl_query_funcs = true;


/*******************************
 ** Ioctl interface functions **
 *******************************/

/* XXX init in COOKIE */
struct SUPDRVSESSION
{
	SUPDRVSESSION() { }
};


static int ioctl_cookie(SUPCOOKIE &request)
{
	warning(__PRETTY_FUNCTION__, " misses session-object creation");

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.u32SessionVersion = SUPDRV_IOC_VERSION;
	request.u.Out.cFunctions = 0;

	return VINF_SUCCESS;
}


static int ioctl_query_funcs(SUPQUERYFUNCS &request)
{
	warning(__PRETTY_FUNCTION__, " reports zero functions");

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.cFunctions = 0;

	before_first_call_of_ioctl_query_funcs = false;

	return VINF_SUCCESS;
}


static int ioctl_gip_map(SUPGIPMAP &request)
{
	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.HCPhysGip = 0;
	request.u.Out.pGipR3    = sup_drv->gip();
	request.u.Out.pGipR0    = 0;

	return VINF_SUCCESS;
}


static int ioctl_vt_caps(SUPVTCAPS &request)
{
	/*
	 * Return VERR_VMX_NO_VMX and VERR_SVM_NO_SVM to trigger the use of
	 * the native execution manager (follow NEMR3Init).
	 */
	switch (sup_drv->cpu_virt()) {
	case Sup::Drv::Cpu_virt::VMX:
		request.Hdr.rc      = VERR_VMX_NO_VMX;
		request.u.Out.fCaps = SUPVTCAPS_VT_X | SUPVTCAPS_NESTED_PAGING;
		break;
	case Sup::Drv::Cpu_virt::SVM:
		request.Hdr.rc      = VERR_SVM_NO_SVM;
		request.u.Out.fCaps = SUPVTCAPS_AMD_V | SUPVTCAPS_NESTED_PAGING;
		break;
	case Sup::Drv::Cpu_virt::NONE:
		request.Hdr.rc      = VERR_UNSUPPORTED_CPU;
		request.u.Out.fCaps = 0;
		break;
	}

	/*
	 * Prevent returning an erroneous rc value when VT caps are queried during
	 * the early initialization path of Host::init, i_updateProcessorFeatures.
	 * Otherwise, the assertions in i_updateProcessorFeatures would trigger.
	 *
	 * Later, when called during the VM initialization via vmR3InitRing3,
	 * HMR3Init, we have to return VERR_VMX_NO_VMX or VERR_SVM_NO_SVM to force
	 * the call of NEMR3Init.
	 */
	if (before_first_call_of_ioctl_query_funcs)
		request.Hdr.rc = VINF_SUCCESS;

	/*
	 * XXX are the following interesting?
	 * SUPVTCAPS_VTX_VMCS_SHADOWING
	 * SUPVTCAPS_VTX_UNRESTRICTED_GUEST
	 */

	return VINF_SUCCESS;
}


static int vmmr0_gvmm_create_vm(GVMMCREATEVMREQ &request)
{
	warning(__PRETTY_FUNCTION__
	       , " pSession=", request.pSession
	       , " cCpus=", request.cCpus
	       , " pVMR3=", request.pVMR3
	       , " pVMR0=", request.pVMR0
	       );

	Sup::Cpu_count cpu_count { request.cCpus };

	request.pVMR3 = &Sup::Vm::create(request.pSession, cpu_count);
	request.pVMR0 = (PVMR0)request.pVMR3;

	return VINF_SUCCESS;
}


static int vmmr0_gmm_initial_reservation(GMMINITIALRESERVATIONREQ &request)
{
	warning(__PRETTY_FUNCTION__
	       , " cBasePages=",   request.cBasePages
	       , " cShadowPages=", request.cShadowPages
	       , " cFixedPages=",  request.cFixedPages
	       , " enmPolicy=",    (int)request.enmPolicy
	       , " enmPriority=",  (int)request.enmPriority
	       );

	Sup::Gmm::Pages pages { request.cBasePages
	                      + request.cShadowPages
	                      + request.cFixedPages };

	sup_drv->gmm().pool_size(pages);

	return VINF_SUCCESS;
}


static int vmmr0_gmm_update_reservation(GMMUPDATERESERVATIONREQ &request)
{
	warning(__PRETTY_FUNCTION__
	       , " cBasePages=",   request.cBasePages
	       , " cShadowPages=", request.cShadowPages
	       , " cFixedPages=",  request.cFixedPages
	       );

	Sup::Gmm::Pages pages { request.cBasePages
	                      + request.cShadowPages
	                      + request.cFixedPages };

	sup_drv->gmm().pool_size(pages);

	return VINF_SUCCESS;
}


static int vmmr0_gmm_allocate_pages(GMMALLOCATEPAGESREQ &request)
{
	warning(__PRETTY_FUNCTION__
	       , " cPages=", request.cPages
	       , " enmAccount=", (int)request.enmAccount
	       );

	Sup::Gmm::Pages pages { request.cPages };

	using Vmm_addr = Sup::Gmm::Vmm_addr;

	Vmm_addr const vmm_addr = sup_drv->gmm().alloc(pages);

	for (unsigned i = 0; i < request.cPages; i++) {

		GMMPAGEDESC &page = request.aPages[i];

		Vmm_addr const page_addr { vmm_addr.value + i*PAGE_SIZE };

		warning(" for guest phys ", Hex(page.HCPhysGCPhys),
		        " page_addr=", Hex(page_addr.value),
		        " page_id=", Hex(sup_drv->gmm().page_id(page_addr).value));

		page.HCPhysGCPhys = page_addr.value;
		page.idPage       = sup_drv->gmm().page_id(page_addr).value;
		page.idSharedPage = NIL_GMM_PAGEID;
	}

	return VINF_SUCCESS;
}


static int vmmr0_gmm_map_unmap_chunk(GMMMAPUNMAPCHUNKREQ &request)
{
	warning(__PRETTY_FUNCTION__
	       , " idChunkMap=", request.idChunkMap
	       , " idChunkUnmap=", request.idChunkUnmap
	       , " pvR3=", request.pvR3
	       );

	if (request.idChunkMap != NIL_GMM_CHUNKID) {
		Sup::Gmm::Page_id const page_id { request.idChunkMap << GMM_CHUNKID_SHIFT };

		request.pvR3 = (RTR3PTR)sup_drv->gmm().vmm_addr(page_id).value;
	}

	return VINF_SUCCESS;
}


static int vmmr0_iom_grow_io_ports(PVMR0 pvmr0, ::uint64_t min_entries)
{
	warning(__PRETTY_FUNCTION__, " min_entries=", min_entries);

	/* satisfy IOMR3IoPortCreate */
	Sup::Vm &vm = *(Sup::Vm *)pvmr0;

	unsigned const old_bound = vm.iom.s.cIoPortAlloc;
	unsigned const new_bound = max(min_entries, old_bound);

	IOMIOPORTENTRYR3     *r3_entries     = new IOMIOPORTENTRYR3    [new_bound] { };
	IOMIOPORTLOOKUPENTRY *lookup_entries = new IOMIOPORTLOOKUPENTRY[new_bound] { };

	/* preserve content of the existing arrays */
	for (unsigned i = 0; i < old_bound; i++) {
		r3_entries    [i] = vm.iom.s.paIoPortRegs[i];
		lookup_entries[i] = vm.iom.s.paIoPortLookup[i];
	}

	/* initialize new array elements */
	for (unsigned i = old_bound; i < new_bound; i++) {
		r3_entries[i].idxSelf  = (uint16_t)i;
		r3_entries[i].idxStats = UINT16_MAX;
	}

	/* replace old arrays with new ones */
	delete vm.iom.s.paIoPortLookup;
	delete vm.iom.s.paIoPortRegs;

	vm.iom.s.paIoPortRegs   = r3_entries;
	vm.iom.s.paIoPortLookup = lookup_entries;
	vm.iom.s.cIoPortAlloc   = new_bound;

	return VINF_SUCCESS;
}


static int vmmr0_iom_grow_mmio_regs(PVMR0 pvmr0, ::uint64_t min_entries)
{
	warning(__PRETTY_FUNCTION__, " min_entries=", min_entries);

	/* satisfy IOMR3MmioCreate */
	Sup::Vm &vm = *(Sup::Vm *)pvmr0;

	unsigned const old_bound = vm.iom.s.cMmioAlloc;
	unsigned const new_bound = max(min_entries, old_bound);

	IOMMMIOENTRYR3     *r3_entries     = new IOMMMIOENTRYR3    [new_bound] { };
	IOMMMIOLOOKUPENTRY *lookup_entries = new IOMMMIOLOOKUPENTRY[new_bound] { };

	/* preserve content of the existing arrays */
	for (unsigned i = 0; i < old_bound; i++) {
		r3_entries    [i] = vm.iom.s.paMmioRegs[i];
		lookup_entries[i] = vm.iom.s.paMmioLookup[i];
	}

	/* initialize new array elements */
	for (unsigned i = old_bound; i < new_bound; i++) {
		r3_entries[i].idxSelf  = (uint16_t)i;
		r3_entries[i].idxStats = UINT16_MAX;
	}

	/* replace old arrays with new ones */
	delete vm.iom.s.paMmioLookup;
	delete vm.iom.s.paMmioRegs;

	vm.iom.s.paMmioRegs   = r3_entries;
	vm.iom.s.paMmioLookup = lookup_entries;
	vm.iom.s.cMmioAlloc   = new_bound;

	return VINF_SUCCESS;
}


static int vmmr0_pdm_device_create(PDMDEVICECREATEREQ &request)
{
	warning("PDMDEVICECREATEREQ for ", Cstring(request.szDevName));

	/*
	 * PDMDevHlp.cpp tests for certain allocation patterns, e.g., in
	 * pdmR3DevHlp_SetDeviceCritSect, there is the following assertion:
	 *
	 *   Assert((uintptr_t)pOldCritSect - (uintptr_t)pDevIns < pDevIns->cbRing3);
	 *
	 * The 'Allocation' object is used to satisfy the pattern.
	 */
	struct Allocation
	{
		PDMDEVINSR3 pdmdev   { };
		PDMCRITSECT critsect { };

		enum { NUM_PCI_DEVS = sizeof(pdmdev.apPciDevs) /
		                      sizeof(pdmdev.apPciDevs[0]) };

		PDMPCIDEV pcidevs[NUM_PCI_DEVS] { };

		Allocation()
		{
			pdmdev.pvInstanceDataForR3 = &pdmdev.achInstanceData[0];
			pdmdev.pCritSectRoR3       = &critsect;
			pdmdev.cbRing3             = sizeof(Allocation);

			/* needed for PDMDEV_CALC_PPCIDEV */
			pdmdev.cPciDevs = NUM_PCI_DEVS;
			pdmdev.cbPciDev = sizeof(PDMPCIDEV);

			for (size_t i = 0; i < NUM_PCI_DEVS; i++) {

				PDMPCIDEV &pcidev = pcidevs[i];

				pcidev.Int.s.idxSubDev = i;
				pcidev.idxSubDev       = i;
				pcidev.u32Magic        = PDMPCIDEV_MAGIC;

				pdmdev.apPciDevs[i] = &pcidev;
			}
		}

	} &allocation = *new Allocation { };

	PDMDEVINSR3 &pdmdev = allocation.pdmdev;

	/*
	 * The 'pvInstanceDataForR3' backing store is used for the R3 device state,
	 * e.g., DEVPCIROOT for the PCI bus, or KBDSTATE for the PS2 keyboard
	 */
	pdmdev.pvInstanceDataR3     = RTMemAllocZ(request.cbInstanceShared);
	pdmdev.fR0Enabled           = true;
	pdmdev.Internal.s.fIntFlags = PDMDEVINSINT_FLAGS_R0_ENABLED;
	pdmdev.u32Version           = PDM_DEVINS_VERSION;

	request.pDevInsR3 = &pdmdev;

	return VINF_SUCCESS;
}


static int vmmr0_pdm_device_gen_call(PDMDEVICEGENCALLREQ &request)
{
	warning("PDMDEVICEGENCALLREQ PDMDEVICEGENCALL=", (int)request.enmCall, " not implemented");

	return VINF_SUCCESS;
}


static int ioctl_call_vmmr0(SUPCALLVMMR0 &request)
{
	warning(__PRETTY_FUNCTION__
	       , " cbIn=", request.Hdr.cbIn
	       , " uOperation=", request.u.In.uOperation
	       , " u64Arg=", request.u.In.u64Arg
	       , " pVMR0=", (void*)request.u.In.pVMR0
	       );

	VMMR0OPERATION const operation = VMMR0OPERATION(request.u.In.uOperation);

	switch (operation) {
	case VMMR0_DO_GVMM_CREATE_VM:
		request.Hdr.rc = vmmr0_gvmm_create_vm(*(GVMMCREATEVMREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	case VMMR0_DO_GMM_INITIAL_RESERVATION:
		request.Hdr.rc = vmmr0_gmm_initial_reservation(*(GMMINITIALRESERVATIONREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	case VMMR0_DO_GMM_UPDATE_RESERVATION:
		request.Hdr.rc = vmmr0_gmm_update_reservation(*(GMMUPDATERESERVATIONREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	case VMMR0_DO_GMM_ALLOCATE_PAGES:
		request.Hdr.rc = vmmr0_gmm_allocate_pages(*(GMMALLOCATEPAGESREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	case VMMR0_DO_GMM_MAP_UNMAP_CHUNK:
		request.Hdr.rc = vmmr0_gmm_map_unmap_chunk(*(GMMMAPUNMAPCHUNKREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	case VMMR0_DO_IOM_GROW_IO_PORTS:
		request.Hdr.rc = vmmr0_iom_grow_io_ports(request.u.In.pVMR0, request.u.In.u64Arg);
		return VINF_SUCCESS;

	case VMMR0_DO_IOM_GROW_MMIO_REGS:
		request.Hdr.rc = vmmr0_iom_grow_mmio_regs(request.u.In.pVMR0, request.u.In.u64Arg);
		return VINF_SUCCESS;

	case VMMR0_DO_PDM_DEVICE_CREATE:
		request.Hdr.rc = vmmr0_pdm_device_create(*(PDMDEVICECREATEREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	case VMMR0_DO_PDM_DEVICE_GEN_CALL:
		request.Hdr.rc = vmmr0_pdm_device_gen_call(*(PDMDEVICEGENCALLREQ *)request.abReqPkt);
		return VINF_SUCCESS;

	default:
		error(__func__, " operation=", (int)operation);
		request.Hdr.rc = VERR_NOT_IMPLEMENTED;
		STOP
	}

	return VERR_NOT_IMPLEMENTED;
}


static int ioctl_get_hmvirt_msrs(SUPGETHWVIRTMSRS &request)
{
	warning(__PRETTY_FUNCTION__
	       , " fForce=", request.u.In.fForce
	       );

	request.Hdr.rc = VINF_SUCCESS;
	::memset(&request.u.Out, 0, sizeof(request.u.Out));

	return VINF_SUCCESS;
}


static int ioctl_ucode_rev(SUPUCODEREV &request)
{
	warning(__PRETTY_FUNCTION__);

	/* fake most recent revision possible */

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.MicrocodeRev = ~0u;

	return VINF_SUCCESS;
}


static int ioctl_get_paging_mode(SUPGETPAGINGMODE &request)
{
	warning(__PRETTY_FUNCTION__);

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.enmMode = sizeof(long) == 32 ? SUPPAGINGMODE_32_BIT_GLOBAL
	                                           : SUPPAGINGMODE_AMD64_GLOBAL_NX;

	return VINF_SUCCESS;
}


static int ioctl_page_alloc_ex(SUPPAGEALLOCEX &request)
{
	warning(__PRETTY_FUNCTION__
	       , " cPages=", request.u.In.cPages
	       , " fKernelMapping=", request.u.In.fKernelMapping
	       , " fUserMapping=", request.u.In.fUserMapping
	       );

	size_t const cPages = request.u.In.cPages;
	void * const base   = RTMemPageAllocZ(cPages*PAGE_SIZE);

	request.Hdr.rc = VINF_SUCCESS;
	request.u.Out.pvR3 = (R3PTRTYPE(void *))base;
	request.u.Out.pvR0 = (R0PTRTYPE(void *))base;

	for (size_t i = 0; i < cPages; ++i)
		request.u.Out.aPages[i] = (RTHCPHYS)base + i*PAGE_SIZE;

	return VINF_SUCCESS;
}


static int ioctl_set_vm_for_fast(SUPSETVMFORFAST &request)
{
	warning(__PRETTY_FUNCTION__
	       , " pVMR0=", request.u.In.pVMR0
	       );

	request.Hdr.rc = VINF_SUCCESS;

	return VINF_SUCCESS;
}


/*********************************
 ** VirtualBox suplib interface **
 *********************************/

int suplibOsInit(PSUPLIBDATA pThis, bool fPreInited, bool fUnrestricted,
                 SUPINITOP *penmWhat, PRTERRINFO pErrInfo)
{
	/* set hDevice to !NIL_RTFILE - checked by SUPR3PageAllocEx() */
	pThis->hDevice       = !NIL_RTFILE;
	pThis->fUnrestricted = fUnrestricted;

	return VINF_SUCCESS;
}


int suplibOsTerm(PSUPLIBDATA pThis) TRACE(VINF_SUCCESS)


int suplibOsInstall(void) TRACE(VERR_NOT_IMPLEMENTED)


int suplibOsUninstall(void) TRACE(VERR_NOT_IMPLEMENTED)


int suplibOsIOCtl(PSUPLIBDATA pThis, uintptr_t opcode, void *req, size_t len)
{
	switch (SUP_CTL_CODE_NO_SIZE(opcode)) {

	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_COOKIE):               return ioctl_cookie(*(SUPCOOKIE *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_QUERY_FUNCS()):        return ioctl_query_funcs(*(SUPQUERYFUNCS *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_GIP_MAP):              return ioctl_gip_map(*(SUPGIPMAP *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_VT_CAPS):              return ioctl_vt_caps(*(SUPVTCAPS *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_CALL_VMMR0_NO_SIZE()): return ioctl_call_vmmr0(*(SUPCALLVMMR0 *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_GET_HWVIRT_MSRS):      return ioctl_get_hmvirt_msrs(*(SUPGETHWVIRTMSRS *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_UCODE_REV):            return ioctl_ucode_rev(*(SUPUCODEREV *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_GET_PAGING_MODE):      return ioctl_get_paging_mode(*(SUPGETPAGINGMODE *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_PAGE_ALLOC_EX):        return ioctl_page_alloc_ex(*(SUPPAGEALLOCEX *)req);
	case SUP_CTL_CODE_NO_SIZE(SUP_IOCTL_SET_VM_FOR_FAST):      return ioctl_set_vm_for_fast(*(SUPSETVMFORFAST *)req);

	default:

		/*
		 * Ioctl not handled, print diagnostic info and spin.
		 * opcode number in lowest 7 bits
		 */
		error(__func__, " function=", opcode & 0x7f);
		STOP
	}

	return VERR_NOT_IMPLEMENTED;
}


int suplibOsIOCtlFast(PSUPLIBDATA pThis, uintptr_t uFunction,
                      uintptr_t idCpu) STOP


int suplibOsPageAlloc(PSUPLIBDATA pThis, size_t cPages, void **ppvPages) STOP


int suplibOsPageFree(PSUPLIBDATA pThis, void *pvPages, size_t cPages) STOP

