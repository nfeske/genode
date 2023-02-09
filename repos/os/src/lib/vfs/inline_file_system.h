/*
 * \brief  Inline filesystem
 * \author Norman Feske
 * \date   2014-04-14
 *
 * This file system allows the content of a file being specified as the content
 * of its config node.
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__VFS__INLINE_FILE_SYSTEM_H_
#define _INCLUDE__VFS__INLINE_FILE_SYSTEM_H_

#include <vfs/file_system.h>

namespace Vfs { class Inline_file_system; }


class Vfs::Inline_file_system : public Single_file_system
{
	private:

		Xml_node _node;

		class Inline_vfs_handle : public Single_vfs_handle
		{
			private:

				Const_byte_range_ptr const &_data_ptr;

				/*
				 * Noncopyable
				 */
				Inline_vfs_handle(Inline_vfs_handle const &);
				Inline_vfs_handle &operator = (Inline_vfs_handle const &);

			public:

				Inline_vfs_handle(Directory_service       &ds,
				               File_io_service            &fs,
				               Genode::Allocator          &alloc,
				               Const_byte_range_ptr const &data_ptr)
				:
					Single_vfs_handle(ds, fs, alloc, 0), _data_ptr(data_ptr)
				{ }

				Read_result read(Byte_range_ptr const &dst, size_t &out_count) override
				{
					/* file read limit is the size of the dataspace */
					size_t const max_size = _data_ptr.num_bytes;

					/* current read offset */
					size_t const read_offset = size_t(seek());

					/* maximum read offset, clamped to dataspace size */
					size_t const end_offset = min(dst.num_bytes + read_offset, max_size);

					/* source address within the dataspace */
					char const *src = _data_ptr.start + read_offset;

					/* check if end of file is reached */
					if (read_offset >= end_offset) {
						out_count = 0;
						return READ_OK;
					}

					/* copy-out bytes from ROM dataspace */
					size_t const num_bytes = (size_t)(end_offset - read_offset);

					memcpy(dst.start, src, num_bytes);

					out_count = num_bytes;
					return READ_OK;
				}

				Write_result write(Const_byte_range_ptr const &,
				                   size_t &out_count) override
				{
					out_count = 0;
					return WRITE_ERR_INVALID;
				}

				bool read_ready()  const override { return true; }
				bool write_ready() const override { return false; }
		};

	public:

		/**
		 * Constructor
		 *
		 * The 'config' XML node (that points to its content) is stored within
		 * the object after construction time. The underlying backing store
		 * must be kept in tact during the lifefile of the object.
		 */
		Inline_file_system(Vfs::Env&, Genode::Xml_node config)
		:
			Single_file_system(Node_type::CONTINUOUS_FILE, name(),
			                   Node_rwx::rx(), config),
			_node(config)
		{ }

		static char const *name()   { return "inline"; }
		char const *type() override { return "inline"; }

		/********************************
		 ** Directory service interface **
		 ********************************/

		Open_result open(char const  *path, unsigned,
		                 Vfs_handle **out_handle,
		                 Allocator   &alloc) override
		{
			if (!_single_file(path))
				return OPEN_ERR_UNACCESSIBLE;

			/* empty node */
			if (_node.content_size() == 0) {
				*out_handle = new (alloc)
					Inline_vfs_handle(*this, *this, alloc,
					                  Const_byte_range_ptr(nullptr, 0));
				return OPEN_OK;
			}

			try {
				_node.with_raw_content([&] (char const *base, size_t size) {
					*out_handle = new (alloc)
						Inline_vfs_handle(*this, *this, alloc,
						                  Const_byte_range_ptr(base, size));
				});
				return OPEN_OK;
			}
			catch (Genode::Out_of_ram)  { return OPEN_ERR_OUT_OF_RAM; }
			catch (Genode::Out_of_caps) { return OPEN_ERR_OUT_OF_CAPS; }
		}

		Stat_result stat(char const *path, Stat &out) override
		{
			Stat_result const result = Single_file_system::stat(path, out);

			_node.with_raw_content([&] (char const *, size_t size) {
				out.size = size; });

			return result;
		}
};

#endif /* _INCLUDE__VFS__INLINE_FILE_SYSTEM_H_ */
