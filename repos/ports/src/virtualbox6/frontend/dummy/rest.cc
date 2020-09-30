#include "dummy/macros.h"

static bool debug = false;


/* ApplianceImplExport.cpp */

#include "MachineImpl.h"

HRESULT Machine::exportTo(const ComPtr<IAppliance> &aAppliance,
                          const com::Utf8Str &aLocation,
                          ComPtr<IVirtualSystemDescription> &aDescription)      DUMMY()

/* com.cpp */

#include "VBox/com/Guid.h"
#include "VBox/com/array.h"

com::Guid const com::Guid::Empty;

char const com::Zeroes[16] = {0, };

#include "xpcom/nsIServiceManager.h"

nsGetServiceByContractID do_GetService(char const*, unsigned int*) DUMMY()


/* DisplayPNGUtil.cpp */

#include "DisplayImpl.h"

int DisplayMakePNG(uint8_t *, uint32_t, uint32_t, uint8_t **, uint32_t *,
                   uint32_t *, uint32_t *, uint8_t)                             DUMMY()


/* ErrorInfo.cpp */

#include "VBox/com/ErrorInfo.h"

com::ProgressErrorInfo::ProgressErrorInfo(IProgress*)                           DUMMY()


/* EventImpl.cpp */

#include "EventImpl.h"

HRESULT VBoxEventDesc::init(IEventSource* aSource, VBoxEventType_T aType, ...)  TRACE(S_OK)
HRESULT VBoxEventDesc::reinit(VBoxEventType_T aType, ...)                       TRACE(S_OK)


/* initterm.cpp */

#include "VBox/com/com.h"

HRESULT com::Initialize(bool) TRACE(S_OK)
HRESULT com::Shutdown()       DUMMY()


///* ProgressProxyImpl.cpp */
//
//#include "ProgressProxyImpl.h"
//
//STDMETHODIMP ProgressProxy::Cancel()                                            DUMMY()
//void ProgressProxy::clearOtherProgressObjectInternal(bool fEarly)               DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(Cancelable)(BOOL *)                       DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(Percent)(ULONG *)                         DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(TimeRemaining)(LONG *)                    DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(Completed)(BOOL *)                        DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(Canceled)(BOOL *)                         DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(ResultCode)(LONG *)                       DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(ErrorInfo)(IVirtualBoxErrorInfo **)       DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(Operation)(ULONG *)                       DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(OperationDescription)(BSTR *)             DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(OperationPercent)(ULONG *)                DUMMY()
//STDMETHODIMP ProgressProxy::COMSETTER(Timeout)(ULONG)                           DUMMY()
//STDMETHODIMP ProgressProxy::COMGETTER(Timeout)(ULONG *)                         DUMMY()
//void ProgressProxy::copyProgressInfo(IProgress *pOtherProgress, bool fEarly)    DUMMY()
//HRESULT ProgressProxy::FinalConstruct()                                         DUMMY()
//void ProgressProxy::FinalRelease()                                              DUMMY()
//HRESULT ProgressProxy::init(VirtualBox*, IUnknown*, unsigned short const*,
//        bool)                                                                   DUMMY()
//HRESULT ProgressProxy::init(VirtualBox*, void*, unsigned short const*, bool,
//                            unsigned int, unsigned short const*, unsigned int,
//                            unsigned int)                                       DUMMY()
//HRESULT ProgressProxy::notifyComplete(HRESULT)                                  DUMMY()
//HRESULT ProgressProxy::notifyComplete(HRESULT, GUID const&, char const*,
//                                      char const*, ...)                         DUMMY()
//STDMETHODIMP ProgressProxy::SetCurrentOperationProgress(ULONG aPercent)         DUMMY()
//STDMETHODIMP ProgressProxy::SetNextOperation(IN_BSTR, ULONG)                    DUMMY()
//bool    ProgressProxy::setOtherProgressObject(IProgress*)                       DUMMY()
//void ProgressProxy::uninit()                                                    DUMMY()
//STDMETHODIMP ProgressProxy::WaitForCompletion(LONG aTimeout)                    DUMMY()
//STDMETHODIMP ProgressProxy::WaitForOperationCompletion(ULONG, LONG)             DUMMY()
//
//
///* SharedFolderImpl.cpp */
//
//#include "SharedFolderImpl.h"
//
//HRESULT SharedFolder::init(Console*, com::Utf8Str const&, com::Utf8Str const&,
//                           bool, bool, bool)                                    DUMMY()


/* USBFilter.cpp */

#include "VBox/usbfilter.h"

USBFILTERMATCH USBFilterGetMatchingMethod(PCUSBFILTER, USBFILTERIDX)             DUMMY()
int  USBFilterGetNum(PCUSBFILTER pFilter, USBFILTERIDX enmFieldIdx)              DUMMY()
const char * USBFilterGetString(PCUSBFILTER pFilter, USBFILTERIDX enmFieldIdx)   DUMMY()
void USBFilterInit(PUSBFILTER pFilter, USBFILTERTYPE enmType)                    DUMMY()
bool USBFilterIsMethodNumeric(USBFILTERMATCH enmMatchingMethod)                  DUMMY()
bool USBFilterIsMethodString(USBFILTERMATCH enmMatchingMethod)                   DUMMY()
bool USBFilterIsNumericField(USBFILTERIDX enmFieldIdx)                           DUMMY()
bool USBFilterIsStringField(USBFILTERIDX enmFieldIdx)                            DUMMY()
bool USBFilterMatch(PCUSBFILTER pFilter, PCUSBFILTER pDevice)                    DUMMY()
int  USBFilterSetIgnore(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx)            DUMMY()
int  USBFilterSetNumExact(PUSBFILTER, USBFILTERIDX, uint16_t, bool)              DUMMY()
int  USBFilterSetNumExpression(PUSBFILTER, USBFILTERIDX, const char *, bool)     DUMMY()
int  USBFilterSetStringExact(PUSBFILTER, USBFILTERIDX, const char *, bool)       DUMMY()
int  USBFilterSetStringPattern(PUSBFILTER, USBFILTERIDX, const char *, bool)     DUMMY()
int  USBFilterSetStringExact(PUSBFILTER, USBFILTERIDX, const char *, bool, bool) DUMMY()


/* USBProxyBackend.cpp */

#include "USBProxyBackend.h"

USBProxyBackendFreeBSD::USBProxyBackendFreeBSD() DUMMY()

USBProxyBackend::USBProxyBackend() DUMMY()
USBProxyBackend::~USBProxyBackend() { }

HRESULT             USBProxyBackend::FinalConstruct() DUMMY()
com::Utf8Str const &USBProxyBackend::i_getAddress()   DUMMY()
com::Utf8Str const &USBProxyBackend::i_getId()        DUMMY()

USBProxyBackendUsbIp::USBProxyBackendUsbIp() DUMMY()


/* USBProxyService.cpp */

#include "USBProxyService.h"

USBProxyService::USBProxyService(Host* aHost) : mHost(aHost), mDevices(), mBackends() DUMMY()

HRESULT USBProxyService::autoCaptureDevicesForVM(SessionMachine *)           DUMMY()
HRESULT USBProxyService::captureDeviceForVM(SessionMachine *, IN_GUID,
                                            com::Utf8Str const&)             DUMMY()
HRESULT USBProxyService::detachAllDevicesFromVM(SessionMachine*, bool, bool) DUMMY()
HRESULT USBProxyService::detachDeviceFromVM(SessionMachine*, IN_GUID, bool)  DUMMY()
void   *USBProxyService::insertFilter(PCUSBFILTER aFilter)                   DUMMY()
void    USBProxyService::removeFilter(void *aId)                             DUMMY()
int     USBProxyService::getLastError()                                      DUMMY()
bool    USBProxyService::isActive()                                          DUMMY()
HRESULT USBProxyService::removeUSBDeviceSource(com::Utf8Str const&)          DUMMY()
HRESULT USBProxyService::addUSBDeviceSource(com::Utf8Str const&,
                                            com::Utf8Str const&,
                                            com::Utf8Str const&,
                                            std::vector<com::Utf8Str, std::allocator<com::Utf8Str> > const&,
                                            std::vector<com::Utf8Str, std::allocator<com::Utf8Str> > const&) DUMMY()
HRESULT USBProxyService::getDeviceCollection(std::vector<ComPtr<IHostUSBDevice>, std::allocator<ComPtr<IHostUSBDevice> > >&) DUMMY()
HRESULT USBProxyService::i_loadSettings(std::__cxx11::list<settings::USBDeviceSource, std::allocator<settings::USBDeviceSource> > const&) DUMMY()
HRESULT USBProxyService::i_saveSettings(std::__cxx11::list<settings::USBDeviceSource, std::allocator<settings::USBDeviceSource> >&) DUMMY()


/* USBFilter.cpp */

#include "VBox/usbfilter.h"

USBLIB_DECL(USBFILTERTYPE) USBFilterGetFilterType(PCUSBFILTER) DUMMY()
USBLIB_DECL(int)           USBFilterSetFilterType(PUSBFILTER, USBFILTERTYPE) DUMMY()


/* ApplianceImpl.cpp */

HRESULT VirtualBox::createAppliance(ComPtr<IAppliance> &aAppliance) DUMMY()


/* ClientWatcher.cpp */

#include "ClientWatcher.h"

VirtualBox::ClientWatcher::~ClientWatcher() { }

VirtualBox::ClientWatcher::ClientWatcher(const ComObjPtr<VirtualBox> &pVirtualBox)
: mLock(LOCKCLASS_OBJECTSTATE) DUMMY()

bool VirtualBox::ClientWatcher::isReady() DUMMY()
void VirtualBox::ClientWatcher::update()  DUMMY()


/* ClientTokenHolder.cpp */

#include "ClientTokenHolder.h"

Session::ClientTokenHolder::~ClientTokenHolder() { }


/* CloudProviderManagerImpl.cpp */

#include "CloudProviderManagerImpl.h"

CloudProviderManager::CloudProviderManager() DUMMY()
CloudProviderManager::~CloudProviderManager() { }

HRESULT CloudProviderManager::FinalConstruct() DUMMY()
HRESULT CloudProviderManager::init()           DUMMY()
void    CloudProviderManager::uninit()         DUMMY()
HRESULT CloudProviderManager::getProviderById       (com::Guid    const&, ComPtr<ICloudProvider>&) DUMMY()
HRESULT CloudProviderManager::getProviderByName     (com::Utf8Str const&, ComPtr<ICloudProvider>&) DUMMY()
HRESULT CloudProviderManager::getProviderByShortName(com::Utf8Str const&, ComPtr<ICloudProvider>&) DUMMY()
HRESULT CloudProviderManager::getProviders(std::vector<ComPtr<ICloudProvider>,
                                           std::allocator<ComPtr<ICloudProvider> > >&) DUMMY()


/* PerformanceFreeBSD.cpp */

#include "Performance.h"

pm::CollectorHAL *pm::createHAL() DUMMY()


/* NetIf-freebsd.cpp */

#include "HostNetworkInterfaceImpl.h"
#include "netif.h"

int NetIfGetLinkSpeed(const char *, uint32_t *) DUMMY()
int NetIfGetState(const char *, NETIFSTATUS *)  DUMMY()
int NetIfRemoveHostOnlyNetworkInterface(VirtualBox *, const Guid &, IProgress **) DUMMY()
int NetIfList(std::__cxx11::list<ComObjPtr<HostNetworkInterface>,
              std::allocator<ComObjPtr<HostNetworkInterface> > >&) DUMMY()


/* fatvfs.cpp */

#include "iprt/fsvfs.h"

RTDECL(int) RTFsFatVolFormat(RTVFSFILE, uint64_t, uint64_t, uint32_t, uint16_t,
                             uint16_t, RTFSFATTYPE, uint32_t, uint32_t,
                             uint8_t, uint16_t, uint32_t, PRTERRINFO) DUMMY()

/* dvm.cpp */

#include "iprt/dvm.h"

RTDECL(uint32_t) RTDvmRelease(RTDVM)                                DUMMY()
RTDECL(int)      RTDvmCreate(PRTDVM, RTVFSFILE, uint32_t, uint32_t) DUMMY()
RTDECL(int)      RTDvmMapInitialize(RTDVM, const char *)            DUMMY()


/* MachineImplMoveVM.cpp */

#include "MachineImplMoveVM.h"

HRESULT MachineMoveVM::init()                              DUMMY()
void    MachineMoveVM::i_MoveVMThreadTask(MachineMoveVM *) DUMMY()


/* NetIf-generic.cpp */

int NetIfCreateHostOnlyNetworkInterface(VirtualBox *, IHostNetworkInterface **, IProgress **, const char *) DUMMY()


/* systemmem-freebsd.cpp */

#include "iprt/system.h"

RTDECL(int) RTSystemQueryTotalRam(uint64_t *pcb) DUMMY()


/* HostDnsServiceResolvConf.cpp */

#include "HostDnsService.h"

HostDnsServiceResolvConf::~HostDnsServiceResolvConf() { }

HRESULT HostDnsServiceResolvConf::init(HostDnsMonitorProxy*, char const*) DUMMY()
void    HostDnsServiceResolvConf::uninit() DUMMY()


/* HostVideoInputDeviceImpl.cpp */

#include "HostVideoInputDeviceImpl.h"

HRESULT HostVideoInputDevice::queryHostDevices(VirtualBox*, std::__cxx11::list<ComObjPtr<HostVideoInputDevice>,
                                               std::allocator<ComObjPtr<HostVideoInputDevice> > >*) DUMMY()

/* DhcpOptions.cpp */

#undef LOG_GROUP
#include "Dhcpd/DhcpOptions.h"

DhcpOption *DhcpOption::parse(unsigned char, int, char const*, int*) DUMMY()
