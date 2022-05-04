/*
 * \brief  Linux's 'vmap' using a managed dataspace
 * \author Sebastian Sumpf
 * \date   2022-05-31
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#ifdef __cplusplus
extern "C" {
#endif
	void *lx_vmap_create(unsigned size);
	void  lx_vmap_destroy(void *vmap);
	int   lx_vmap_attach(void *vmap, void *virt_addr, unsigned long offset);
	void *lx_vmap_address(void *vmap);
#ifdef __cplusplus
}
#endif

