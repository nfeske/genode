/*
 * \brief  Common representation of all storage devices
 * \author Norman Feske
 * \date   2018-05-02
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _STORAGE_DEVICE_H_
#define _STORAGE_DEVICE_H_

#include "types.h"
#include "partition.h"
#include "capacity.h"
#include "xml.h"

namespace Sculpt_manager { struct Storage_device; };


struct Sculpt_manager::Storage_device
{
	enum State {
		UNKNOWN, /* partition information not yet known */
		USED,    /* part_blk is running and has reported partition info */
		RELEASED /* partition info is known but part_blk is not running */
	};

	Allocator &_alloc;

	typedef String<32> Label;

	Label const label;

	Capacity capacity; /* non-const because USB storage devices need to update it */

	State state { UNKNOWN };

	bool whole_device = false;

	Reconstructible<Partition> whole_device_partition {
		Partition::Args::whole_device(capacity) };

	Partitions partitions { };

	Attached_rom_dataspace _partitions_rom;

	unsigned _part_blk_version = 0;

	/**
	 * Trigger the rediscovery of the device, e.g., after partitioning of after
	 * formatting the whole device.
	 */
	void rediscover()
	{
		log("\n\nREDISCOVER device ", label);
		state = UNKNOWN;
		_part_blk_version++;

		Partition_update_policy policy(_alloc);
		partitions.update_from_xml(policy, Xml_node("<partitions/>"));
	}

	void process_part_blk_report()
	{
		_partitions_rom.update();
		log("partition info: ", _partitions_rom.xml());

		Xml_node const report = _partitions_rom.xml();
		if (!report.has_type("partitions"))
			return;

		whole_device = (report.attribute_value("type", String<16>()) == "disk");

		Partition_update_policy policy(_alloc);
		partitions.update_from_xml(policy, report);

		/* import whole-device partition information */
		whole_device_partition.construct(Partition::Args::whole_device(capacity));
		report.for_each_sub_node("partition", [&] (Xml_node partition) {
			if (partition.attribute_value("number", Partition::Number()) == "0")
				whole_device_partition.construct(Partition::Args::from_xml(partition)); });

		/* finish initial discovery phase */
		if (state == UNKNOWN)
			state = RELEASED;
	}

	/**
	 * Constructor
	 *
	 * \param label  label of block device
	 * \param sigh   signal handler to be notified on partition-info updates
	 */
	Storage_device(Env &env, Allocator &alloc, Label const &label,
	               Capacity capacity, Signal_context_capability sigh)
	:
		_alloc(alloc), label(label), capacity(capacity),
		_partitions_rom(env, String<80>("report -> runtime/", label, ".part_blk/partitions").string())
	{
		_partitions_rom.sigh(sigh);
		process_part_blk_report();
	}

	bool part_blk_needed_for_discovery() const
	{
		return state == UNKNOWN;
	}

	bool part_blk_needed_for_access() const
	{
		bool needed_for_access = false;
		partitions.for_each([&] (Partition const &partition) {
			needed_for_access |= partition.check_in_progress;
			needed_for_access |= partition.format_in_progress;
			needed_for_access |= partition.file_system_inspected;
		});

		if (whole_device_partition->format_in_progress
		 || whole_device_partition->check_in_progress) {
			needed_for_access = false;
		}

		return needed_for_access;
	}

	/**
	 * Generate content of start node for part_blk
	 *
	 * \param service_name  name of server that provides the block device, or
	 *                      if invalid, request block device from parent.
	 */
	inline void gen_part_blk_start_content(Xml_generator &xml, Label const &server_name) const;

	template <typename FN>
	void for_each_partition(FN const &fn) const
	{
		fn(*whole_device_partition);
		partitions.for_each([&] (Partition const &partition) { fn(partition); });
	}

	template <typename FN>
	void for_each_partition(FN const &fn)
	{
		fn(*whole_device_partition);
		partitions.for_each([&] (Partition &partition) { fn(partition); });
	}
};


void Sculpt_manager::Storage_device::gen_part_blk_start_content(Xml_generator &xml,
                                                                Label const &server_name) const
{
	xml.attribute("version", _part_blk_version);

	gen_common_start_content(xml, Label(label, ".part_blk"),
	                         Cap_quota{100}, Ram_quota{8*1024*1024});

	gen_named_node(xml, "binary", "part_blk");

	xml.node("config", [&] () {
		xml.node("report", [&] () { xml.attribute("partitions", "yes"); });

		for (unsigned i = 1; i < 10; i++) {
			xml.node("policy", [&] () {
				xml.attribute("label",     i);
				xml.attribute("partition", i);
				xml.attribute("writeable", "yes");
			});
		}
	});

	gen_provides<Block::Session>(xml);

	xml.node("route", [&] () {
		gen_parent_rom_route(xml, "part_blk");
		gen_parent_rom_route(xml, "ld.lib.so");
		gen_parent_route<Cpu_session> (xml);
		gen_parent_route<Pd_session>  (xml);
		gen_parent_route<Log_session> (xml);

		gen_service_node<Block::Session>(xml, [&] () {
			if (server_name.valid())
				gen_named_node(xml, "child", server_name);
			else
				xml.node("parent", [&] () {
					xml.attribute("label", label); }); });

		gen_service_node<Report::Session>(xml, [&] () {
			xml.attribute("label", "partitions");
			xml.node("parent", [&] () { }); });
	});
}

#endif /* _STORAGE_DEVICE_H_ */
