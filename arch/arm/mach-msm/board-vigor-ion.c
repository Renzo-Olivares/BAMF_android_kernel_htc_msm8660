/* linux/arch/arm/mach-msm/board-vigor-ion.c
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
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

extern void pmem_request_smi_region(void *data);
extern void pmem_release_smi_region(void *data);
extern void *pmem_setup_smi_region(void);

#define MSM_ION_CAMERA_BASE   0x40E00000
#define MSM_ION_CAMERA_SIZE   0x2000000
#define MSM_ION_AUDIO_SIZE	  0x19000
#define MSM_PMEM_SF_SIZE         0x8200000 /* 130 Mbytes */

#define MSM_PMEM_ADSP2_SIZE      0x800000
#define MSM_PMEM_ADSP_SIZE       0x7800000
/* MAX( prim, video)
 * prim = 1280 * 736 * 4 * 2
 * video = 1152 * 1920 * 1.5 * 2
*/
#define MSM_PMEM_AUDIO_SIZE      0x4CF000
#define MSM_PMEM_TZCOM_SIZE      0xC7000

#define MSM_PMEM_ADSP2_BASE      (0x80000000 - MSM_PMEM_ADSP2_SIZE)
#define MSM_PMEM_ADSP_BASE       (MSM_PMEM_ADSP2_BASE - MSM_PMEM_ADSP_SIZE)
#define MSM_PMEM_SF_BASE         (0x40400000)
#define MSM_PMEM_TZCOM_BASE      (MSM_PMEM_SF_BASE + MSM_PMEM_SF_SIZE)
#define MSM_PMEM_AUDIO_BASE      (MSM_PMEM_TZCOM_BASE + MSM_PMEM_TZCOM_SIZE)
#define MSM_FB_BASE              (MSM_PMEM_AUDIO_BASE + MSM_PMEM_AUDIO_SIZE)

#define MSM_SMI_BASE				0x38000000
#define MSM_SMI_SIZE				0x4000000

/* Kernel SMI PMEM Region for video core, used for Firmware */
/* and encoder, decoder scratch buffers */
/* Kernel SMI PMEM Region Should always precede the user space */
/* SMI PMEM Region, as the video core will use offset address */
/* from the Firmware base */
#define KERNEL_SMI_BASE			 (MSM_SMI_BASE)
#define KERNEL_SMI_SIZE			 0x600000

/* User space SMI PMEM Region for video core*/
/* used for encoder, decoder input & output buffers  */
#define USER_SMI_BASE			   (KERNEL_SMI_BASE + KERNEL_SMI_SIZE)
#define USER_SMI_SIZE			   (MSM_SMI_SIZE - KERNEL_SMI_SIZE)
#define MSM_PMEM_SMIPOOL_BASE	   USER_SMI_BASE
#define MSM_PMEM_SMIPOOL_SIZE	   USER_SMI_SIZE
#define MSM_PMEM_KERNEL_EBI1_SIZE  0x600000 /* (6MB) For QSECOM */
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define MSM_ION_SF_SIZE       MSM_PMEM_SF_SIZE
#define MSM_ION_ROTATOR_SIZE  MSM_PMEM_ADSP2_SIZE
#define MSM_ION_MM_FW_SIZE    0x200000  /* KERNEL_SMI_SIZE */
#define MSM_ION_MM_SIZE       0x3D00000 /* USER_SMI_SIZE */
#define MSM_ION_MFC_SIZE      SZ_8K  /* KERNEL_SMI_SIZE */
#define MSM_ION_WB_SIZE       0x2FD000  /* MSM_OVERLAY_BLT_SIZE */

#ifdef CONFIG_TZCOM
#define MSM_ION_QSECOM_SIZE   0x600000
#define MSM_ION_HEAP_NUM      8
#else
#define MSM_ION_HEAP_NUM      8
#endif

#define MSM_ION_MM_FW_BASE    0x38000000
#define MSM_ION_MM_BASE	      (MSM_ION_MM_FW_BASE + MSM_ION_MM_FW_SIZE)
#define MSM_ION_MFC_BASE      (MSM_ION_MM_BASE + MSM_ION_MM_SIZE)

#define MSM_ION_WB_BASE       (0x80000000 - MSM_ION_WB_SIZE)
#define MSM_ION_ROTATOR_BASE  (MSM_ION_WB_BASE - MSM_ION_ROTATOR_SIZE)
#define MSM_ION_QSECOM_BASE   (MSM_ION_ROTATOR_BASE - MSM_ION_QSECOM_SIZE)

#else /* CONFIG_MSM_MULTIMEDIA_USE_ION */
#define MSM_ION_HEAP_NUM      8
#endif

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
static struct ion_cp_heap_pdata cp_wb_ion_pdata = {
	.permission_type = IPT_TYPE_MDP_WRITEBACK,
	.align = PAGE_SIZE,
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
			.id	= ION_CP_WB_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_WB_HEAP_NAME,
			.base	= MSM_ION_WB_BASE,
			.size	= MSM_ION_WB_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_wb_ion_pdata,
		},
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
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