/* adlist.c - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2010, Salvatore Sanfilippo <antirez at gmail dot com>
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


#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */
// 创建一个list
list *listCreate(void)
{
    struct list *list;

    if ((list = zmalloc(sizeof(*list))) == NULL)
        return NULL;
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

/* Remove all the elements from the list without destroying the list itself. */
// 删除所有结点，但是保留list
void listEmpty(list *list)
{
    unsigned long len;
    listNode *current, *next;

    current = list->head;
    len = list->len;
    while(len--) {  // 从头结点往后删除
        next = current->next;
        if (list->free) list->free(current->value);     // 如果有设置value的删除函数，调用删除value函数
        zfree(current);     // 释放listNode结点
        current = next;
    }
    list->head = list->tail = NULL;
    list->len = 0;
}

/* Free the whole list.
 *
 * This function can't fail. */
// 删除所有结点，同时删除list
void listRelease(list *list)
{
    listEmpty(list);
    zfree(list);
}

/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
/*
 * 添加数据value到list的头结点，自动创建listNode，o(1)
 * 返回list
 */
list *listAddNodeHead(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)    // 创建一个listNode结点
        return NULL;
    node->value = value;
    if (list->len == 0) {   // list没有结点
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->len++;
    return list;
}

/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
/*
 * 添加数据value到list的尾部，自动创建listNode，list有维护尾结点所以操作是o(1)
 * 返回list
 */
list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)    // 创建一个listNode结点
        return NULL;
    node->value = value;
    if (list->len == 0) {   // list没有结点
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}

/*
 * 在old_node之前或之后插入一个value，自动创建listNode
 * 参数：
 *     list：list结构体指针
 *     old_node：要插入的位置（listNode结构体指针）
 *     value：数据指针
 *     after：非0在old_node之后，为0时在old_node之前
 */
list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;

    // 设置node的prev和next
    if (after) {    // 在old_node之后插入
        node->prev = old_node;
        node->next = old_node->next;
        if (list->tail == old_node) {   // 如果old_node是尾结点，则更新list的tail
            list->tail = node;
        }
    } else {    // 在old_node之前插入
        node->next = old_node;
        node->prev = old_node->prev;
        if (list->head == old_node) {   // 如果old_node是头结点，则更新list的head
            list->head = node;
        }
    }
    if (node->prev != NULL) {   // 如果node不是头结点，则让上一个结点的next指向node
        node->prev->next = node;
    }
    if (node->next != NULL) {   // 如果node不是尾结点，则让下一个结点的prev指向node
        node->next->prev = node;
    }
    list->len++;
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */
/*
 * 删除一个结点，调用list里的free函数指针释放value指针
 */
void listDelNode(list *list, listNode *node)
{
    if (node->prev)
        node->prev->next = node->next;  // 中间结点
    else
        list->head = node->next;    // 头结点
    if (node->next)
        node->next->prev = node->prev;  // 中间结点
    else
        list->tail = node->prev;    // 尾结点
    if (list->free) list->free(node->value);
    zfree(node);
    list->len--;
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */
/*
 * 获取一个迭代器，使用后要调用listReleaseIterator释放迭代器
 * 参数：
 *     list：链表
 *     direction：0：正向，1：反向
 */
listIter *listGetIterator(list *list, int direction)
{
    listIter *iter;

    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;
    if (direction == AL_START_HEAD)
        iter->next = list->head;
    else
        iter->next = list->tail;
    iter->direction = direction;
    return iter;
}

/* Release the iterator memory */
// 释放迭代器内存
void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure */
// 重置迭代器li为list的正向迭代器
void listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;
}

// 重置迭代器li为list的反向迭代器
void listRewindTail(list *list, listIter *li) {
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * */
/*
 * 获取迭代器的一个listNode，会根据迭代器里的正反向标识更新迭代器里的next
 */
listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)   // 正向
            iter->next = current->next;
        else
            iter->next = current->prev;     // 反向
    }
    return current;
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */
/*
 * 复制一个链表，自动分配内存空间
 */
list *listDup(list *orig)
{
    list *copy;
    listIter iter;
    listNode *node;

    if ((copy = listCreate()) == NULL)
        return NULL;
    copy->dup = orig->dup;      // 复制函数
    copy->free = orig->free;    // 删除函数
    copy->match = orig->match;  // 比较函数
    listRewind(orig, &iter);    // 正向迭代器
    while((node = listNext(&iter)) != NULL) {
        void *value;

        // 复制value
        if (copy->dup) {
            value = copy->dup(node->value); // 有设置复制函数则调用复制函数
            if (value == NULL) {
                listRelease(copy);
                return NULL;
            }
        } else
            value = node->value;    // 没有设置复制函数则直接设置指针地址
        if (listAddNodeTail(copy, value) == NULL) { // 添加到新链表的尾部
            listRelease(copy);
            return NULL;
        }
    }
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */
/*
 * 遍历链表，查找key，如果match有设置则调用，没有设置则直接进行指针比较
 */
listNode *listSearchKey(list *list, void *key)
{
    listIter iter;
    listNode *node;

    listRewind(list, &iter);    // 正向迭代器
    while((node = listNext(&iter)) != NULL) {
        if (list->match) {
            if (list->match(node->value, key)) {    // 调用match函数
                return node;
            }
        } else {
            if (key == node->value) {   // 直接比较指针地址
                return node;
            }
        }
    }
    return NULL;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */
/*
 * 获取list的第index个结点。从0开始，0代表头结点；可以为负数，-1代表尾结点
 */
listNode *listIndex(list *list, long index) {
    listNode *n;

    if (index < 0) { // 从尾结点开始算
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {
        n = list->head;
        while(index-- && n) n = n->next;
    }
    return n;
}

/* Rotate the list removing the tail node and inserting it to the head. */
/*
 * 将尾结点插入到头结点
 */
void listRotate(list *list) {
    listNode *tail = list->tail;

    if (listLength(list) <= 1) return;

    /* Detach current tail */
    list->tail = tail->prev;
    list->tail->next = NULL;
    /* Move it as head */
    list->head->prev = tail;
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}

/* Add all the elements of the list 'o' at the end of the
 * list 'l'. The list 'other' remains empty but otherwise valid. */
/*
 * 将链表o添加到l后面，之后o的不再拥有原先的结点但不会释放o的list内存，o(1)
 */
void listJoin(list *l, list *o) {
    if (o->head)    // o链表有元素
        o->head->prev = l->tail;

    if (l->tail)
        l->tail->next = o->head;    // l链表有元素
    else
        l->head = o->head;

    if (o->tail) l->tail = o->tail;
    l->len += o->len;

    /* Setup other as an empty list. */
    o->head = o->tail = NULL;
    o->len = 0;
}
