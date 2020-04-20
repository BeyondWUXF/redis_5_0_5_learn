/* SDSLib 2.0 -- A C dynamic strings library
 *
 * Copyright (c) 2006-2015, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2015, Oran Agra
 * Copyright (c) 2015, Redis Labs, Inc
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

#ifndef __SDS_H
#define __SDS_H

#define SDS_MAX_PREALLOC (1024*1024)    // 当扩充的新长度小于这个值时，扩容的大小为新长度的两倍，当大于这个值时扩充为新长度+这个大小
const char *SDS_NOINIT;

#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>

typedef char *sds;

/* __packed__不进行对齐
 * lsb: 最低有效位
 * msb: 最高有效位*/
/* Note: sdshdr5 is never used, we just access the flags byte directly.
 * However is here to document the layout of type 5 SDS strings. */
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    char buf[];
};
/* len: 实际使用长度
 * alloc: 不包含头部和null标识
 * flags: 低3位表示类型，高5位不使用
 * flags统一在buf前面，这样可以通过s[buf-1]就能获取类型，从而获取字符串长度*/
struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len; /* used */
    uint8_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len; /* used */
    uint32_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7     // 获取类型的掩码
#define SDS_TYPE_BITS 3     // 类型位数
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));    // 根据字符串起始位置定义一个sdshdr**的结构体指针
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))     // 将字符串起始位置的指针转化为sdshdr**的结构体指针
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)  // 计算sdshdr5的字符串长度

/* 根据字符串起始位置获取字符串长度。
 * 先获取类型，再根据不同类型获取len字段
 * 参数：字符串起始位置
 * 返回值：字符串长度*/
static inline size_t sdslen(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->len;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->len;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->len;
    }
    return 0;
}

/* 获取剩下的可用容量
 * 根据字符串起始位置获取类型，再根据分配的长度和已使用的长度计算剩余空间
 * 参数：字符串起始位置
 * 返回值：剩下容量*/
static inline size_t sdsavail(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            return 0;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_16: {
            SDS_HDR_VAR(16,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_32: {
            SDS_HDR_VAR(32,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64,s);
            return sh->alloc - sh->len;
        }
    }
    return 0;
}

/* 设置长度，即设置结构体里已使用（len）的值
 * 参数：
 *     s：字符串起始位置
 *     newlen：长度
 * 返回值：空 */
static inline void sdssetlen(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len = newlen;
            break;
    }
}

/* 增加字符串长度，即增加结构体里已使用（len）的值
 * 参数：
 *     s：字符串起始位置
 *     inc：增量值 */
static inline void sdsinclen(sds s, size_t inc) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                unsigned char newlen = SDS_TYPE_5_LEN(flags)+inc;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len += inc;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len += inc;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len += inc;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len += inc;
            break;
    }
}

/* 获取总容量，即alloc字符
 * 参数：字符串起始位置 */
/* sdsalloc() = sdsavail() + sdslen() */
static inline size_t sdsalloc(const sds s) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->alloc;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->alloc;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->alloc;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->alloc;
    }
    return 0;
}

/* 设置总容量，即设置alloc字段
 * 参数：
 *     s：字符串起始位置
 *     newlen：容量值 */
static inline void sdssetalloc(sds s, size_t newlen) {
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            /* Nothing to do, this type has no total allocation info. */
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->alloc = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->alloc = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->alloc = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->alloc = newlen;
            break;
    }
}

sds sdsnewlen(const void *init, size_t initlen);    // 创建结构体存放init指向的内存initlen长度的字符，返回新字符串起始位置
sds sdsnew(const char *init);   // 创建一个结构体存储init的内容，init里不能包含二进制，因为是通过strlen计算长度的
sds sdsempty(void);     // 创建一个容量为0的结构体
sds sdsdup(const sds s);    // 复制一个sds字符串
void sdsfree(sds s);    // 释放一个sds字符串，参数为字符串的起始位置，函数里自动计算结构体的起始地址
sds sdsgrowzero(sds s, size_t len); /* 调用sdsMakeRoomFor扩容*/
sds sdscatlen(sds s, const void *t, size_t len);    /* 字符串拼接，二进制安全，s可能失效*/
sds sdscat(sds s, const char *t);   /* \0结束的字符串拼接，s可能失效*/
sds sdscatsds(sds s, const sds t);  /* 字符串拼接，二进制安全，s可能失效*/
sds sdscpylen(sds s, const char *t, size_t len);    /* 二进制安全的copy，s空间不足会自动扩容，s可能失效*/
sds sdscpy(sds s, const char *t);   /* copy \0结束的字符串，s空间不足会自动扩容，s可能失效，调用sdscpylen*/

sds sdscatvprintf(sds s, const char *fmt, va_list ap);  /* 格式化后再拼接在s后面，自动扩容，s可能失效*/
#ifdef __GNUC__
sds sdscatprintf(sds s, const char *fmt, ...)   /* 将...转为va_list后调用sdscatvprintf*/
    __attribute__((format(printf, 2, 3)));      // 告诉编译器检查参数类型是否匹配
#else
sds sdscatprintf(sds s, const char *fmt, ...);
#endif

sds sdscatfmt(sds s, char const *fmt, ...); /* 不使用libc的sprintf，速度比sdscatprintf快，但是能使用的格式有限，自动扩容，s可能失效*/
sds sdstrim(sds s, const char *cset);   /* 删除字符串前后cset里的字符集*/
void sdsrange(sds s, ssize_t start, ssize_t end);   /* 删除从start开始的end个字符，start和end如果是负数则从右开始算位置*/
void sdsupdatelen(sds s);   // 根据strlen长度设计sds的长度
void sdsclear(sds s);       /* 清空sds，但是不内存中不会设置为\0*/
int sdscmp(const sds s1, const sds s2); /*和strcmp一样的规则*/

/* 分割字符串，二进制安全
 * 参数：
 *     s：字符串
 *     len：长度
 *     sep：分割符集
 *     seplen：分割符个数
 *     *count：分割后的子串数量
 * 返回值：
 *     子串数组。非空时需要调用sdsfreesplitres释放空间*/
sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);

void sdsfreesplitres(sds *tokens, int count);
void sdstolower(sds s);     /* 遍历调用tolower*/
void sdstoupper(sds s);    /* 遍历调用toupper*/
sds sdsfromlonglong(long long value);   /* long long转字符串，返回sds字符串*/
sds sdscatrepr(sds s, const char *p, size_t len);   /* 将不可打印字符转化为\n\r\t\a\b\\\"，其它的转为\x02x，s会失效*/

/* 分割参数列表，可以用双引号或单引号，支持\xff格式的16进制输入方式，双引号会转化为二进制，单引号不会转换
 *
 * 返回值：
 *     子串数组。非空时需要调用sdsfreesplitres释放空间*/
sds *sdssplitargs(const char *line, int *argc);
sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);    /* 将s里from替换为to，from和to的长度必须一样，setlen为from长度*/
sds sdsjoin(char **argv, int argc, char *sep);  /* 将字符串数组以sep为分割符拼接成一个字符串*/
sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen);    /* 将字符串数组以sep为分割字符串拼接成一个字符串*/

/* Low level functions exposed to the user API */
sds sdsMakeRoomFor(sds s, size_t addlen); /* 扩大sds缓存，如果addlen<avail，不做任何操作，sds的len不会变化*/
void sdsIncrLen(sds s, ssize_t incr);   /* 当往手动往内存中添加数据后，调用此函数增加结构体里的len字段，当增incr超过剩余容量时会assert*/
sds sdsRemoveFreeSpace(sds s);  /* 缩容，缩容后容量与长度一样，并且原指针失效*/
size_t sdsAllocSize(sds s);     /* 返回sds的全部大小，包括结构体的大小、容量及\0*/
void *sdsAllocPtr(sds s);       /* 返回结构体的地址*/

/* Export the allocator used by SDS to the program using SDS.
 * Sometimes the program SDS is linked to, may use a different set of
 * allocators, but may want to allocate or free things that SDS will
 * respectively free or allocate. */
void *sds_malloc(size_t size);
void *sds_realloc(void *ptr, size_t size);
void sds_free(void *ptr);

#ifdef REDIS_TEST
int sdsTest(int argc, char *argv[]);
#endif

#endif
