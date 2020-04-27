/* adlist.h - A generic doubly linked list implementation
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

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */

typedef struct listNode {   // 链表结点
    struct listNode *prev;  // 前一个结点
    struct listNode *next;  // 后一个结点
    void *value;    // 数据
} listNode;

typedef struct listIter {   // 迭代器
    listNode *next; // 正向时指向head，返回时指向tail
    int direction;  // 0：正向，1：反向
} listIter;

typedef struct list {   // 整个链表
    listNode *head;     // 链表头
    listNode *tail;     // 链表尾
    void *(*dup)(void *ptr);    // 复制list的时用来拷贝value的指，不设置时直接复制value的指针。返回类型为指针的函数指针
    void (*free)(void *ptr);    // 删除listNode结点里value的函数，不设置时不会释放value指针。没有返回值的函数指针
    int (*match)(void *ptr, void *key); // listNode结点里value的比较函数。返回类型为int的函数指针
    unsigned long len;  // 链表长度
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len)        // 获取链表长度，l为list指针
#define listFirst(l) ((l)->head)        // 获取链表头
#define listLast(l) ((l)->tail)         // 获取链表尾
#define listPrevNode(n) ((n)->prev)     // 获取结点n的前一个结点，n为listNode指针
#define listNextNode(n) ((n)->next)     // 获取结点n的后一个结点
#define listNodeValue(n) ((n)->value)   // 获取结点n的数据指针

#define listSetDupMethod(l,m) ((l)->dup = (m))      // 设置listNode结点value的复制函数
#define listSetFreeMethod(l,m) ((l)->free = (m))    // 设置listNode结点value的删除函数
#define listSetMatchMethod(l,m) ((l)->match = (m))  // 设置listNode结点的value的比较函数

#define listGetDupMethod(l) ((l)->dup)
#define listGetFree(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

/* Prototypes */
list *listCreate(void);         // 创建一个list
void listRelease(list *list);   // 删除所有结点，同时删除list
void listEmpty(list *list);     // 删除所有结点，但是保留list
list *listAddNodeHead(list *list, void *value);     // 添加数据value到list的头结点，自动创建listNode，o(1)
list *listAddNodeTail(list *list, void *value);     // 添加数据value到list的尾部，自动创建listNode，list有维护尾结点所以操作是o(1)
list *listInsertNode(list *list, listNode *old_node, void *value, int after);   // 在old_node之前或之后插入一个value，自动创建listNode
void listDelNode(list *list, listNode *node);       // 删除一个结点，调用list里的free函数指针释放value指针
listIter *listGetIterator(list *list, int direction);   // 获取一个迭代器，使用后要调用listReleaseIterator释放迭代器
listNode *listNext(listIter *iter);             // 获取迭代器的一个listNode，会根据迭代器里的正反向标识更新迭代器里的next
void listReleaseIterator(listIter *iter);       // 释放迭代器内存
list *listDup(list *orig);                      // 复制一个链表，自动分配内存空间，复制、删除、比较函数都会相同
listNode *listSearchKey(list *list, void *key); // 遍历链表，查找key，如果match有设置则调用，没有设置则直接进行指针比较
listNode *listIndex(list *list, long index);    // 获取list的第index个结点。从0开始，0代表头结点；可以为负数，-1代表尾结点
void listRewind(list *list, listIter *li);      // 重置迭代器li为list的正向迭代器
void listRewindTail(list *list, listIter *li);  // 重置迭代器li为list的反向迭代器
void listRotate(list *list);                    // 将尾结点插入到头结点
void listJoin(list *l, list *o);                // 将链表o添加到l后面，之后o的不再拥有原先的结点但不会释放o的list内存，o(1)

/* Directions for iterators */
#define AL_START_HEAD 0     // 正向迭代器
#define AL_START_TAIL 1     // 返回迭代器

#endif /* __ADLIST_H__ */
