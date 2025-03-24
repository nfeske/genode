/*
 * \brief  Common types used by the block tester
 * \author Norman Feske
 * \date   2025-03-25
 */

/*
 * Copyright (C) 2016-2025 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _TYPES_H_
#define _TYPES_H_

/* Genode includes */
#include <base/heap.h>
#include <base/log.h>

namespace Test {

	using namespace Genode;

	struct Scenario;

	struct Stats
	{
		uint64_t rx, tx;
		size_t   bytes;
		unsigned completed;
		unsigned job_cnt;
	};

	using block_number_t = Block::block_number_t;
}


struct Test::Scenario : Interface, private Fifo<Scenario>::Element
{
	friend class Fifo<Scenario>;

	struct Attr
	{
		size_t   io_buffer;
		uint64_t progress_interval;
		size_t   batch;
		bool     copy;
		bool     verbose;

		static Attr from_xml(Xml_node const &node)
		{
			return {
				.io_buffer         = node.attribute_value("io_buffer", Number_of_bytes(4*1024*1024)),
				.progress_interval = node.attribute_value("progress", (uint64_t)0),
				.batch             = node.attribute_value("batch",   1u),
				.copy              = node.attribute_value("copy",    true),
				.verbose           = node.attribute_value("verbose", false),
			};
		}
	};

	Attr const attr;

	Scenario(Xml_node const &node) : attr(Attr::from_xml(node)) { }

	struct Init_attr
	{
		size_t         block_size;        /* size of one block in bytes */
		block_number_t block_count;       /* number of blocks */
		size_t         scratch_buffer_size;
	};

	[[nodiscard]] virtual bool init(Init_attr const &) = 0;

	struct No_job { };
	using Next_job_result = Attempt<Block::Operation, No_job>;
	virtual Next_job_result next_job(Stats const &) = 0;

	virtual size_t request_size() const = 0;
	virtual char const *name() const = 0;
	virtual void print(Output &) const = 0;
};

#endif /* _TYPES_H_ */
