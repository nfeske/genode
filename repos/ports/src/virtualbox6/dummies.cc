/*
 * \brief  Dummy implementations of symbols needed by VirtualBox
 * \author Norman Feske
 * \date   2013-08-22
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/log.h>
#include <base/sleep.h>
#include <util/string.h>

/* local includes */
#include "stub_macros.h"
#include "util.h"

static bool const debug = true;


/* ApplianceImplExport.cpp */

#include "MachineImpl.h"

HRESULT Machine::exportTo(const ComPtr<IAppliance> &aAppliance,
                          const com::Utf8Str &aLocation,
                          ComPtr<IVirtualSystemDescription> &aDescription) STOP

/* com.cpp */

int com::VBoxLogRelCreate(char const*, char const*, unsigned int, char const*,
                          char const*, unsigned int, unsigned int, unsigned int,
                          unsigned int, unsigned long, RTERRINFO*) TRACE(NS_OK)


/* DisplayPNGUtil.cpp */

#include "DisplayImpl.h"

int DisplayMakePNG(uint8_t *, uint32_t, uint32_t, uint8_t **, uint32_t *,
                   uint32_t *, uint32_t *, uint8_t) STOP


/* EventImpl.cpp */

#include "EventImpl.h"

HRESULT VBoxEventDesc::init(IEventSource*, VBoxEventType_T, ...) TRACE(S_OK)
HRESULT VBoxEventDesc::reinit(VBoxEventType_T, ...)              TRACE(S_OK)


/* initterm.cpp */

#include "VBox/com/com.h"

HRESULT com::Initialize(uint32_t) TRACE(S_OK)
HRESULT com::Shutdown()           STOP


/* USBFilter.cpp */

#include "VBox/usbfilter.h"

USBFILTERMATCH USBFilterGetMatchingMethod(PCUSBFILTER, USBFILTERIDX) STOP
char const *   USBFilterGetString        (PCUSBFILTER, USBFILTERIDX) STOP

int  USBFilterGetNum          (PCUSBFILTER, USBFILTERIDX) STOP
void USBFilterInit            (PUSBFILTER, USBFILTERTYPE) STOP
bool USBFilterIsMethodNumeric (USBFILTERMATCH)            STOP
bool USBFilterIsMethodString  (USBFILTERMATCH)            STOP
bool USBFilterIsNumericField  (USBFILTERIDX)              STOP
bool USBFilterIsStringField   (USBFILTERIDX)              STOP
bool USBFilterMatch           (PCUSBFILTER, PCUSBFILTER)  STOP
int  USBFilterSetIgnore       (PUSBFILTER, USBFILTERIDX)  STOP
int  USBFilterSetNumExact     (PUSBFILTER, USBFILTERIDX, uint16_t, bool)     STOP
int  USBFilterSetNumExpression(PUSBFILTER, USBFILTERIDX, const char *, bool) STOP
int  USBFilterSetStringExact  (PUSBFILTER, USBFILTERIDX, const char *, bool) STOP
int  USBFilterSetStringPattern(PUSBFILTER, USBFILTERIDX, const char *, bool) STOP
int  USBFilterSetStringExact  (PUSBFILTER, USBFILTERIDX, const char *, bool, bool) STOP
int  USBFilterMatchRated      (PCUSBFILTER, PCUSBFILTER) STOP


/* USBProxyBackend.cpp */

#include "USBProxyBackend.h"

USBProxyBackendFreeBSD::USBProxyBackendFreeBSD() STOP

USBProxyBackend::USBProxyBackend() STOP
USBProxyBackend::~USBProxyBackend() { }

HRESULT             USBProxyBackend::FinalConstruct() STOP
com::Utf8Str const &USBProxyBackend::i_getAddress()   STOP
com::Utf8Str const &USBProxyBackend::i_getId()        STOP

USBProxyBackendUsbIp::USBProxyBackendUsbIp() STOP


/* USBProxyService.cpp */

#include "USBProxyService.h"

USBProxyService::USBProxyService(Host* aHost) : mHost(aHost), mDevices(), mBackends() { }
USBProxyService::~USBProxyService() { }

HRESULT       USBProxyService::init() { return VINF_SUCCESS; }
RWLockHandle *USBProxyService::lockHandle() const                                  STOP
HRESULT       USBProxyService::autoCaptureDevicesForVM(SessionMachine *)           STOP
HRESULT       USBProxyService::captureDeviceForVM(SessionMachine *, IN_GUID,
                                                  com::Utf8Str const&)             STOP
HRESULT       USBProxyService::detachAllDevicesFromVM(SessionMachine*, bool, bool) STOP
HRESULT       USBProxyService::detachDeviceFromVM(SessionMachine*, IN_GUID, bool)  STOP
void         *USBProxyService::insertFilter(PCUSBFILTER aFilter)                   STOP
void          USBProxyService::removeFilter(void *aId)                             STOP
int           USBProxyService::getLastError()                                      STOP
bool          USBProxyService::isActive()                                          STOP
HRESULT       USBProxyService::removeUSBDeviceSource(com::Utf8Str const&)          STOP
HRESULT       USBProxyService::addUSBDeviceSource(com::Utf8Str const&,
                                                  com::Utf8Str const&,
                                                  com::Utf8Str const&,
                                                  std::vector<com::Utf8Str, std::allocator<com::Utf8Str> > const&,
                                                  std::vector<com::Utf8Str, std::allocator<com::Utf8Str> > const&) STOP
HRESULT       USBProxyService::getDeviceCollection(std::vector<ComPtr<IHostUSBDevice>,
                                                   std::allocator<ComPtr<IHostUSBDevice> > >&) STOP

using USBDeviceSourceList =
	std::__cxx11::list<settings::USBDeviceSource, std::allocator<settings::USBDeviceSource> >;

HRESULT USBProxyService::i_saveSettings(USBDeviceSourceList &)       TRACE(VINF_SUCCESS)
HRESULT USBProxyService::i_loadSettings(USBDeviceSourceList const &) TRACE(VINF_SUCCESS)


/* USBFilter.cpp */

#include "VBox/usbfilter.h"

USBLIB_DECL(USBFILTERTYPE) USBFilterGetFilterType(PCUSBFILTER) STOP
USBLIB_DECL(int)           USBFilterSetFilterType(PUSBFILTER, USBFILTERTYPE) STOP


/* ApplianceImpl.cpp */

HRESULT VirtualBox::createAppliance(ComPtr<IAppliance> &) STOP


/* CloudProviderManagerImpl.cpp */

#include "CloudProviderManagerImpl.h"

CloudProviderManager::CloudProviderManager() TRACE()
CloudProviderManager::~CloudProviderManager() { }

HRESULT CloudProviderManager::FinalConstruct() TRACE(VINF_SUCCESS)
void    CloudProviderManager::FinalRelease()   TRACE()
HRESULT CloudProviderManager::init()           TRACE(VINF_SUCCESS)
void    CloudProviderManager::uninit()         STOP
HRESULT CloudProviderManager::getProviderById       (com::Guid    const&, ComPtr<ICloudProvider>&) STOP
HRESULT CloudProviderManager::getProviderByName     (com::Utf8Str const&, ComPtr<ICloudProvider>&) STOP
HRESULT CloudProviderManager::getProviderByShortName(com::Utf8Str const&, ComPtr<ICloudProvider>&) STOP
HRESULT CloudProviderManager::getProviders(std::vector<ComPtr<ICloudProvider>,
                                           std::allocator<ComPtr<ICloudProvider> > >&) STOP


/* NetIf-freebsd.cpp */

#include "HostNetworkInterfaceImpl.h"
#include "netif.h"

int NetIfGetLinkSpeed(const char *, uint32_t *) STOP
int NetIfGetState(const char *, NETIFSTATUS *)  STOP
int NetIfRemoveHostOnlyNetworkInterface(VirtualBox *, const Guid &, IProgress **) STOP
int NetIfList(std::__cxx11::list<ComObjPtr<HostNetworkInterface>,
              std::allocator<ComObjPtr<HostNetworkInterface> > >&) TRACE(VINF_SUCCESS)


/* fatvfs.cpp */

#include "iprt/fsvfs.h"

RTDECL(int) RTFsFatVolFormat(RTVFSFILE, uint64_t, uint64_t, uint32_t, uint16_t,
                             uint16_t, RTFSFATTYPE, uint32_t, uint32_t,
                             uint8_t, uint16_t, uint32_t, PRTERRINFO) STOP

/* dvm.cpp */

#include "iprt/dvm.h"

RTDECL(uint32_t) RTDvmRelease(RTDVM)                                STOP
RTDECL(int)      RTDvmCreate(PRTDVM, RTVFSFILE, uint32_t, uint32_t) STOP
RTDECL(int)      RTDvmMapInitialize(RTDVM, const char *)            STOP


/* MachineImplMoveVM.cpp */

#include "MachineImplMoveVM.h"

HRESULT MachineMoveVM::init()                              STOP
void    MachineMoveVM::i_MoveVMThreadTask(MachineMoveVM *) STOP


/* NetIf-generic.cpp */

int NetIfCreateHostOnlyNetworkInterface(VirtualBox *, IHostNetworkInterface **,
                                        IProgress **, const char *) STOP


/* systemmem-freebsd.cpp */

#include "iprt/system.h"

RTDECL(int) RTSystemQueryTotalRam(uint64_t *pcb) STOP


/* HostDnsServiceResolvConf.cpp */

#include "HostDnsService.h"

HostDnsServiceResolvConf::~HostDnsServiceResolvConf() { }

HRESULT HostDnsServiceResolvConf::init(HostDnsMonitorProxy*, char const*) TRACE(VINF_SUCCESS)
void    HostDnsServiceResolvConf::uninit() STOP


/* HostVideoInputDeviceImpl.cpp */

#include "HostVideoInputDeviceImpl.h"

using VideoDeviceList =
	std::__cxx11::list<ComObjPtr<HostVideoInputDevice>,
	                   std::allocator<ComObjPtr<HostVideoInputDevice> > >;

HRESULT HostVideoInputDevice::queryHostDevices(VirtualBox*, VideoDeviceList *) STOP


/* DhcpOptions.cpp */

#undef LOG_GROUP
#include "Dhcpd/DhcpOptions.h"

DhcpOption *DhcpOption::parse(unsigned char, int, char const*, int*) STOP


/* AutostartDb-generic.cpp */

#include "AutostartDb.h"

int AutostartDb::addAutostartVM   (char const *) STOP
int AutostartDb::addAutostopVM    (char const *) STOP
int AutostartDb::removeAutostopVM (char const *) STOP
int AutostartDb::removeAutostartVM(char const *)  STOP

AutostartDb::AutostartDb() TRACE()
AutostartDb::~AutostartDb() { }
int AutostartDb::setAutostartDbPath(char const*) TRACE(VINF_SUCCESS)

RT_C_DECLS_BEGIN

static_assert(sizeof(RTR0PTR) == sizeof(RTR3PTR), "pointer transformation bug");
static_assert(sizeof(RTR0PTR) == sizeof(void *) , "pointer transformation bug");
static_assert(sizeof(RTR3PTR) == sizeof(RTR0PTR), "pointer transformation bug");

RTR0PTR MMHyperR3ToR0(PVM pVM, RTR3PTR R3Ptr) { return (RTR0PTR)R3Ptr; }
RTRCPTR MMHyperR3ToRC(PVM pVM, RTR3PTR R3Ptr) { return to_rtrcptr(R3Ptr); }
RTR0PTR MMHyperCCToR0(PVM pVM, void *pv)      { return (RTR0PTR)pv; }
RTRCPTR MMHyperCCToRC(PVM pVM, void *pv)      { return to_rtrcptr(pv); }
RTR3PTR MMHyperR0ToR3(PVM pVM, RTR0PTR R0Ptr) { return (RTR3PTR*)(R0Ptr | 0UL); }
RTR3PTR MMHyperRCToR3(PVM pVM, RTRCPTR RCPtr)
{
	static_assert(sizeof(RCPtr) <= sizeof(RTR3PTR), "ptr transformation bug");
	return reinterpret_cast<RTR3PTR>(0UL | RCPtr);
}

int  pgmR3InitSavedState(PVM, uint64_t) TRACE(VINF_SUCCESS)
int  emR3InitDbg(PVM)                   TRACE(VINF_SUCCESS)
int  SELMR3Init(PVM)                    TRACE(VINF_SUCCESS)
int  SELMR3Term(PVM)                    TRACE(VINF_SUCCESS)
void SELMR3Relocate(PVM)                TRACE()
void SELMR3Reset(PVM)                   TRACE()

/* module loader of pluggable device manager */
int  pdmR3LdrInitU(PUVM)                            TRACE(VINF_SUCCESS)
int  PDMR3LdrLoadVMMR0U(PUVM)                       TRACE(VINF_SUCCESS)
void PDMR3LdrRelocateU(PUVM, RTGCINTPTR)            TRACE()
int  pdmR3LoadR3U(PUVM, const char *, const char *) TRACE(VINF_SUCCESS)
void pdmR3LdrTermU(PUVM)                            TRACE()

char *pdmR3FileR3(const char * file, bool)
{
	char * pv = reinterpret_cast<char *>(RTMemTmpAllocZ(1));

	if (debug)
		Genode::log(__func__, ": file ", file, " ", (void *)pv, " ", __builtin_return_address(0));

	TRACE(pv)
}

const char * RTBldCfgRevisionStr(void)
{
	return "Genode";
}

DECLHIDDEN(int) rtProcInitExePath(char *pszPath, size_t cchPath)
{
	Genode::copy_cstring(pszPath, "/undefined_ProcInitExePath", cchPath);

	return VINF_SUCCESS;
}

RT_C_DECLS_END


/* HostHardwareLinux.cpp */

#include "HostHardwareLinux.h"

int VBoxMainDriveInfo::updateDVDs() TRACE(VINF_SUCCESS)


/* buildconfig.cpp */

#include <iprt/buildconfig.h>

uint32_t RTBldCfgRevision(void)     { return ~0; }
uint32_t RTBldCfgVersionBuild(void) { return ~0; }
uint32_t RTBldCfgVersionMajor(void) { return ~0; }
uint32_t RTBldCfgVersionMinor(void) { return ~0; }


/* VDIfTcpNet.cpp */

VBOXDDU_DECL(int) VDIfTcpNetInstDefaultCreate(PVDIFINST, PVDINTERFACE *) TRACE(VINF_SUCCESS)


/* SharedFolderImpl.cpp */

#include <SharedFolderImpl.h>

HRESULT SharedFolder::init(Console*, com::Utf8Str const&, com::Utf8Str const&,
                           bool, bool, com::Utf8Str const&, bool) TRACE(E_FAIL)


/* DBGFBp.cpp */

#include <DBGFInternal.h>

int dbgfR3BpInit(VM*) STOP


/* DBGFR3BugCheck.cpp */

int dbgfR3BugCheckInit(VM*) STOP
