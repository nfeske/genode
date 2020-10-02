#ifndef ____H_MACROS
#define ____H_MACROS

#include <base/log.h>

#define TRACE(X) \
	{ \
		if (debug) \
			Genode::log(__func__, " called (", __FILE__, ") - eip=", \
			            __builtin_return_address(0)); \
		return X; \
	}

#define DUMMY() \
	{ \
		Genode::error(__func__, " called (", __FILE__, ":", __LINE__, "), " \
		              "not implemented, eip=", \
		              __builtin_return_address(0)); \
		while (1) \
			asm volatile ("ud2a"); \
		\
		class Not_implemented { }; \
		throw Not_implemented();  \
	}

#define DUMMY_STATIC(X) \
	{ \
		static X dummy; \
		Genode::error("static ", __func__, " called (", __FILE__, "), " \
		              "not implemented, eip=", \
		              __builtin_return_address(0)); \
		while (1) \
			asm volatile ("ud2a"); \
		\
		return dummy; \
	}

#endif /* ____H_MACROS */
