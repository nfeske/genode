/*
 * \brief  Port of VirtualBox to Genode
 * \author Norman Feske
 * \author Alexander Boettcher
 * \date   2013-08-20
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */


/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <base/log.h>
#include <libc/component.h>
#include <libc/args.h>

/* Virtualbox includes */
#include <iprt/initterm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <VBox/com/com.h>
#include <VBox/vmm/vmapi.h>

/* Virtualbox includes of generic Main frontend */
#include "ConsoleImpl.h"
#include "MachineImpl.h"
#include "MouseImpl.h"
#include "SessionImpl.h"
#include "VirtualBoxImpl.h"

/* Genode port specific includes */
#include "fb.h"
#include "vmm.h"
#include "sup.h"

static char c_vbox_file[128];
static char c_vbox_vmname[128];

/* initial environment for the FreeBSD libc implementation */
extern char **environ;


int com::GetVBoxUserHomeDirectory(char *aDir, size_t aDirLen, bool fCreateDir)
{
    AssertReturn(aDir, VERR_INVALID_POINTER);
    AssertReturn(aDirLen > 1, VERR_BUFFER_OVERFLOW);

	memcpy(aDir, "/", 1);
	aDir[1] = 0;
	return VINF_SUCCESS;
}

extern "C"
RTDECL(int) RTPathUserHome(char *pszPath, size_t cchPath)
{
	return com::GetVBoxUserHomeDirectory(pszPath, cchPath);
}


extern "C" VirtualBox * genode_global_vbox_pointer;
VirtualBox * genode_global_vbox_pointer = nullptr;

HRESULT setupmachine(Genode::Env &env)
{
	HRESULT rc;

	using Genode::warning;
	using Genode::error;

	static com::Utf8Str vm_config(c_vbox_file);
	static com::Utf8Str vm_name(c_vbox_vmname);

	/* Machine object */
	static ComObjPtr<Machine> machine;
	rc = machine.createObject();
	if (FAILED(rc))
		return rc;

	/*
	 * Create VirtualBox object
	 *
	 * We cannot create the object via 'ComObjPtr<VirtualBox>::createObject'
	 * because 'FinalConstruction' uses a temporary 'ComObjPtr<VirtualBox>'
	 * (implicitely constructed as argument for the 'ClientWatcher' constructor.
	 * Upon the destruction of the temporary, the 'VirtualBox' refcnt becomes
	 * zero, which prompts 'VirtualBox::Release' to destuct the object.
	 *
	 * To sidestep this suicidal behavior, we manually perform the steps of
	 * 'createObject' but calling 'AddRef' before 'FinalConstruct'.
	 */
	VirtualBox *virtualbox_ptr = new VirtualBox();

	virtualbox_ptr->AddRef();

	ComObjPtr<VirtualBox> virtualbox(virtualbox_ptr);
	{
		rc = virtualbox->FinalConstruct();
		if (FAILED(rc)) {
			error("construction of VirtualBox object failed, rc=", rc);
			return rc;
		}
	}

	/*
	 * Used in src-client/ConsoleImpl.cpp, which constructs Progress objects,
	 * which requires the only-one pointer to VirtualBox object
	 */
	genode_global_vbox_pointer = virtualbox;

	rc = machine->initFromSettings(virtualbox, vm_config, nullptr);
	if (FAILED(rc))
		return rc;

	rc = genode_setup_machine(machine);
	if (FAILED(rc))
		return rc;

	/*
	 * Add the machine to th VirtualBox::allMachines list
	 *
	 * Unfortunately, the 'i_registerMachine' function performs a
	 * 'i_saveSettings' should the 'VirtualBox' object not be in the
	 * 'InInit' state. However, the object is already in 'Ready' state.
	 * So, 'i_saveSettings' attempts to write a 'VirtualBox.xml' file
	 */
	{
		AutoWriteLock alock(virtualbox.m_p COMMA_LOCKVAL_SRC_POS);

		rc = machine->i_prepareRegister();
		if (FAILED(rc))
			return rc;
	}

	static ComObjPtr<Session> session;
	rc = session.createObject();
	if (FAILED(rc))
		return rc;

	rc = machine->LockMachine(session, LockType_VM);
	if (FAILED(rc))
		return rc;

	/* Validate configured memory of vbox file and Genode config */
	ULONG memory_vbox;
	rc = machine->COMGETTER(MemorySize)(&memory_vbox);
	if (FAILED(rc))
		return rc;

	static ComPtr<IConsole> gConsole;
	rc = session->COMGETTER(Console)(gConsole.asOutParam());

	static ComPtr<IDisplay> display;
	rc = gConsole->COMGETTER(Display)(display.asOutParam());
	if (FAILED(rc))
		return rc;

	static ComPtr<IGraphicsAdapter> graphics_adapter;
	rc = machine->COMGETTER(GraphicsAdapter)(graphics_adapter.asOutParam());
	if (FAILED(rc))
		return rc;

	PRUint32 cMonitors = 1;
	rc = graphics_adapter->COMGETTER(MonitorCount)(&cMonitors);
	if (FAILED(rc))
		return rc;

	static Gui::Connection gui { env };
	static Bstr gaFramebufferId[64];

	for (unsigned uScreenId = 0; uScreenId < cMonitors; uScreenId++)
	{
		Genodefb *fb = new Genodefb(env, gui, display);
		HRESULT rc = display->AttachFramebuffer(uScreenId, fb, gaFramebufferId[uScreenId].asOutParam());
		if (FAILED(rc))
			return rc;
	}

	/* Power up the VMM */
	ComPtr <IProgress> progress;
	rc = gConsole->PowerUp(progress.asOutParam());
	if (FAILED(rc))
		return rc;

	/* check whether enough memory is available for VM + VMM */
	ULONG const required_memory_vm = (13 * 1024 + 6 * memory_vbox) << 10;
	rc = genode_check_memory_config(machine, required_memory_vm);
	if (FAILED(rc))
		return rc;

	/* wait until VM is up */
	MachineState_T machineState = MachineState_Null;
	do {
		if (machineState != MachineState_Null)
			RTThreadSleep(1000);
warning("setup_machine 19");

		rc = machine->COMGETTER(State)(&machineState);
	} while (machineState == MachineState_Starting);
	if (rc != S_OK || (machineState != MachineState_Running))
		return E_FAIL;
warning("setup_machine 20");

	/* request mouse object */
	static ComPtr<IMouse> gMouse;
	rc = gConsole->COMGETTER(Mouse)(gMouse.asOutParam());
	if (FAILED(rc))
		return rc;
	Assert (&*gMouse);
warning("setup_machine 21");

	/* request keyboard object */
	static ComPtr<IKeyboard> gKeyboard;
	rc = gConsole->COMGETTER(Keyboard)(gKeyboard.asOutParam());
	if (FAILED(rc))
		return rc;
	Assert (&*gKeyboard);
warning("setup_machine 22");

//	genodeConsole->init_backends(gKeyboard, gMouse);

warning("setup_machine 23");
	/* check whether enough memory for the fb is available */
	ULONG required_memory_fb = 0;
	for (unsigned uScreenId = 0; uScreenId < cMonitors; uScreenId++)
	{
		IFramebuffer * aFramebuffer = nullptr;
		HRESULT rc = display->QueryFramebuffer(uScreenId, &aFramebuffer);
		Genodefb * fb = dynamic_cast<Genodefb *>(aFramebuffer);
		if (FAILED(rc) || !fb)
			continue;

		required_memory_fb += fb->w() * fb->h() * 4;
	}
warning("setup_machine 24");
	if (!required_memory_fb)
		required_memory_fb = 4096 * 2160 * 4;
warning("setup_machine 25");

	rc = genode_check_memory_config(machine, required_memory_fb);
	if (FAILED(rc))
		return rc;
warning("setup_machine 26");

	return rc;
}


static Genode::Env *genode_env_ptr = nullptr;


Genode::Env &genode_env()
{
	struct Genode_env_ptr_uninitialized : Genode::Exception { };
	if (!genode_env_ptr)
		throw Genode_env_ptr_uninitialized();

	return *genode_env_ptr;
}

#include <nsXPCOM.h>
#include <nsCOMPtr.h>

Genode::Allocator &vmm_heap()
{
	static Genode::Heap heap (genode_env().ram(), genode_env().rm());
	return heap;
}


void Libc::Component::construct(Libc::Env &env)
{
	/* make Genode environment accessible via the global 'genode_env()' */
	genode_env_ptr = &env;

	{
		using namespace Genode;

		Attached_rom_dataspace config_ds(env, "config");
		Xml_node const config = config_ds.xml();

		if (!config.has_attribute("vbox_file") || !config.has_attribute("vm_name")) {
			error("missing attributes in configuration, minimum requirements: ");
			error("  <config vbox_file=\"...\" vm_name=\"...\">" );
			throw Exception();
		}

		typedef String<128> Name;

		Name const vbox_file = config.attribute_value("vbox_file", Name());
		copy_cstring(c_vbox_file, vbox_file.string(), sizeof(c_vbox_file));

		Name const vm_name = config.attribute_value("vm_name", Name());
		copy_cstring(c_vbox_vmname, vm_name.string(), sizeof(c_vbox_vmname));
	}

	Libc::with_libc([&] () {

		/* extract args and environment variables from config */
		int argc    = 0;
		char **argv = nullptr;
		char **envp = nullptr;
	
		populate_args_and_env(env, argc, argv, envp);
	
		environ = envp;

		/* sidestep 'rtThreadPosixSelectPokeSignal' */
		uint32_t const fFlags = RTR3INIT_FLAGS_UNOBTRUSIVE;

		{
			int const rc = RTR3InitExe(argc, &argv, fFlags);
			if (RT_FAILURE(rc))
				throw -1;
		}

		{
			nsCOMPtr<nsIServiceManager> serviceManager;
			HRESULT const rc = NS_InitXPCOM2(getter_AddRefs(serviceManager), nsnull, nsnull);
			if (NS_FAILED(rc))
			{
				Genode::error("failed to initialize XPCOM, rc=", rc);
				throw -2;
			}
		}

		{
			HRESULT const hrc = setupmachine(env);
			if (FAILED(hrc)) {
				Genode::error("startup of VMM failed - reason ", hrc, " '",
				              RTErrCOMGet(hrc)->pszMsgFull, "' - exiting ...");
				throw -3;
			}
		}
	});
}


NS_IMPL_THREADSAFE_ISUPPORTS1_CI(Genodefb, IFramebuffer)
NS_DECL_CLASSINFO(Genodefb)
