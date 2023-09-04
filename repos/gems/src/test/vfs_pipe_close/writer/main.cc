/*
 * \brief  Test for abrupt closing a named pipe
 * \author Norman Feske
 * \date   2023-09-04
 */

/*
 * Copyright (C) 2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/heap.h>
#include <base/attached_rom_dataspace.h>
#include <os/vfs.h>
#include <timer_session/connection.h>

namespace Writer {

	using namespace Genode;

	struct Main;
}


struct Writer::Main
{
	Env &_env;

	Timer::Connection _timer { _env };

	Heap _heap { _env.ram(), _env.rm() };

	Attached_rom_dataspace _config { _env, "config" };

	Vfs::Simple_env _vfs_env { _env, _heap, _config.xml().sub_node("vfs") };

	Directory _root_dir { _vfs_env };

	Main(Env &env) : _env(env)
	{
		Append_file pipe { _root_dir, "named" };

		while (1) {

			auto write = [&] (char const *cstring)
			{
				pipe.append(Const_byte_range_ptr { cstring, strlen(cstring) });
			};

			_timer.msleep(100);

			write("test");

			_vfs_env.io().commit_and_wait();
		}
	}
};


void Component::construct(Genode::Env &env)
{
	static Writer::Main main(env);
}

