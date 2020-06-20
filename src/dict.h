/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

#include <stdint.h>

#ifndef __DICT_H
#define __DICT_H

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

typedef struct dictEntry {  // 每个元素
    void *key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct dictEntry *next;
} dictEntry;

typedef struct dictType {
    uint64_t (*hashFunction)(const void *key);
    void *(*keyDup)(void *privdata, const void *key);
    void *(*valDup)(void *privdata, const void *obj);
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
typedef struct dictht {     // 哈希表
    dictEntry **table;      // 相同哈希值对应一个链表
    unsigned long size;     // 容量
    unsigned long sizemask;
    unsigned long used;
} dictht;

typedef struct dict {   // 整个字典
    dictType *type;
    void *privdata;
    dictht ht[2];
    long rehashidx; /* rehashing not in progress if rehashidx == -1 是否正在重新hash*/
    unsigned long iterators; /* number of iterators currently running 正在遍历dict的迭代器数量，如果有迭代器正在使用，在某些操作的时候就不会执行单步rehash操作*/
} dict;

/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
typedef struct dictIterator {   // 迭代器
    dict *d;
    long index;
    int table, safe;    // safe为1时会更新dict::iterators数量
    dictEntry *entry, *nextEntry;
    /* unsafe iterator fingerprint for misuse detection. */
    long long fingerprint;
} dictIterator;

typedef void (dictScanFunction)(void *privdata, const dictEntry *de);
typedef void (dictScanBucketFunction)(void *privdata, dictEntry **bucketref);

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4

/* ------------------------------- Macros ------------------------------------*/
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)

#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        (entry)->v.val = (_val_); \
} while(0)

#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)

#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)

#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)

#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)

#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))

#define dictHashKey(d, key) (d)->type->hashFunction(key)
#define dictGetKey(he) ((he)->key)
#define dictGetVal(he) ((he)->v.val)
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)
#define dictGetDoubleVal(he) ((he)->v.d)
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)
#define dictIsRehashing(d) ((d)->rehashidx != -1)

/* API */
dict *dictCreate(dictType *type, void *privDataPtr);    // 创建一个字典结构
int dictExpand(dict *d, unsigned long size);    // 扩容、缩容；只进行扩容，不会重哈希操作
int dictAdd(dict *d, void *key, void *val);     // 添加元素，如果key已存在则添加失败
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);    // 只添加一个新entry或查找，但是不设置值，由调用者根据返回值设置不同类型的值。如果key不存在则返回值为新entry，如果key存在并且existing不为空则通过existing返回已存在的entry
dictEntry *dictAddOrFind(dict *d, void *key);   // 添加一个新key，如果key已存在则不更新元素值，直接返回key的entry
int dictReplace(dict *d, void *key, void *val); // 添加一个新元素或更新已存在的key，返回值：0-key已存在，1-新增key
int dictDelete(dict *d, const void *key);   // 删除元素，元素存在则返回DICT_OK
dictEntry *dictUnlink(dict *ht, const void *key);   // 该函数只会将元素从字典中移除但不会释放空间，调用者需要自己释放空间，返回值为空表示元素不存在. 使用该函数可以避免多一次查询操作
void dictFreeUnlinkedEntry(dict *d, dictEntry *he); // 释放dictUnlink返回的entryp空间
void dictRelease(dict *d);  // 释放整个dict
dictEntry * dictFind(dict *d, const void *key); // 查找一个key
void *dictFetchValue(dict *d, const void *key); // 查找key的value值，key不存在返回null，key存在返回元素的值
int dictResize(dict *d);
dictIterator *dictGetIterator(dict *d);     // 获取迭代器，初始化跌器
dictIterator *dictGetSafeIterator(dict *d); // 调用dictGetIterator生成迭代器，并将safe字段置为1，开始遍历后会更新dict里跌代器的数量
dictEntry *dictNext(dictIterator *iter);    // 通过跌器获取下一个元素
void dictReleaseIterator(dictIterator *iter);   // 释放跌代器，如果是安全跌代器则会将dict里跌代器数量减1
dictEntry *dictGetRandomKey(dict *d);   // 随机获取一个元素

// 随机返回count个元素，不保证一定返回count个元素，也不保证返回的元素不重复
// des必须保证有足够的空间。返回值是des里获取到的元素个数。
// 如果想要获取n个随机值，该函数比多次调用dictGetRandomKey快的多
unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);
void dictGetStats(char *buf, size_t bufsize, dict *d);  // 获取整个dict的统计信息
uint64_t dictGenHashFunction(const void *key, int len); // 计算hash值
uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);    // 计算hash值(不区分大小写)
void dictEmpty(dict *d, void(callback)(void*)); // 清空字典
void dictEnableResize(void);    // 设置全局变量，允许扩展空间
void dictDisableResize(void);   // 设置全局变量，不允许扩展空间
int dictRehash(dict *d, int n); // 重新哈希，每次重新hash n个元素
int dictRehashMilliseconds(dict *d, int ms);    // 每次调用dicRehash重新hash 100个元素，如果时间超过ms则停止返回
void dictSetHashFunctionSeed(uint8_t *seed);
uint8_t *dictGetHashFunctionSeed(void);
unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);   // 遍历字典，返回的元素可能重复，遍历的元素会调用回调函数，并将privdata作为回调函数的第一个参数
uint64_t dictGetHash(dict *d, const void *key); // 计算hash值
dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, uint64_t hash);   // 根据hash值和key的指针获取元素entry

/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
