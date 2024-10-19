/*
 * SPDX-FileCopyrightText: Copyright (c) 2020-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include "g_videoeventlist_nvoc.h"

#ifndef VIDEO_EVENT_LIST_H
#define VIDEO_EVENT_LIST_H

/*!
 * @file   videoeventlist.h
 * @brief  Provides definition for video tracelog callback on EventBuffer, as well as a list holding the subscribers to the event
 */

#include "core/core.h"
#include "containers/multimap.h"
#include "resserv/resserv.h"
#include "kernel/gpu/eng_desc.h"

#include "class/cl90cdvideo.h"
#include "ctrl/ctrl2080/ctrl2080event.h"

class EventBuffer;
class KernelChannel;
typedef struct
{
    EventBuffer *pEventBuffer;
    NvHandle     hClient;
    NvHandle     hNotifier;
    NvHandle     hEventBuffer;

    NvU64        pUserInfo;

    NvBool       bAdmin;
    NvBool       bKernel;
    NvBool       eventMask;
} NV_EVENT_BUFFER_BIND_POINT_VIDEO;
MAKE_MULTIMAP(VideoEventBufferBindMultiMap, NV_EVENT_BUFFER_BIND_POINT_VIDEO);

/*!
 * Data-structure for notify video events.
 */
typedef struct {
#if PORT_IS_MODULE_SUPPORTED(crypto)
    PORT_CRYPTO_PRNG       *pVideoLogPrng;
#endif
    NvU64                   noisyTimestampStart;
    void                   *pEventData;
} NOTIFY_VIDEO_EVENT;

NV_STATUS videoAddBindpoint
(
    OBJGPU *pGpu,
    RsClient *pClient,
    RsResourceRef *pEventBufferRef,
    NvHandle hNotifier,
    NvBool bAllUsers,
    NV2080_CTRL_EVENT_VIDEO_BIND_EVTBUF_LOD levelOfDetail,
    NvU32 eventFilter
);

void videoBufferTeardown(OBJGPU *pGpu);
void videoRemoveAllBindpoints(EventBuffer *pEventBuffer);
void videoRemoveBindpoint(OBJGPU *pGpu, NvU64 uid, NV_EVENT_BUFFER_BIND_POINT_VIDEO *pBind);
void videoRemoveAllBindpointsForGpu(OBJGPU *pGpu);

/* The callback function that transfers video tracelog buffer entries to user eventbuffers */
void nvEventBufferVideoCallback(OBJGPU *pGpu, void *pArgs);

NV_STATUS videoEventTraceCtxInit(OBJGPU *pGpu, KernelChannel *pKernelChannel, ENGDESCRIPTOR engDesc);

#endif // VIDEO_EVENT_LIST_H
