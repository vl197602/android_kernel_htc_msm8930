/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "kgsl_sync.h"

struct sync_pt *kgsl_sync_pt_create(struct sync_timeline *timeline,
	unsigned int timestamp)
{
	struct sync_pt *pt;
	pt = sync_pt_create(timeline, (int) sizeof(struct kgsl_sync_pt));
	if (pt) {
		struct kgsl_sync_pt *kpt = (struct kgsl_sync_pt *) pt;
		kpt->timestamp = timestamp;
	}
	return pt;
}

void kgsl_sync_pt_destroy(struct sync_pt *pt)
{
	sync_pt_free(pt);
}

static struct sync_pt *kgsl_sync_pt_dup(struct sync_pt *pt)
{
	struct kgsl_sync_pt *kpt = (struct kgsl_sync_pt *) pt;
	return kgsl_sync_pt_create(pt->parent, kpt->timestamp);
}

static int kgsl_sync_pt_has_signaled(struct sync_pt *pt)
{
	struct kgsl_sync_pt *kpt = (struct kgsl_sync_pt *) pt;
	struct kgsl_sync_timeline *ktimeline =
		 (struct kgsl_sync_timeline *) pt->parent;
	unsigned int ts = kpt->timestamp;
	unsigned int last_ts = ktimeline->last_timestamp;
	if (timestamp_cmp(last_ts, ts) >= 0) {
		
		return 1;
	}
	return 0;
}

static int kgsl_sync_pt_compare(struct sync_pt *a, struct sync_pt *b)
{
	struct kgsl_sync_pt *kpt_a = (struct kgsl_sync_pt *) a;
	struct kgsl_sync_pt *kpt_b = (struct kgsl_sync_pt *) b;
	unsigned int ts_a = kpt_a->timestamp;
	unsigned int ts_b = kpt_b->timestamp;
	return timestamp_cmp(ts_a, ts_b);
}

void kgsl_sync_timeline_value_str(struct sync_timeline *timeline, char *str,
				   int size)
{
	struct kgsl_sync_timeline *ktimeline =
		 (struct kgsl_sync_timeline *) timeline;
	snprintf(str, size, "%d", ktimeline->last_timestamp);
}

void kgsl_sync_pt_value_str(struct sync_pt *pt, char *str, int size)
{
	struct kgsl_sync_pt *kpt = (struct kgsl_sync_pt *) pt;
	snprintf(str, size, "%d", kpt->timestamp);
}

struct kgsl_fence_event_priv {
	struct kgsl_context *context;
	unsigned int timestamp;
};


static inline void kgsl_fence_event_cb(struct kgsl_device *device,
	void *priv, u32 context_id, u32 timestamp, u32 type)
{
	struct kgsl_fence_event_priv *ev = priv;

	/* Signal time timeline for every event type */
	kgsl_sync_timeline_signal(ev->context->timeline, timestamp);
	kgsl_context_put(ev->context);
	kfree(ev);
}


int kgsl_add_fence_event(struct kgsl_device *device,
	u32 context_id, u32 timestamp, void __user *data, int len,
	struct kgsl_device_private *owner)
{
	struct kgsl_fence_event_priv *event;
	struct kgsl_timestamp_event_fence priv;
	struct kgsl_context *context;
	struct sync_pt *pt;
	struct sync_fence *fence = NULL;
	int ret = -EINVAL;

	if (len != sizeof(priv))
		return -EINVAL;

	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (event == NULL)
		return -ENOMEM;

	context = kgsl_context_get_owner(owner, context_id);

	if (context == NULL) {
		kfree(event);
		return -EINVAL;
	}

	event->context = context;
	event->timestamp = timestamp;

	pt = kgsl_sync_pt_create(context->timeline, timestamp);
	if (pt == NULL) {
		KGSL_DRV_ERR(device, "kgsl_sync_pt_create failed\n");
		ret = -ENOMEM;
		goto fail_pt;
	}

	fence = sync_fence_create("kgsl-fence", pt);
	if (fence == NULL) {
		
		kgsl_sync_pt_destroy(pt);
		KGSL_DRV_ERR(device, "sync_fence_create failed\n");
		ret = -ENOMEM;
		goto fail_fence;
	}

	priv.fence_fd = get_unused_fd_flags(0);
	if (priv.fence_fd < 0) {
		KGSL_DRV_ERR(device, "invalid fence fd\n");
		ret = -EINVAL;
		goto fail_fd;
	}
	sync_fence_install(fence, priv.fence_fd);

	if (copy_to_user(data, &priv, sizeof(priv))) {
		ret = -EFAULT;
		goto fail_copy_fd;
	}

	/*
	 * Hold the context ref-count for the event - it will get released in
	 * the callback
	 */
	ret = kgsl_add_event(device, context_id, timestamp,
			kgsl_fence_event_cb, event, owner);
	if (ret)
		goto fail_event;

	return 0;

fail_event:
fail_copy_fd:
	
	sync_fence_put(fence);
	put_unused_fd(priv.fence_fd);
fail_fd:
	
	sync_fence_put(fence);
fail_fence:
fail_pt:
	kgsl_context_put(context);
	kfree(event);
	return ret;
}

static const struct sync_timeline_ops kgsl_sync_timeline_ops = {
	.dup = kgsl_sync_pt_dup,
	.has_signaled = kgsl_sync_pt_has_signaled,
	.compare = kgsl_sync_pt_compare,
	.timeline_value_str = kgsl_sync_timeline_value_str,
	.pt_value_str = kgsl_sync_pt_value_str,
};

int kgsl_sync_timeline_create(struct kgsl_context *context)
{
	struct kgsl_sync_timeline *ktimeline;

	context->timeline = sync_timeline_create(&kgsl_sync_timeline_ops,
		(int) sizeof(struct kgsl_sync_timeline), "kgsl-timeline");
	if (context->timeline == NULL)
		return -EINVAL;

	ktimeline = (struct kgsl_sync_timeline *) context->timeline;
	ktimeline->last_timestamp = 0;

	return 0;
}

void kgsl_sync_timeline_signal(struct sync_timeline *timeline,
	unsigned int timestamp)
{
	struct kgsl_sync_timeline *ktimeline =
		(struct kgsl_sync_timeline *) timeline;

	if (timestamp_cmp(timestamp, ktimeline->last_timestamp) > 0)
		ktimeline->last_timestamp = timestamp;
	sync_timeline_signal(timeline);
}

void kgsl_sync_timeline_destroy(struct kgsl_context *context)
{
	sync_timeline_destroy(context->timeline);
}
