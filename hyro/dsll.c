#include "dsll.h"

VOID LinkedListInit(LinkedList* list) {
  list->head = NULL;
  list->tail = NULL;
}

BOOL LinkedListIsEmpty(LinkedList* list) {
  return list->head == NULL;
}

VOID LinkedListInsertHead(LinkedList* list, PVOID value) {
  LinkedListNode* node = (LinkedListNode*)MemAlloc_ZNP(sizeof(LinkedListNode));
  if (node == NULL) {
    HV_LOG_ERROR("Failed to allocate memory for linked list node");
    return;
  }
  node->value = value;
  node->next = NULL;
  if (list->tail) {
    list->tail->next = node;
  }
  else {
    list->head = node;
  }
  list->tail = node;
  return;
}

VOID LinkedListRemoveHead(LinkedList* list, PVOID* value) {
  if (LinkedListIsEmpty(list)) {
    return;
  }
  LinkedListNode* node = list->head;
  *value = node->value;
  list->head = node->next;
  if (list->head == NULL) {
    list->tail = NULL;
  }
  MemFree_P(node);
  return;
}

VOID LinkedListClear(LinkedList* list) {
  LinkedListNode* node = list->head;
  while (node != NULL) {
    LinkedListNode* temp = node;
    node = node->next;
    MemFree_P(temp);
  }
  list->head = NULL;
  list->tail = NULL;
}

VOID LinkedListRemoveSpecific(LinkedList* list, PVOID value) {
  if (list->head == NULL) {
    return;
  }
  if (list->head->value == value) {
    LinkedListNode* temp = list->head;
    list->head = list->head->next;
    if (list->head == NULL) {
      list->tail = NULL;
    }
    MemFree_P(temp);
    return;
  }
  LinkedListNode* prev = list->head;
  LinkedListNode* curr = list->head->next;
  while (curr != NULL) {
    if (curr->value == value) {
      prev->next = curr->next;
      if (curr == list->tail) {
        list->tail = prev;
      }
      MemFree_P(curr);
      return;
    }
    prev = curr;
    curr = curr->next;
  }
  return;
}