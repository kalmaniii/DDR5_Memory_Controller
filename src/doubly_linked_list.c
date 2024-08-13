/**
 * @file    doubly_linked_list.c
 * @author  your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date    2023-11-14
 *
 * @copyright Copyright (c) 2023
 *
 **/

/*** includes ***/
#include "doubly_linked_list.h"
#include "common.h"

/***
 * functions to initialize linked list object
 ***/
int8_t doubly_ll_create(
    DoublyLinkedList_t **list) {
  /**
   * @brief  initialized a linked list object
   *
   * @param  list     the linked list object
   * @param  value    the value to store in the first node
   * @return int8_t   error code; -1 = fatal error, 0 = no error, 1 = user error
   **/

  // check if list already exist
  if (*list != NULL) {
    return LL_EXIT_USER_ERR;
  }

  // create list
  *list = (DoublyLinkedList_t *)malloc(sizeof(DoublyLinkedList_t));

  if (*list == NULL) {
    return LL_EXIT_FATAL;
  }

  // init list
  (*list)->list_head = NULL;
  (*list)->list_tail = NULL;
  (*list)->size = 0;

  return LL_EXIT_SUCCESS;
}

int8_t doubly_ll_destroy(
    DoublyLinkedList_t **list) {
  /**
   * @brief  frees all the memory allocated to a linked list
   *
   * @param  list     the linked list to free
   * @return int8_t   error code; -1 = fatal error, 0 = no error, 1 = user error
   **/

  // check if list exist
  if (*list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  // traverse list
  node_t *temp = NULL;

  while ((*list)->list_head != NULL) {
    temp = (*list)->list_head->next_node;
    free((*list)->list_head);
    (*list)->list_head = temp;

    (*list)->size--;
  }

  // something went wrong
  if ((*list)->size != 0) {
    return LL_EXIT_FATAL;
  }

  // danglers begone
  (*list)->list_tail = NULL;
  (*list)->list_head = NULL;

  free(*list);
  *list = NULL;
  temp = NULL;

  return LL_EXIT_SUCCESS;
}

/***
 * functions to add nodes in linked list
 ***/
int8_t doubly_ll_insert_at(
    DoublyLinkedList_t **list,
    uint64_t index,
    MemoryRequest_t new_item) {
  /**
   * @brief  inserts node at index given by user
   *
   * @param  list     linked list
   * @param  index    index to insert
   * @return int8_t   error code
   **/

  // check if list exist
  if (*list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  // check if index is in range of list
  if (index > (*list)->size) {
    return LL_EXIT_USER_ERR;
  }

  // create node
  node_t *new_node = NULL;
  new_node = (node_t *)malloc(sizeof(node_t));

  if (new_node == NULL) {
    return LL_EXIT_FATAL;
  }

  new_node->item = new_item;

  // insert at head
  if (index == 0) {
    new_node->prev_node = (*list)->list_tail;
    new_node->next_node = NULL;
    (*list)->list_tail = new_node;

    if ((*list)->size == 0) {
      (*list)->list_head = new_node;
    }

    (*list)->size++;

    return LL_EXIT_SUCCESS;
  }

  // insert at head
  if (index == (*list)->size) {
    new_node->next_node = (*list)->list_head;  // prev points to old tail
    new_node->prev_node = NULL;

    (*list)->list_head->prev_node = new_node;
    (*list)->list_head = new_node;

    (*list)->size++;

    return LL_EXIT_SUCCESS;
  }

  // insert inbetween list
  node_t *current_node = (*list)->list_tail;

  /*** current node points to node right before insertion point
   *   when index equals 1. This is required to insert node at appropriate location.
   ***/
  while (index != 1) {
    current_node = current_node->prev_node;
    index--;
  }

  // new node points to next node and prev node (aka current node)
  new_node->prev_node = current_node->prev_node;
  new_node->next_node = current_node;

  // have the old next node point back new node (the node that took its place)
  current_node->prev_node->next_node = new_node;
  // current node (aka prev node) point to new node
  current_node->prev_node = new_node;

  (*list)->size++;

  return LL_EXIT_SUCCESS;
}

int8_t doubly_ll_insert_head(
    DoublyLinkedList_t **list,
    MemoryRequest_t new_item) {
  /**
   * @brief  adds node to head of list
   *
   * @param  list     the linked list object
   * @param  new_item    the value of the new node
   * @return int8_t   error code
   **/

  // check if list exist
  if (*list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  // create node
  node_t *new_node = NULL;
  new_node = (node_t *)malloc(sizeof(node_t));

  if (new_node == NULL) {
    return LL_EXIT_FATAL;
  }
  new_node->item = new_item;
  new_node->prev_node = NULL;

  // case 1; list is empty
  if ((*list)->size == 0) {
    new_node->next_node = NULL;
    (*list)->list_head = new_node;
    (*list)->list_tail = new_node;
    (*list)->size++;

    return LL_EXIT_SUCCESS;
  }

  // insert to head
  new_node->next_node = (*list)->list_head;  // point to old head
  (*list)->list_head->prev_node = new_node;
  (*list)->list_head = new_node;

  (*list)->size++;

  return LL_EXIT_SUCCESS;
}

int8_t doubly_ll_insert_tail(
    DoublyLinkedList_t **list,
    MemoryRequest_t new_item) {
  /**
   * @brief  adds node to tail of list
   *
   * @param  list     the linked list object
   * @param  new_item    the vlaue of the new node
   * @return int8_t   error code
   **/

  // check if list exist
  if (*list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  // create node
  node_t *new_node = NULL;
  new_node = (node_t *)malloc(sizeof(node_t));

  if (new_node == NULL) {
    return LL_EXIT_FATAL;
  }
  new_node->item = new_item;
  new_node->next_node = NULL;

  // attach to tail if list is empty
  if ((*list)->list_tail == NULL) {
    new_node->prev_node = NULL;
    (*list)->list_tail = new_node;
    (*list)->list_head = new_node;
    (*list)->size++;

    return LL_EXIT_SUCCESS;
  }

  // attach to tail
  new_node->prev_node = (*list)->list_tail;
  (*list)->list_tail->next_node = new_node;
  (*list)->list_tail = new_node;
  (*list)->size++;

  return LL_EXIT_SUCCESS;
}

/***
 * functions to delete nodes in linked list
 ***/
MemoryRequest_t doubly_ll_delete_at(
    DoublyLinkedList_t **list,
    uint64_t index) {
  /**
   * @brief  deletes node at a given index.
   *
   * @param  list
   * @param  index
   * @return int8_t
   **/

  // list existane or empty list
  if ((*list) == NULL || (*list)->size == 0) {
    fprintf(stderr, "%s:%d: delete_at failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  // check if index is in range
  if (index > (*list)->size - 1) {
    fprintf(stderr, "%s:%d: delete_at failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  // traverse list
  node_t *temp = NULL;
  node_t *current_node = (*list)->list_tail;

  // if deleting tail node
  if (index == 0) {
    (*list)->list_tail = current_node->prev_node;
    // (*list)->list_head = current_node->next_node;
    MemoryRequest_t stored_item = current_node->item;

    // check if node is the last one in list
    if ((*list)->size == 1) {
      (*list)->list_head = NULL;
      (*list)->size--;
      free(current_node);

      return stored_item;
    }

    (*list)->list_tail->next_node = NULL;
    (*list)->size--;
    free(current_node);

    return stored_item;
  }

  // if deleting last node in queue
  if (index == (*list)->size - 1) {
    current_node = (*list)->list_head;
    MemoryRequest_t stored_item = current_node->item;

    // case 1; only one node in list
    if ((*list)->size == 1) {
      (*list)->list_head = NULL;
      (*list)->list_tail = NULL;
      (*list)->size--;

      free(current_node);
      return stored_item;
    }

    (*list)->list_head = current_node->next_node;
    (*list)->list_head->prev_node = NULL;
    (*list)->size--;

    free(current_node);
    return stored_item;
  }

  // current_node stops at node right before node at index
  while (index != 1) {
    current_node = current_node->prev_node;
    index--;
  }

  // temp points to node that will be deleted
  temp = current_node->prev_node;
  MemoryRequest_t stored_item = temp->item;

  // have current node point to temp's next node cuz temp node will be del
  current_node->prev_node = temp->prev_node;

  // have next node point back to current node cuz temp node will be del
  temp->prev_node->next_node = current_node;

  (*list)->size--;
  free(temp);  // delete the node temp is pointing to

  return stored_item;
}

MemoryRequest_t doubly_ll_delete_head(
    DoublyLinkedList_t **list) {
  /**
   * @brief  deletes the first node in list
   *
   * @param  list
   * @return int8_t
   **/

  // check if list exist or is empty
  if (*list == NULL || (*list)->size == 0) {
    fprintf(stderr, "%s:%d: delete_head failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  // point to node that will be deleted
  node_t *temp = (*list)->list_head;
  MemoryRequest_t stored_item = temp->item;

  (*list)->list_head = temp->next_node;

  // check if theres only one node
  if ((*list)->size == 1) {
    (*list)->list_tail = NULL;

    // delete node
    free(temp);
    (*list)->size--;

    return stored_item;
  }

  // make sure new head node does not point to old head node
  (*list)->list_head->prev_node = NULL;

  // delete node
  free(temp);
  (*list)->size--;

  return stored_item;
}

MemoryRequest_t doubly_ll_delete_tail(
    DoublyLinkedList_t **list) {
  /**
   * @brief  deletes the last node in list
   *
   * @param  list
   * @return int8_t
   **/

  // check if list exist or is empty
  if (*list == NULL || (*list)->size == 0) {
    fprintf(stderr, "%s:%d: delete_tail failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  node_t *temp = NULL;
  temp = (*list)->list_tail;
  MemoryRequest_t stored_item = temp->item;

  // case 1; only one node in list
  if ((*list)->size == 1) {
    (*list)->list_head = NULL;
    (*list)->list_tail = NULL;
    (*list)->size--;

    free(temp);
    return stored_item;
  }

  (*list)->list_tail = temp->prev_node;
  (*list)->list_tail->next_node = NULL;
  (*list)->size--;

  free(temp);

  return stored_item;
}

/***
 * functions to replace values stored in nodes
 ***/
int8_t doubly_ll_replace_at(
    DoublyLinkedList_t **list,
    uint64_t index,
    MemoryRequest_t new_item) {
  /**
   * @brief  replaces node at index given by user
   *
   * @param  list     linked list
   * @param  index    index to insert
   * @param  new_item the new value that will replace the old
   * @return int8_t   error code
   **/

  // check if list exist
  if (*list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  // check if index is in range of list
  if (index + 1 > (*list)->size) {
    return LL_EXIT_USER_ERR;
  }

  // replace at head
  if (index == 0) {
    (*list)->list_head->item = new_item;
    return LL_EXIT_SUCCESS;
  }

  // insert at tail
  if (index == (*list)->size) {
    (*list)->list_tail->item = new_item;
    return LL_EXIT_SUCCESS;
  }

  // insert inbetween list
  node_t *current_node = (*list)->list_head;

  /*** current node points to node whos value will be replace
  ***/
  while (index != 0) {
    current_node = current_node->next_node;
    index--;
  }

  current_node->item = new_item;

  return LL_EXIT_SUCCESS;
}

int8_t doubly_ll_replace_head(
    DoublyLinkedList_t **list,
    MemoryRequest_t new_item) {
  /**
   * @brief  replaces node at index given by user
   *
   * @param  list     linked list
   * @param  new_item the new value that will replace the old
   * @return int8_t   error code
   **/

  // check if list exist or is empty
  if (*list == NULL || (*list)->size == 0) {
    return LL_EXIT_USER_ERR;
  }

  (*list)->list_head->item = new_item;
  return LL_EXIT_SUCCESS;
}

int8_t doubly_ll_replace_tail(
    DoublyLinkedList_t **list,
    MemoryRequest_t new_item) {
  /**
   * @brief  replaces node at index given by user
   *
   * @param  list     linked list
   * @param  new_item the new value that will replace the old
   * @return int8_t   error code
   **/

  // check if list exist or is empty
  if (*list == NULL || (*list)->size == 0) {
    return LL_EXIT_USER_ERR;
  }

  (*list)->list_tail->item = new_item;

  return LL_EXIT_SUCCESS;
}

void doubly_ll_swap(
  DoublyLinkedList_t **list,
  uint16_t index1,
  uint16_t index2
) {
  // list existane or empty list
  if ((*list) == NULL || (*list)->size == 0) {
    fprintf(stderr, "%s:%d: swap function failed\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  // check if index is in range
  if (index1 >= (*list)->size || index2 >= (*list)->size || index1 == index2) {
    fprintf(stderr, "%s:%d: Invalid index for swap\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  


}

/***
 * functions to retrieve values stored in nodes
 ***/
MemoryRequest_t *doubly_ll_value_at(
    DoublyLinkedList_t *list,
    uint64_t index) {
  /**
   * @brief  retrieve the value from a node at the given index
   *
   * @param  list     linked list
   * @param  index    the node location
   **/

  // check if list exist or is empty
  if (list == NULL || list->size == 0) {
    return NULL;
  }

  // check if index is in range of list
  if (index + 1 > list->size) {
    return NULL;
  }

  // front of queue
  if (index == 0) {
    return &(list->list_tail->item);
  }

  // insert at tail
  if (index == list->size) {
    return &(list->list_head->item);
  }

  // insert inbetween list
  node_t *current_node = list->list_tail;

  /*** current node points to node whos value will be replace
  ***/
  while (index != 0) {
    current_node = current_node->prev_node;
    index--;
  }

  return &(current_node->item);
}

MemoryRequest_t *doubly_ll_value_at_head(
    DoublyLinkedList_t *list) {
  /**
   * @brief  retrieve the value from the first node in the list
   *
   * @param  list     linked list
   * @param  ret_val  the variable where the value gets stored in
   * @return int8_t   error code
   **/

  // check if list exist or is empty
  if (list == NULL || list->size == 0) {
    return NULL;
  }

  return &(list->list_head->item);
}

MemoryRequest_t *doubly_ll_value_at_tail(
    DoublyLinkedList_t *list) {
  /**
   * @brief  retrieve the value from the last node in the list
   *
   * @param  list     linked list
   * @param  ret_val  the variable where the value gets stored in
   * @return int8_t   error code
   **/

  // check if list exist or is empty
  if (list == NULL || list->size == 0) {
    return NULL;
  }

  return &(list->list_tail->item);
}

/***
 * functions to debug linked list
 ***/
int8_t doubly_ll_print_list(
    DoublyLinkedList_t *list) {
  /**
   * @brief  prints all the values stored in the linked list.
   *
   * @param  list     the linked list object
   * @return int8_t   error code
   **/
  // check if list is empty
  if (list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  /*** although "list" is a local variable, "list_head" is not and is being reference.
   *   hence, modifing "list_head" in this function will result in a permanent change.
   ***/
  node_t *temp = list->list_head;

  LOG("queue (size = %" PRIu64 "): \n", list->size);

  // traverse linked list
  while (temp != NULL) {
    LOG(
        "CORE: %hhu, OPERATION: %hhu\n"
        "BG: %hhu, BA: %hhu\n"
        "ROW: %04X, COLH: %02X, COLL: %0X\n"
        "|   \n"
        "V   \n",
        temp->item.core,
        temp->item.operation,
        temp->item.bank_group,
        temp->item.bank,
        temp->item.row,
        temp->item.column_high,
        temp->item.column_low
    );
    temp = temp->next_node;
  }

  LOG("NULL\n\n");

  return LL_EXIT_SUCCESS;
}

int8_t doubly_ll_list_status(
    DoublyLinkedList_t *list) {
  /**
   * @brief  prints all members of list obj.
   *
   * @param  list
   * @return int8_t
   **/

  // check if list exist
  if (list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  LOG(
      "\ndoubly_linked_list_t:\n"
      "\tlist @%p\n"
      "\thead @%p\n"
      "\ttail @%p\n"
      "\tsize = %" PRIu64 "\n",
      list,
      list->list_head,
      list->list_tail,
      list->size);

  return LL_EXIT_SUCCESS;
}

int8_t doubly_ll_node_status(
    DoublyLinkedList_t *list,
    uint64_t index) {
  /**
   * @brief  prints all members of node obj.
   *
   * @param  list
   * @return int8_t
   **/

  // check if list exist
  if (list == NULL) {
    return LL_EXIT_USER_ERR;
  }

  if (index > list->size - 1) {
    return LL_EXIT_USER_ERR;
  }

  node_t *current_node = list->list_head;

  while (index != 0) {
    current_node = current_node->next_node;
    index--;
  }

  LOG(
      "node_t: @%p\n"
      "next node: @%p\n"
      "prev node: @%p\n"
      "item:\n"
      "\ttime: %8" PRIu64 " \n"
      "\tcore: %8hhu\n"
      "\toperation: %8hhu\n"
      "\tbyte_select: %8hhu\n"
      "\tcolumn_low: %8hhu\n"
      "\tchannel: %8hhu\n"
      "\tbank_group: %8hhu\n"
      "\tbank: %8hhu\n"
      "\tcolumn_high: %8hhu\n"
      "\trow: %8u\n\n",
      current_node,
      current_node->next_node,
      current_node->prev_node,
      current_node->item.time,
      current_node->item.core,
      current_node->item.operation,
      current_node->item.byte_select,
      current_node->item.column_low,
      current_node->item.channel,
      current_node->item.bank_group,
      current_node->item.bank,
      current_node->item.column_high,
      current_node->item.row);

  return LL_EXIT_SUCCESS;
}

void doubly_ll_print_err_code(
    int8_t err_code) {
  /**
   * @brief  prints a description of the error code returned
   *         from the linked list API
   *
   * @param  err_code
   * @return int8_t
   **/

  switch (err_code) {
    case LL_EXIT_SUCCESS:
      printf("\nno error occurred\n");
      break;
    case LL_EXIT_FATAL:
      printf("\nmalloc failed\n");
      break;
    case LL_EXIT_USER_ERR:
      printf("\nimproper use of linked list API\n");
      break;
    default:
      printf(
          "\nthe value: %d, does not get return "
          "from any linked list API function\n",
          err_code);
      break;
  }
}

uint64_t doubly_ll_size(
    DoublyLinkedList_t *list) {
  /**
   * @brief  returns the total number of nodes in linked list
   *
   * @param  list         the linked list object
   * @return uint64_t     the total number of nodes
   **/

  // check if list exist
  if (list == NULL) {
    return 0;
  }

  return list->size;
}

MemoryRequest_t *doubly_ll_search_for(
    DoublyLinkedList_t *list,
    MemoryRequest_t value,
    uint64_t *ret_index) {
  /**
   * @brief  returns the index of the *first* node whos value matches
   *         the value passed by the user.
   *
   * @param  list     linked list
   * @param  value    the value to search for in list
   **/

  // check if list exist or is empty
  if (list == NULL || list->size == 0) {
    return NULL;
  }

  // index of node with matching item
  uint64_t current_index = 0;

  // cursor node
  node_t *current_node = list->list_tail;

  // traverse list from front of queue to rear
  while (current_node != NULL) {
    if (
        current_node->item.time == value.time &&
        current_node->item.core == value.core &&
        current_node->item.operation == value.operation &&
        current_node->item.byte_select == value.byte_select &&
        current_node->item.column_low == value.column_low &&
        current_node->item.channel == value.channel &&
        current_node->item.bank_group == value.bank_group &&
        current_node->item.bank == value.bank &&
        current_node->item.column_high == value.column_high &&
        current_node->item.row == value.row
    ) {
      *ret_index = current_index;
      return &(current_node->item);
    }

    // search next node
    current_node = current_node->prev_node;
    current_index++;
  }

  // no match found
  return NULL;
}