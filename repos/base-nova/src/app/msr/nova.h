#pragma once

namespace Nova_msr {

	typedef Genode::addr_t  mword_t;
	typedef Genode::uint8_t uint8_t;

	enum Syscall { NOVA_EC_CTRL = 9 };
	enum Ec_op   { EC_MSR = 6U };

	enum {
		SM_MSR            = 0x20,  /* convention on Genode */
	};

	inline mword_t rdi(Syscall s, uint8_t flags, mword_t sel)
	{
		return sel << 8 | (flags & 0xf) << 4 | s;
	}

	inline uint8_t syscall_2(Syscall s, uint8_t flags, mword_t sel, mword_t p1,
	                         mword_t p2)
	{
		mword_t status = rdi(s, flags, sel);

		asm volatile ("syscall"
		              : "+D" (status)
		              : "S" (p1), "d" (p2)
		              : "rcx", "r11", "memory");
		return (uint8_t)status;
	}

	inline uint8_t ec_ctrl(Ec_op op, mword_t ec = ~0UL, mword_t para = ~0UL,
	                       unsigned crd = 0)
	{
		return syscall_2(NOVA_EC_CTRL, op, ec, para, crd);
	}

	inline uint8_t msr() { return ec_ctrl(EC_MSR, Nova_msr::SM_MSR); }
}

namespace Nova {
	typedef Genode::addr_t  mword_t;

	enum Status
	{
		NOVA_OK = 0,
	};

	struct Utcb {
		mword_t items;
		mword_t crd_xlt;   /* receive capability-range descriptor for translation */
		mword_t crd_rcv;   /* receive capability-range descriptor for delegation */
		mword_t tls;

		mword_t mr[(4096 - 4 * sizeof(mword_t)) / sizeof(mword_t)];

		mword_t * msg() { return mr; }

		void set_msg_word(unsigned num) { items = num; }
		unsigned msg_words() { return items & 0xffffU; }
	};
}
