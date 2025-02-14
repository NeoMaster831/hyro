#pragma once

#include "global.h"
#include "mem.h"

typedef struct _LinkedListNode {
  PVOID value;
  struct _LinkedListNode* next;
} LinkedListNode;

typedef struct _LinkedList {
  LinkedListNode* head;
  LinkedListNode* tail;
} LinkedList;

#define FOR_EACH_LIST_ENTRY(Iterator, List) \
    for (LinkedListNode* Iterator = (List)->head; Iterator != NULL; Iterator = Iterator->next)

VOID LinkedListInit(LinkedList* list);
BOOL LinkedListIsEmpty(LinkedList* list);
VOID LinkedListInsertHead(LinkedList* list, PVOID value);
VOID LinkedListRemoveHead(LinkedList* list, PVOID* value);
VOID LinkedListClear(LinkedList* list);
VOID LinkedListRemoveSpecific(LinkedList* list, PVOID value);