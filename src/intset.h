/*
 * Copyright (c) 2009-2012, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __INTSET_H
#define __INTSET_H
#include <stdint.h>

// 保存整数集合，保存的元素结构是数组且是有序的
// 可作为集合键底层实现， 如果一个集合满足以下两个条件：
// 1 保存可转化为long long类型的元素
// 2 元素数量不多
// 3 contents里是小端存储
// 4 编码类型只会加大，不会缩小
typedef struct intset {
    uint32_t encoding;  // 保存元素所使用的类型长度  int16_t/int32_t/int64_t所占的字节数
    uint32_t length;    // 元素个数
    int8_t contents[];  // 保存元素的数组，根据不同类型会将数组转换为不同类型的指针，如:(int64_t*)is->contents
} intset;

intset *intsetNew(void);    // 创建一个空的intset，默认编码为INTSET_ENC_INT16，不会预先分配contents的内存
intset *intsetAdd(intset *is, int64_t value, uint8_t *success); // 将一个整数插入到intset，会重新分配内存
intset *intsetRemove(intset *is, int64_t value, int *success);  // 从intset里删除元素个数，会重新分配内存
uint8_t intsetFind(intset *is, int64_t value);  // 确定一个值是否属于这个集合
int64_t intsetRandom(intset *is);  // 随机返回一个元素
uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value);    // 根据下标获取元素，返回0表示下标超过元素个数
uint32_t intsetLen(const intset *is);   // 获取intset元素个数
size_t intsetBlobLen(intset *is);   // 以字节为单位返回intset blob大小

#ifdef REDIS_TEST
int intsetTest(int argc, char *argv[]);
#endif

#endif // __INTSET_H
