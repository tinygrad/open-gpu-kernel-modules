/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES
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

#ifndef FSP_CLOCK_BOOST_RPC_H
#define FSP_CLOCK_BOOST_RPC_H


#define FSP_CLOCK_BOOST_FEATURE_DISABLE_SUBMESSAGE_ID    0x0
#define FSP_CLOCK_BOOST_FEATURE_ENABLE_SUBMESSAGE_ID     0x1
#define FSP_CLOCK_BOOST_TRIGGER_RESTORE_SUBMESSAGE_ID    0x2

#pragma pack(1)

 /*!
  * @brief Clock Boost payload command to FSP
  */
typedef struct
{
    NvU8 subMessageId;
} FSP_CLOCK_BOOST_RPC_PAYLOAD_PARAMS;

#pragma pack()

#endif // FSP_CLOCK_BOOST_RPC_H
