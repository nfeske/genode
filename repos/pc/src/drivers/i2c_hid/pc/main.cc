/*
 * \brief  I2C HID driver for PC-integrated devices
 * \author Christian Helmuth
 * \date   2022-05-02
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

/* Genode includes */
#include <base/component.h>
#include <base/env.h>
#include <genode_c_api/event.h>

/* Lx_kit/emul includes */
#include <lx_emul/init.h>
#include <lx_kit/init.h>
#include <lx_kit/env.h>

using namespace Genode;


struct Main : private Entrypoint::Io_progress_handler
{
	Env &env;

	Main(Env & env);

	/**
	 * Entrypoint::Io_progress_handler
	 */
	void handle_io_progress() override { /* FIXME is this required ? */ }
};

Main::Main(Env &env) : env(env)
{
	/* TODO apply config */

	Lx_kit::initialize(env);
	env.exec_static_constructors();

	/* init C API */
	genode_event_init(genode_env_ptr(env),
	                  genode_allocator_ptr(Lx_kit::env().heap));

	lx_emul_start_kernel(nullptr);
}


void Component::construct(Env & env) { static Main main(env); }
