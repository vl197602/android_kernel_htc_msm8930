/*
 * include/linux/ion.h
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#ifndef _LINUX_ION_H
#define _LINUX_ION_H

#include <linux/ioctl.h>
#include <linux/types.h>

struct ion_handle;
/**
 * enum ion_heap_types - list of all possible types of heaps
 * @ION_HEAP_TYPE_SYSTEM:	 memory allocated via vmalloc
 * @ION_HEAP_TYPE_SYSTEM_CONTIG: memory allocated via kmalloc
 * @ION_HEAP_TYPE_CARVEOUT:	 memory allocated from a prereserved
 * 				 carveout heap, allocations are physically
 * 				 contiguous
 * @ION_HEAP_TYPE_IOMMU: IOMMU memory
 * @ION_HEAP_TYPE_CP:	 memory allocated from a prereserved
 *				carveout heap, allocations are physically
 *				contiguous. Used for content protection.
 * @ION_HEAP_TYPE_DMA:          memory allocated via DMA API
 * @ION_HEAP_END:		helper for iterating over heaps
 */
enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM,
	ION_HEAP_TYPE_SYSTEM_CONTIG,
	ION_HEAP_TYPE_CARVEOUT,
	ION_HEAP_TYPE_DMA,
	ION_HEAP_TYPE_CUSTOM, /* must be last so device specific heaps always
				 are at the end of this enum */
	ION_NUM_HEAPS,
};

#define ION_HEAP_SYSTEM_MASK		(1 << ION_HEAP_TYPE_SYSTEM)
#define ION_HEAP_SYSTEM_CONTIG_MASK	(1 << ION_HEAP_TYPE_SYSTEM_CONTIG)
#define ION_HEAP_CARVEOUT_MASK		(1 << ION_HEAP_TYPE_CARVEOUT)
#define ION_HEAP_TYPE_DMA_MASK         (1 << ION_HEAP_TYPE_DMA)

/**
 * heap flags - the lower 16 bits are used by core ion, the upper 16
 * bits are reserved for use by the heaps themselves.
 */
#define ION_FLAG_CACHED 1		/* mappings of this buffer should be
					   cached, ion will do cache
					   maintenance when the buffer is
					   mapped for dma */

#ifdef __KERNEL__
#include <linux/err.h>
#include <mach/ion.h>
struct ion_device;
struct ion_heap;
struct ion_mapper;
struct ion_client;
struct ion_buffer;

#define ion_phys_addr_t unsigned long
#define ion_virt_addr_t unsigned long

struct ion_platform_heap {
	enum ion_heap_type type;
	unsigned int id;
	const char *name;
	ion_phys_addr_t base;
	size_t size;
	enum ion_memory_types memory_type;
	unsigned int has_outer_cache;
	void *extra_data;
};

/**
 * struct ion_platform_data - array of platform heaps passed from board file
 * @has_outer_cache:    set to 1 if outer cache is used, 0 otherwise.
 * @nr:    number of structures in the array
 * @request_region: function to be called when the number of allocations goes
 *						from 0 -> 1
 * @release_region: function to be called when the number of allocations goes
 *						from 1 -> 0
 * @setup_region:   function to be called upon ion registration
 * @heaps: array of platform_heap structions
 *
 * Provided by the board file in the form of platform data to a platform device.
 */
struct ion_platform_data {
	unsigned int has_outer_cache;
	int nr;
	int (*request_region)(void *);
	int (*release_region)(void *);
	void *(*setup_region)(void);
	struct ion_platform_heap heaps[];
};

#ifdef CONFIG_ION

void ion_reserve(struct ion_platform_data *data);

struct ion_client *ion_client_create(struct ion_device *dev,
				     unsigned int heap_mask, const char *name);


struct ion_client *msm_ion_client_create(unsigned int heap_mask,
					const char *name);

void ion_client_destroy(struct ion_client *client);

struct ion_handle *ion_alloc(struct ion_client *client, size_t len,
			     size_t align, unsigned int flags);

void ion_free(struct ion_client *client, struct ion_handle *handle);

int ion_phys(struct ion_client *client, struct ion_handle *handle,
	     ion_phys_addr_t *addr, size_t *len);

struct sg_table *ion_sg_table(struct ion_client *client,
			      struct ion_handle *handle);

/**
 * ion_map_kernel - create mapping for the given handle
 * @client:	the client
 * @handle:	handle to map
 * @flags:	flags for this mapping
 *
 * Map the given handle into the kernel and return a kernel address that
 * can be used to access this address. If no flags are specified, this
 * will return a non-secure uncached mapping.
 */
void *ion_map_kernel(struct ion_client *client, struct ion_handle *handle);

void ion_unmap_kernel(struct ion_client *client, struct ion_handle *handle);

int ion_share_dma_buf(struct ion_client *client, struct ion_handle *handle);

struct ion_handle *ion_import_dma_buf(struct ion_client *client, int fd);

int ion_handle_get_flags(struct ion_client *client, struct ion_handle *handle,
				unsigned long *flags);


/**
 * ion_map_iommu - map the given handle into an iommu
 *
 * @client - client who allocated the handle
 * @handle - handle to map
 * @domain_num - domain number to map to
 * @partition_num - partition number to allocate iova from
 * @align - alignment for the iova
 * @iova_length - length of iova to map. If the iova length is
 *		greater than the handle length, the remaining
 *		address space will be mapped to a dummy buffer.
 * @iova - pointer to store the iova address
 * @buffer_size - pointer to store the size of the buffer
 * @flags - flags for options to map
 * @iommu_flags - flags specific to the iommu.
 *
 * Maps the handle into the iova space specified via domain number. Iova
 * will be allocated from the partition specified via partition_num.
 * Returns 0 on success, negative value on error.
 */
int ion_map_iommu(struct ion_client *client, struct ion_handle *handle,
			int domain_num, int partition_num, unsigned long align,
			unsigned long iova_length, unsigned long *iova,
			unsigned long *buffer_size,
			unsigned long flags, unsigned long iommu_flags);



int ion_handle_get_size(struct ion_client *client, struct ion_handle *handle,
			unsigned long *size);

void ion_unmap_iommu(struct ion_client *client, struct ion_handle *handle,
			int domain_num, int partition_num);


int ion_secure_heap(struct ion_device *dev, int heap_id, int version,
			void *data);

int ion_unsecure_heap(struct ion_device *dev, int heap_id, int version,
			void *data);

/**
 * msm_ion_do_cache_op - do cache operations.
 *
 * @client - pointer to ION client.
 * @handle - pointer to buffer handle.
 * @vaddr -  virtual address to operate on.
 * @len - Length of data to do cache operation on.
 * @cmd - Cache operation to perform:
 *		ION_IOC_CLEAN_CACHES
 *		ION_IOC_INV_CACHES
 *		ION_IOC_CLEAN_INV_CACHES
 *
 * Returns 0 on success
 */
int msm_ion_do_cache_op(struct ion_client *client, struct ion_handle *handle,
			void *vaddr, unsigned long len, unsigned int cmd);

int ion_iommu_heap_dump_size(void);

#else
static inline void ion_reserve(struct ion_platform_data *data)
{

}

static inline struct ion_client *ion_client_create(struct ion_device *dev,
				     unsigned int heap_mask, const char *name)
{
	return ERR_PTR(-ENODEV);
}

static inline struct ion_client *msm_ion_client_create(unsigned int heap_mask,
					const char *name)
{
	return ERR_PTR(-ENODEV);
}

static inline void ion_client_destroy(struct ion_client *client) { }

static inline struct ion_handle *ion_alloc(struct ion_client *client,
			size_t len, size_t align, unsigned int flags)
{
	return ERR_PTR(-ENODEV);
}

static inline void ion_free(struct ion_client *client,
	struct ion_handle *handle) { }


static inline int ion_phys(struct ion_client *client,
	struct ion_handle *handle, ion_phys_addr_t *addr, size_t *len)
{
	return -ENODEV;
}

static inline struct sg_table *ion_sg_table(struct ion_client *client,
			      struct ion_handle *handle)
{
	return ERR_PTR(-ENODEV);
}

static inline void *ion_map_kernel(struct ion_client *client,
	struct ion_handle *handle, unsigned long flags)
{
	return ERR_PTR(-ENODEV);
}

static inline void ion_unmap_kernel(struct ion_client *client,
	struct ion_handle *handle) { }

static inline int ion_share_dma_buf(struct ion_client *client, struct ion_handle *handle)
{
	return -ENODEV;
}

static inline struct ion_handle *ion_import_dma_buf(struct ion_client *client, int fd)
{
	return ERR_PTR(-ENODEV);
}

static inline int ion_handle_get_flags(struct ion_client *client,
	struct ion_handle *handle, unsigned long *flags)
{
	return -ENODEV;
}

static inline int ion_map_iommu(struct ion_client *client,
			struct ion_handle *handle, int domain_num,
			int partition_num, unsigned long align,
			unsigned long iova_length, unsigned long *iova,
			unsigned long *buffer_size,
			unsigned long flags,
			unsigned long iommu_flags)
{
	return -ENODEV;
}

static inline void ion_unmap_iommu(struct ion_client *client,
			struct ion_handle *handle, int domain_num,
			int partition_num)
{
	return;
}

static inline int ion_secure_heap(struct ion_device *dev, int heap_id,
					int version, void *data)
{
	return -ENODEV;

}

static inline int ion_unsecure_heap(struct ion_device *dev, int heap_id,
					int version, void *data)
{
	return -ENODEV;
}

static inline int msm_ion_do_cache_op(struct ion_client *client,
			struct ion_handle *handle, void *vaddr,
			unsigned long len, unsigned int cmd)
{
	return -ENODEV;
}

#endif 
#endif 


struct ion_allocation_data {
	size_t len;
	size_t align;
	unsigned int flags;
	struct ion_handle *handle;
};


struct ion_allocation_data_compat {
	size_t len;
	size_t align;
	unsigned int flags;
	struct ion_handle *handle;
};
/**
 * struct ion_fd_data - metadata passed to/from userspace for a handle/fd pair
 * @handle:	a handle
 * @fd:		a file descriptor representing that handle
 *
 * For ION_IOC_SHARE or ION_IOC_MAP userspace populates the handle field with
 * the handle returned from ion alloc, and the kernel returns the file
 * descriptor to share or map in the fd field.  For ION_IOC_IMPORT, userspace
 * provides the file descriptor and the kernel returns the handle.
 */
struct ion_fd_data {
	struct ion_handle *handle;
	int fd;
};

struct ion_handle_data {
	struct ion_handle *handle;
};

struct ion_custom_data {
	unsigned int cmd;
	unsigned long arg;
};
#define ION_IOC_MAGIC		'I'

#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data)


#define ION_IOC_ALLOC_COMPAT		_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data_compat)
/**
 * DOC: ION_IOC_FREE - free memory
 *
 * Takes an ion_handle_data struct and frees the handle.
 */
#define ION_IOC_FREE		_IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)

#define ION_IOC_MAP		_IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)

#define ION_IOC_SHARE		_IOWR(ION_IOC_MAGIC, 4, struct ion_fd_data)

/**
 * DOC: ION_IOC_IMPORT - imports a shared file descriptor
 *
 * Takes an ion_fd_data struct with the fd field populated with a valid file
 * descriptor obtained from ION_IOC_SHARE and returns the struct with the handle
 * filed set to the corresponding opaque handle.
 */
#define ION_IOC_IMPORT		_IOWR(ION_IOC_MAGIC, 5, struct ion_fd_data)
#define ION_IOC_IMPORT_COMPAT		_IOWR(ION_IOC_MAGIC, 5, int)

/**
 * DOC: ION_IOC_CUSTOM - call architecture specific ion ioctl
 *
 * Takes the argument of the architecture specific ioctl to call and
 * passes appropriate userdata for that ioctl
 */
#define ION_IOC_CUSTOM		_IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)

#define ION_IOC_CLEAN_CACHES_COMPAT    _IOWR(ION_IOC_MAGIC, 7, \
                                                struct ion_flush_data)
#define ION_IOC_INV_CACHES_COMPAT      _IOWR(ION_IOC_MAGIC, 8, \
                                                struct ion_flush_data)
#define ION_IOC_CLEAN_INV_CACHES_COMPAT       _IOWR(ION_IOC_MAGIC, 9, \
                                                struct ion_flush_data)
#define ION_IOC_GET_FLAGS_COMPAT               _IOWR(ION_IOC_MAGIC, 10, \
                                                struct ion_flag_data)

#endif /* _LINUX_ION_H */
