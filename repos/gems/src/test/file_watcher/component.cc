/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <base/component.h>
#include <os/vfs.h>
#include <timer_session/connection.h>


namespace File_watcher
{
	using namespace Genode;

	class Component;
}


class File_watcher::Component final
{
	private:

		Env&                       _env;
		Heap                       _heap           { _env.ram(), _env.rm() };
		Attached_rom_dataspace     _config         { _env, "config" };
		Root_directory             _vfs            { _env, _heap, _config.xml().sub_node("vfs") };

		Timer::Connection          _timer          { _env };
		Signal_handler<Component>  _timer_handler  { _env.ep(), *this, &Component::_handle_timer };

	private:

		void _handle_timer()
		{
			log("exiting");
			_env.parent().exit(-3);
		}

	public:

		Component(Env &env)
		:
			_env { env }
		{
			log("started");

			_timer.sigh(_timer_handler);
			_timer.trigger_once(150 * 1000);
		}
};


void Component::construct(Genode::Env &env)
{
	static File_watcher::Component component { env };
}
