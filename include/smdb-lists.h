/*    Copyright 2023 Davide Libenzi
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 * 
 */


#ifndef _SMDB_LISTS_H
#define _SMDB_LISTS_H

struct smdb_listhead {
	struct smdb_listhead *_next;
	struct smdb_listhead *_prev;
};

#define SMDB_LIST_HEAD_INIT(name) { &(name), &(name) }

#define SMDB_LIST_HEAD(name) struct smdb_listhead name = SMDB_LIST_HEAD_INIT(name)

#define SMDB_INIT_LIST_HEAD(ptr)				\
	do {							\
		(ptr)->_next = (ptr); (ptr)->_prev = (ptr);	\
	} while (0)

#define SMDB_LIST_ADD(new, prev, next)			\
	do {						\
		struct smdb_listhead *_prev = prev;	\
		struct smdb_listhead *_next = next;	\
		_next->_prev = new;                     \
		(new)->_next = _next;                   \
		(new)->_prev = _prev;                   \
		_prev->_next = new;                     \
	} while (0)

#define SMDB_LIST_ADDH(new, head) SMDB_LIST_ADD(new, head, (head)->_next)

#define SMDB_LIST_ADDT(new, head) SMDB_LIST_ADD(new, (head)->_prev, head)

#define SMDB_LIST_UNLINK(prev, next)		\
	do {					\
		(next)->_prev = prev;		\
		(prev)->_next = next;		\
	} while (0)

#define SMDB_LIST_DEL(entry) SMDB_LIST_UNLINK((entry)->_prev, (entry)->_next)

#define SMDB_LIST_EMPTY(head) ((head)->_next == head)

#define SMDB_LIST_SPLICE(list, head)					\
	do {								\
		struct smdb_listhead *first = (list)->_next;		\
		if (first != list) {					\
			struct smdb_listhead *last = (list)->_prev;	\
			struct smdb_listhead *at = (head)->_next;	\
			(first)->_prev = head;				\
			(head)->_next = first;				\
			(last)->_next = at;				\
			(at)->_prev = last;				\
		}							\
	} while (0)

#define SMDB_OFFSETOF(type, member) ((long) &((type *) 0L)->member)

#define SMDB_LIST_ENTRY(ptr, type, member) ((type *) ((char *) (ptr) - SMDB_OFFSETOF(type, member)))

#define SMDB_LIST_FOR_EACH(pos, head) for (pos = (head)->_next; pos != (head); pos = (pos)->_next)

#define SMDB_LIST_FOR_EACH_SAFE(pos, n, head)				\
	for (pos = (head)->_next, n = pos->_next; pos != (head); pos = n, n = pos->_next)

#define SMDB_LIST_FIRST(head) (((head)->_next != (head)) ? (head)->_next: NULL)

#define SMDB_LIST_LAST(head) (((head)->_prev != (head)) ? (head)->_prev: NULL)

#define SMDB_LIST_PREV(head, ptr) (((ptr)->_prev != (head)) ? (ptr)->_prev: NULL)

#define SMDB_LIST_NEXT(head, ptr) (((ptr)->_next != (head)) ? (ptr)->_next: NULL)

#define SMDB_LIST_COUNT(cnt, head)		\
	do {					\
		struct smdb_listhead *curr;	\
		(cnt) = 0;			\
		SMDB_LIST_FOR_EACH(curr, head)	\
			(cnt)++;		\
	} while (0)

#endif

