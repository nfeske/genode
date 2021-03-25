/*
 * \brief  I2C driver base class to be implemented by platform specific 
 * \author Jean-Adrien Domage <jean-adrien.domage@gapfruit.com>
 * \date   2021-02-08
 */

/*
 * Copyright (C) 2013-2021 Genode Labs GmbH
 * Copyright (C) 2021 gapfruit AG
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _I2C_DRIVER__INTERFACE_H_
#define _I2C_DRIVER__INTERFACE_H_

/* Genode includes */
#include <base/env.h>
#include <util/xml_node.h>

namespace I2c {
	using namespace Genode;
	using Device_name = String<64>;
	class Driver_base;
}


/**
 * Base class for platform specific driver to implement
 *
 * Note about the endianess: the driver is transparent.
 *
 * The driver read/write bytes to memory in the order they are
 * read/write to the bus.
 * It is the responsability of the component interacting with
 * a slave device on the bus to figure out how to interpret the data. 
 */
class I2c::Driver_base
{
	protected:

		Env           &_env;
		Xml_node const _config;

	public:

		class Bad_bus_no: Exception {};

		/**
		 * The ctor the Driver_base saves the env and config for you.
		 *
		 * Notes: your specific platform driver MUST have the same ctor signature. You can 
		 * have a look at src/drivers/i2c/main.cc to see how it is meant to be used.
		 */
		Driver_base(Env &env, Xml_node const &config)
		:
			_env(env), _config(config)
		{ }

		virtual ~Driver_base() = default;

		/**
		 * Write to the I2C bus
		 *
		 * \param address     device address
		 * \param buffer_in   buffer containing data to be send
		 * \param buffer_size size of the buffer to be send
		 *
		 * \throw I2c::Session::Bus_error An error occured while performing an operation on the bus
		 */
		virtual void write(uint8_t address, uint8_t const *buffer_in, size_t buffer_size) = 0;

		/**
		 * Read from the I2C bus
		 *
		 * \param address     device address
		 * \param buffer_out  preallocated buffer to store the data in
		 * \param buffer_size size of the buffer and to be read
		 *
		 * \throw I2c::Session::Bus_error An error occure while performing an operation on the bus
		 */
		virtual void read(uint8_t address, uint8_t *buffer_out, size_t buffer_size) = 0;

		/**
		 * Driver name getter
		 *
		 * \return Driver name string
		 *
		 * Details this method is overridable to customise the name based on the platform.
		 */
		virtual char const *name() const { return "i2c driver"; }
};

#endif /* _I2C_INTERFACE_H_ */
