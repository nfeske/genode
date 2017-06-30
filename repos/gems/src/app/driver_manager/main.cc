/*
 * \brief  Driver manager
 * \author Norman Feske
 * \date   2017-06-13
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/heap.h>
#include <base/registry.h>
#include <base/attached_rom_dataspace.h>
#include <os/reporter.h>
#include <block_session/block_session.h>
#include <framebuffer_session/framebuffer_session.h>
#include <io_mem_session/io_mem_session.h>
#include <io_port_session/io_port_session.h>
#include <timer_session/timer_session.h>
#include <log_session/log_session.h>
#include <usb_session/usb_session.h>
#include <platform_session/platform_session.h>

namespace Driver_manager {
	using namespace Genode;
	struct Main;
	struct Block_devices_generator;
	struct Device_driver;
	struct Usb_block_driver;
	struct Intel_fb_driver;
	struct Vesa_fb_driver;
	struct Ahci_driver;
}


struct Driver_manager::Block_devices_generator
{
	virtual void generate_block_devices() = 0;
};


class Driver_manager::Device_driver : Noncopyable
{
	public:

		typedef String<64>  Name;
		typedef String<100> Binary;
		typedef String<32>  Service;

	protected:

		static void _gen_common_start_node_content(Xml_generator &xml,
		                                           Name    const &name,
		                                           Binary  const &binary,
		                                           Ram_quota ram, Cap_quota caps)
		{
			xml.attribute("name", name);
			xml.attribute("caps", String<64>(caps));
			xml.node("binary", [&] () { xml.attribute("name", binary); });
			xml.node("resource", [&] () {
				xml.attribute("name", "RAM");
				xml.attribute("quantum", String<64>(ram));
			});
		}

		template <typename SESSION>
		static void _gen_provides_node(Xml_generator &xml)
		{
			xml.node("provides", [&] () {
				xml.node("service", [&] () {
					xml.attribute("name", SESSION::service_name()); }); });
		}

		static void _gen_config_route(Xml_generator &xml, char const *config_name)
		{
			xml.node("service", [&] () {
				xml.attribute("name", Rom_session::service_name());
				xml.attribute("label", "config");
				xml.node("parent", [&] () {
					xml.attribute("label", config_name); });
			});
		}

		static void _gen_default_parent_route(Xml_generator &xml)
		{
			xml.node("any-service", [&] () {
				xml.node("parent", [&] () { }); });
		}

		template <typename SESSION>
		static void _gen_forwarded_service(Xml_generator &xml,
		                                   Device_driver::Name const &name)
		{
			xml.node("service", [&] () {
				xml.attribute("name", SESSION::service_name());
				xml.node("default-policy", [&] () {
					xml.node("child", [&] () {
						xml.attribute("name", name);
					});
				});
			});
		};

		virtual ~Device_driver() { }

	public:

		virtual void generate_start_node(Xml_generator &xml) const = 0;
};


struct Driver_manager::Usb_block_driver : Device_driver
{
	Env &_env;
	Block_devices_generator &_block_devices_generator;

	typedef String<64> Label;

	Label         const _label;
	unsigned long const _bus;
	unsigned long const _dev;

	enum State { INITIAL, STARTED, AVAILABLE };

	State _state = INITIAL;

	Constructible<Attached_rom_dataspace> _devices;

	void _handle_devices_update()
	{
		_devices->update();
		_block_devices_generator.generate_block_devices();
	}

	Signal_handler<Usb_block_driver> _devices_update_handler {
		_env.ep(), *this, &Usb_block_driver::_handle_devices_update };

	Usb_block_driver(Env &env, Block_devices_generator &block_devices_generator,
	                 Xml_node device)
	:
		_env(env), _block_devices_generator(block_devices_generator),
		_label(device.attribute_value("label", Label())),
		_bus(device.attribute_value("bus", 0UL)),
		_dev(device.attribute_value("dev", 0UL))
	{ }

	String<128> _prefixed_label() const { return String<128>("dynamic -> ", _label); }

	bool has_label(Label const &label) const { return _label == label; }

	bool initial_state() const { return _state == INITIAL; }

	void started()
	{
		_devices.construct(_env, String<128>("dynamic/", _label).string());
		_devices->sigh(_devices_update_handler);
		_state = STARTED;
	}

	bool matches_policy(Xml_node policy) const
	{
		return policy.attribute_value("label", Label()) == _prefixed_label()
		    && policy.attribute_value("bus",   0UL)     == _bus
		    && policy.attribute_value("dev",   0UL)     == _dev;
	}

	void generate_start_node(Xml_generator &xml) const override
	{
		/* don't start the driver before the USB policy is configured */
		if (initial_state())
			return;

		xml.node("start", [&] () {
			_gen_common_start_node_content(xml, _label, "usb_block_drv",
			                               Ram_quota{3*1024*1024}, Cap_quota{100});
			_gen_provides_node<Block::Session>(xml);
			xml.node("config", [&] () {
				xml.attribute("report",    "yes");
				xml.attribute("writeable", "yes");
			});
			xml.node("route", [&] () {
				xml.node("service", [&] () {
					xml.attribute("name", "Usb");
					xml.node("parent", [&] () { xml.attribute("label", _label); });
				});
				xml.node("service", [&] () {
					xml.attribute("name", "Report");
					xml.node("parent", [&] () { xml.attribute("label", _label); });
				});
				_gen_default_parent_route(xml);
			});
		});
	}

	void generate_block_service_forwarding_policy(Xml_generator &xml) const
	{
		xml.node("policy", [&] () {
			xml.attribute("label_suffix", String<64>(" ", _label));
			xml.node("child", [&] () { xml.attribute("name", _label); });
		});
	}

	void generate_usb_driver_policy(Xml_generator &xml) const
	{
		xml.node("policy", [&] () {
			xml.attribute("label", _prefixed_label());
			xml.attribute("bus",   _bus);
			xml.attribute("dev",   _dev);
		});
	}

	void generate_block_devices(Xml_generator &xml) const
	{
		if (!_devices.constructed())
			return;

		_devices->xml().for_each_sub_node("device", [&] (Xml_node device) {
			xml.node("device", [&] () {

				unsigned long const
					block_count = device.attribute_value("block_count", 0UL),
					block_size  = device.attribute_value("block_size",  0UL);

				typedef String<64> Info;
				Info const vendor  = device.attribute_value("vendor",  Info()),
				           product = device.attribute_value("product", Info());

				xml.attribute("label",       _label);
				xml.attribute("block_count", block_count);
				xml.attribute("block_size",  block_size);
				xml.attribute("vendor",      vendor);
				xml.attribute("product",     product);
			});
		});
	}
};


struct Driver_manager::Intel_fb_driver : Device_driver
{
	void generate_start_node(Xml_generator &xml) const override
	{
		xml.node("start", [&] () {
			_gen_common_start_node_content(xml, "intel_fb_drv", "intel_fb_drv",
			                               Ram_quota{20*1024*1024}, Cap_quota{100});
			_gen_provides_node<Framebuffer::Session>(xml);
			xml.node("route", [&] () {
				_gen_config_route(xml, "fb_drv.config");
				_gen_default_parent_route(xml);
			});
		});
		_gen_forwarded_service<Framebuffer::Session>(xml, "intel_fb_drv");
	}
};


struct Driver_manager::Vesa_fb_driver : Device_driver
{
	void generate_start_node(Xml_generator &xml) const override
	{
		xml.node("start", [&] () {
			_gen_common_start_node_content(xml, "vesa_fb_drv", "fb_drv",
			                               Ram_quota{8*1024*1024}, Cap_quota{100});
			_gen_provides_node<Framebuffer::Session>(xml);
			xml.node("route", [&] () {
				_gen_config_route(xml, "fb_drv.config");
				_gen_default_parent_route(xml);
			});
		});
		_gen_forwarded_service<Framebuffer::Session>(xml, "vesa_fb_drv");
	}
};


struct Driver_manager::Ahci_driver : Device_driver
{
	void generate_start_node(Xml_generator &xml) const override
	{
		xml.node("start", [&] () {
			_gen_common_start_node_content(xml, "ahci_drv", "ahci_drv",
			                               Ram_quota{10*1024*1024}, Cap_quota{100});
			_gen_provides_node<Block::Session>(xml);
			xml.node("config", [&] () {
				xml.node("report", [&] () { xml.attribute("ports", "yes"); });
				for (unsigned i = 0; i < 6; i++) {
					xml.node("policy", [&] () {
						xml.attribute("label_suffix", String<64>(" ahci-", i));
						xml.attribute("device", i);
					});
				}
			});
			xml.node("route", [&] () {
				xml.node("service", [&] () {
					xml.attribute("name", "Report");
					xml.node("parent", [&] () { xml.attribute("label", "ahci_ports"); });
				});
				_gen_default_parent_route(xml);
			});
		});
	}

	void generate_block_service_forwarding_policy(Xml_generator &xml) const
	{
		for (unsigned i = 0; i < 6; i++) {
			xml.node("policy", [&] () {
				xml.attribute("label_suffix", String<64>(" ahci-", i));
				xml.node("child", [&] () {
					xml.attribute("name", "ahci_drv");
				});
			});
		}
	}
};


struct Driver_manager::Main : Block_devices_generator
{
	Env &_env;

	Attached_rom_dataspace _init_state        { _env, "init_state"  };
	Attached_rom_dataspace _usb_devices       { _env, "usb_devices" };
	Attached_rom_dataspace _pci_devices       { _env, "pci_devices" };
	Attached_rom_dataspace _ahci_ports        { _env, "ahci_ports"  };
	Attached_rom_dataspace _usb_active_config { _env, "usb_active_config" };

	Reporter _init_config    { _env, "config", "init.config" };
	Reporter _usb_drv_config { _env, "config", "usb_drv.config" };
	Reporter _block_devices  { _env, "block_devices" };

	Constructible<Intel_fb_driver> _intel_fb_driver;
	Constructible<Vesa_fb_driver>  _vesa_fb_driver;
	Constructible<Ahci_driver>     _ahci_driver;

	Heap _heap { _env.ram(), _env.rm() };

	Registry<Registered<Usb_block_driver> > _usb_block_drivers;

	void _handle_pci_devices_update();

	Signal_handler<Main> _pci_devices_update_handler {
		_env.ep(), *this, &Main::_handle_pci_devices_update };

	void _handle_usb_devices_update();

	Signal_handler<Main> _usb_devices_update_handler {
		_env.ep(), *this, &Main::_handle_usb_devices_update };

	void _handle_usb_active_config_update();

	Signal_handler<Main> _usb_active_config_update_handler {
		_env.ep(), *this, &Main::_handle_usb_active_config_update };

	void _handle_ahci_ports_update();

	Signal_handler<Main> _ahci_ports_update_handler {
		_env.ep(), *this, &Main::_handle_ahci_ports_update };

	static void _gen_parent_service_xml(Xml_generator &xml, char const *name)
	{
		xml.node("service", [&] () { xml.attribute("name", name); });
	};

	void _generate_init_config    (Reporter &) const;
	void _generate_usb_drv_config (Reporter &) const;
	void _generate_block_devices  (Reporter &) const;

	/**
	 * Block_devices_generator interface
	 */
	void generate_block_devices() override { _generate_block_devices(_block_devices); }

	Main(Env &env) : _env(env)
	{
		_init_config.enabled(true);
		_usb_drv_config.enabled(true);
		_block_devices.enabled(true);

		_pci_devices      .sigh(_pci_devices_update_handler);
		_usb_devices      .sigh(_usb_devices_update_handler);
		_ahci_ports       .sigh(_ahci_ports_update_handler);
		_usb_active_config.sigh(_usb_active_config_update_handler);

		_generate_init_config(_init_config);
		_generate_usb_drv_config(_usb_drv_config);

		_handle_pci_devices_update();
		_handle_usb_devices_update();
		_handle_ahci_ports_update();
	}
};


void Driver_manager::Main::_handle_pci_devices_update()
{
	_pci_devices.update();

	bool has_vga            = false;
	bool has_intel_graphics = false;
	bool has_ahci           = false;

	_pci_devices.xml().for_each_sub_node([&] (Xml_node device) {

		uint16_t const vendor_id  = device.attribute_value("vendor_id",  0UL);
		uint16_t const class_code = device.attribute_value("class_code", 0UL) >> 8;

		enum {
			VENDOR_INTEL = 0x8086U,
			CLASS_VGA    = 0x300U,
			CLASS_AHCI   = 0x106U,
		};

		if (class_code == CLASS_VGA)
			has_vga = true;

		if (vendor_id == VENDOR_INTEL && class_code == CLASS_VGA)
			has_intel_graphics = true;

		if (vendor_id == VENDOR_INTEL && class_code == CLASS_AHCI)
			has_ahci = true;
	});

	if (!_intel_fb_driver.constructed() && has_intel_graphics) {
		_intel_fb_driver.construct();
		_vesa_fb_driver.destruct();
		_generate_init_config(_init_config);
	}

	if (!_vesa_fb_driver.constructed() && has_vga && !has_intel_graphics) {
		_intel_fb_driver.destruct();
		_vesa_fb_driver.construct();
		_generate_init_config(_init_config);
	}

	if (!_ahci_driver.constructed() && has_ahci) {
		_ahci_driver.construct();
		_generate_init_config(_init_config);
	}
}


void Driver_manager::Main::_handle_ahci_ports_update()
{
	_ahci_ports.update();
	_generate_block_devices(_block_devices);
}


void Driver_manager::Main::_handle_usb_devices_update()
{
	_usb_devices.update();

	/*
	 * XXX remove drivers for disappeared devices
	 */

	/*
	 * Add drivers for new devices
	 */
	_usb_devices.xml().for_each_sub_node([&] (Xml_node device) {

		unsigned long const class_code = device.attribute_value("class", 0UL);

		enum { USB_CLASS_MASS_STORAGE = 8 };

		if (class_code != USB_CLASS_MASS_STORAGE)
			return;

		typedef Usb_block_driver::Label Label;
		Label const label = device.attribute_value("label", Label());

		bool driver_already_present = false;
		_usb_block_drivers.for_each([&] (Usb_block_driver const &driver) {
			if (driver.has_label(label))
				driver_already_present = true;
		});

		if (!driver_already_present) {
			new (_heap) Registered<Usb_block_driver>(_usb_block_drivers, _env,
			                                         *this, device);
		}
	});

	_generate_usb_drv_config(_usb_drv_config);
}


void Driver_manager::Main::_handle_usb_active_config_update()
{
	_usb_active_config.update();
	Xml_node config = _usb_active_config.xml();

	if (!config.has_sub_node("raw"))
		return;

	bool init_config_needs_update = false;

	config.sub_node("raw").for_each_sub_node("policy", [&] (Xml_node policy) {

		_usb_block_drivers.for_each([&] (Usb_block_driver &driver) {
			if (driver.initial_state() && driver.matches_policy(policy)) {
				driver.started();
				init_config_needs_update = true;
			}
		});
	});

	if (init_config_needs_update)
		_generate_init_config(_init_config);
}


void Driver_manager::Main::_generate_init_config(Reporter &init_config) const
{
	Reporter::Xml_generator xml(init_config, [&] () {

		xml.attribute("verbose", false);

		xml.node("report", [&] () { xml.attribute("child_ram", true); });

		xml.node("parent-provides", [&] () {
			_gen_parent_service_xml(xml, Rom_session::service_name());
			_gen_parent_service_xml(xml, Io_mem_session::service_name());
			_gen_parent_service_xml(xml, Io_port_session::service_name());
			_gen_parent_service_xml(xml, Cpu_session::service_name());
			_gen_parent_service_xml(xml, Pd_session::service_name());
			_gen_parent_service_xml(xml, Log_session::service_name());
			_gen_parent_service_xml(xml, Timer::Session::service_name());
			_gen_parent_service_xml(xml, Platform::Session::service_name());
			_gen_parent_service_xml(xml, Report::Session::service_name());
			_gen_parent_service_xml(xml, Usb::Session::service_name());
		});


		if (_intel_fb_driver.constructed())
			_intel_fb_driver->generate_start_node(xml);

		if (_vesa_fb_driver.constructed())
			_vesa_fb_driver->generate_start_node(xml);

		bool block_driver_present = false;

		if (_ahci_driver.constructed()) {
			_ahci_driver->generate_start_node(xml);
			block_driver_present = true;
		}

		_usb_block_drivers.for_each([&] (Usb_block_driver const &driver) {
			driver.generate_start_node(xml);
			block_driver_present = true;
		});

		/* block-service forwarding rules */
		xml.node("service", [&] () {
			xml.attribute("name", Block::Session::service_name());
			if (_ahci_driver.constructed())
				_ahci_driver->generate_block_service_forwarding_policy(xml);
			_usb_block_drivers.for_each([&] (Usb_block_driver const &driver) {
				driver.generate_block_service_forwarding_policy(xml); });
		});
	});
}


void Driver_manager::Main::_generate_block_devices(Reporter &block_devices) const
{
	Reporter::Xml_generator xml(block_devices, [&] () {

		_ahci_ports.xml().for_each_sub_node([&] (Xml_node ahci_port) {

			xml.node("device", [&] () {

				unsigned long const
					num         = ahci_port.attribute_value("num",         0UL),
					block_count = ahci_port.attribute_value("block_count", 0UL),
					block_size  = ahci_port.attribute_value("block_size",  0UL);

				typedef String<80> Model;
				Model const model = ahci_port.attribute_value("model", Model());

				xml.attribute("label",       String<64>("ahci-", num));
				xml.attribute("block_count", block_count);
				xml.attribute("block_size",  block_size);
				xml.attribute("model",       model);
			});
		});

		_usb_block_drivers.for_each([&] (Usb_block_driver const &driver) {
			driver.generate_block_devices(xml); });
	});
}


void Driver_manager::Main::_generate_usb_drv_config(Reporter &usb_drv_config) const
{
	Reporter::Xml_generator xml(usb_drv_config, [&] () {

		xml.attribute("uhci", true);
		xml.attribute("ehci", true);
		xml.attribute("xhci", true);
		xml.node("hid", [&] () { });
		xml.node("raw", [&] () {
			xml.node("report", [&] () { xml.attribute("devices", true); });

			_usb_block_drivers.for_each([&] (Usb_block_driver const &driver) {
				driver.generate_usb_driver_policy(xml);
			});
		});
	});
}


void Component::construct(Genode::Env &env) { static Driver_manager::Main main(env); }
