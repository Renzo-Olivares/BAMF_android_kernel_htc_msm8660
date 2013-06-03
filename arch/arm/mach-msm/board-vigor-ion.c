/* linux/arch/arm/mach-msm/board-pyramid-ion.c
 *
 * Copyright (c) 2013 Sebastian Sobczyk <sebastiansobczyk@wp.pl>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <mach/msm_memtypes.h>
#include <linux/ion.h>
#include <mach/ion.h>
#include <mach/board.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

extern void pmem_request_smi_region(void *data);
extern void pmem_release_smi_region(void *data);
extern void *pmem_setup_smi_region(void);

// #define MSM_ION_CAMERA_BASE   0x40E00000
 #define MSM_ION_AUDIO_BASE    0x6FB00000
// 
// #define MSM_ION_ROTATOR_SIZE  0x654000
// #define MSM_ION_MM_FW_SIZE    0x200000
// #define MSM_ION_MM_SIZE       0x3D00000
// #define MSM_ION_MFC_SIZE      0x100000
// #define MSM_ION_CAMERA_SIZE   0x2000000
// #define MSM_ION_SF_SIZE       0x4000000
// #define MSM_ION_AUDIO_SIZE	  0x4CF000
// 
// #ifdef CONFIG_TZCOM
// #define MSM_ION_QSECOM_SIZE   0xC7000
// #define MSM_ION_HEAP_NUM      9
// #else
// #define MSM_ION_HEAP_NUM      8
// #endif

static int request_smi_region(void *data)
{
	pmem_request_smi_region(data);
	return 0;
}

static int release_smi_region(void *data)
{
	pmem_release_smi_region(data);
	return 0;
}

static struct ion_cp_heap_pdata cp_mm_ion_pdata = {
	.permission_type = IPT_TYPE_MM_CARVEOUT,
	.align = PAGE_SIZE,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = pmem_setup_smi_region,
};

static struct ion_cp_heap_pdata cp_mfc_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = pmem_setup_smi_region,
};

static struct ion_co_heap_pdata fw_co_ion_pdata = {
	.adjacent_mem_id = ION_CP_MM_HEAP_ID,
	.align = SZ_128K,
};

static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};

static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
		{
			.id	= ION_CP_MM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MM_HEAP_NAME,
			.size	= MSM_ION_MM_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &cp_mm_ion_pdata,
		},
		{
			.id	= ION_MM_FIRMWARE_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_MM_FIRMWARE_HEAP_NAME,
			.size	= MSM_ION_MM_FW_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &fw_co_ion_pdata,
		},
		{
			.id	= ION_CP_MFC_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MFC_HEAP_NAME,
			.size	= MSM_ION_MFC_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &cp_mfc_ion_pdata,
		},
#ifdef CONFIG_TZCOM
		{
			.id	= ION_QSECOM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_QSECOM_HEAP_NAME,
			.size	= MSM_ION_QSECOM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
#endif
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.size	= MSM_ION_SF_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
		{
			.id	= ION_CP_ROTATOR_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_ROTATOR_HEAP_NAME,
			.size	= MSM_ION_ROTATOR_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = &co_ion_pdata,
		},
		{
			.id	= ION_CAMERA_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_CAMERA_HEAP_NAME,
			.base	= MSM_ION_CAMERA_BASE,
			.size	= MSM_ION_CAMERA_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = &co_ion_pdata,
		},
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.base	= MSM_ION_AUDIO_BASE,
			.size	= MSM_ION_AUDIO_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
	}
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};

int __init vigor_ion_reserve_memory(struct memtype_reserve *table) {
	table[MEMTYPE_SMI].size += MSM_ION_MM_FW_SIZE;
	table[MEMTYPE_SMI].size += MSM_ION_MM_SIZE;
	table[MEMTYPE_SMI].size += MSM_ION_MFC_SIZE;
#ifdef CONFIG_TZCOM
	table[MEMTYPE_EBI1].size += MSM_ION_QSECOM_SIZE;
#endif
	table[MEMTYPE_EBI1].size += MSM_ION_SF_SIZE;
	table[MEMTYPE_EBI1].size += MSM_ION_ROTATOR_SIZE;
	table[MEMTYPE_EBI1].size += MSM_ION_AUDIO_SIZE;
	return 0;
}

int __init vigor_ion_init(void) {
	platform_device_register(&ion_dev);
	return 0;
}
