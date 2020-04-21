/* zmalloc - total amount of allocated memory aware version of malloc()
 *
 * Copyright (c) 2009-2010, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef __ZMALLOC_H
#define __ZMALLOC_H

/* Double expansion needed for stringification of macro values. */
#define __xstr(s) __str(s)
#define __str(s) #s

/* 1、系统中存在Google的TC_MALLOC库，则使用tc_malloc一族函数取代原本的malloc一族函数。
 * 2、若系统中存在FaceBook的JEMALLOC库，则使用je_malloc一族函数取代原本的malloc一族函数。
 * 3、若当前系统是Mac系统，则使用<malloc/malloc.h>中的内存分配函数。
 * 4、其它情况，在每一段分配好的空间前头，同一时候多分配一个定长的字段，用来记录分配的空间大小。
 */
#if defined(USE_TCMALLOC)
#define ZMALLOC_LIB ("tcmalloc-" __xstr(TC_VERSION_MAJOR) "." __xstr(TC_VERSION_MINOR))
#include <google/tcmalloc.h>
#if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) tc_malloc_size(p)
#else
#error "Newer version of tcmalloc required"
#endif

#elif defined(USE_JEMALLOC)
#define ZMALLOC_LIB ("jemalloc-" __xstr(JEMALLOC_VERSION_MAJOR) "." __xstr(JEMALLOC_VERSION_MINOR) "." __xstr(JEMALLOC_VERSION_BUGFIX))
#include <jemalloc/jemalloc.h>
#if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) je_malloc_usable_size(p)
#else
#error "Newer version of jemalloc required"
#endif

#elif defined(__APPLE__)
#include <malloc/malloc.h>
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) malloc_size(p)
#endif

/* malloc_usable_size:获取从堆中分配的内存块大小
 * 返回动态分配的缓冲区ptr中可用的字节数，该字节数可能大于请求的大小(但如果请求成功，则保证至少与所请求的大小相同)。
 * 通常，应该自己存储请求的分配大小，而不是使用这个函数。
 * 由于malloc_usable_size获取的内存块大小不是准确的，所以如果需要纪录分配的大小每次分配在地址前加一段定长空间用来记录分配的大小
 */
#ifndef ZMALLOC_LIB
#define ZMALLOC_LIB "libc"
#ifdef __GLIBC__
#include <malloc.h>
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) malloc_usable_size(p)
#endif
#endif

/* We can enable the Redis defrag capabilities only if we are using Jemalloc
 * and the version used is our special version modified for Redis having
 * the ability to return per-allocation fragmentation hints. */
#if defined(USE_JEMALLOC) && defined(JEMALLOC_FRAG_HINT)
#define HAVE_DEFRAG
#endif

void *zmalloc(size_t size);  // 分配内存，不初始化
void *zcalloc(size_t size);  // 分配内存，初始化为\0
void *zrealloc(void *ptr, size_t size); // 重新分配内存，赋值原来内存内容
void zfree(void *ptr);      // 释放
char *zstrdup(const char *s);   // 复制内存的内容到一个新地址，以\0为结束标识
size_t zmalloc_used_memory(void);   // 全局已分配内存大小
void zmalloc_set_oom_handler(void (*oom_handler)(size_t));  // 设置内存不足时的回调函数，默认会abort
size_t zmalloc_get_rss(void);   // 获取进程实际占用内存（包括共享库占用的内存）
int zmalloc_get_allocator_info(size_t *allocated, size_t *active, size_t *resident);    // 使用jemalloc才有意义
size_t zmalloc_get_private_dirty(long pid); // 从/proc/<pid>/smaps获取Private_Dirty。Private_Dirty是映射中已由此进程写入但未被任何其他进程引用的页面
size_t zmalloc_get_smap_bytes_by_field(char *field, long pid);  // 从/proc/self/smaps时获取相应字段值，pid：-1表示当前进程
size_t zmalloc_get_memory_size(void);   // 获取机器内存大小
void zlibc_free(void *ptr);

#ifdef HAVE_DEFRAG
void zfree_no_tcache(void *ptr);
void *zmalloc_no_tcache(size_t size);
#endif

#ifndef HAVE_MALLOC_SIZE
size_t zmalloc_size(void *ptr);
size_t zmalloc_usable(void *ptr);
#else
#define zmalloc_usable(p) zmalloc_size(p)
#endif

#ifdef REDIS_TEST
int zmalloc_test(int argc, char **argv);
#endif

#endif /* __ZMALLOC_H */
