/*

 * CIS2750 F2017

 * Assignment 2

 * Jackson Zavarella 0929350

 * This file contains the implementation of the linked List API.

 * No code was used from previous classes/ sources

 */



#include "LinkedListAPI.h"

/** Function to initialize the list metadata head with the appropriate function pointers.
*@return the list struct
*@param printFunction function pointer to print a single node of the list
*@param deleteFunction function pointer to delete a single piece of data from the list
*@param compareFunction function pointer to compare two nodes of the list in order to test for equality or order
**/
List initializeList(char* (*printFunction)(void *toBePrinted),void (*deleteFunction)(void *toBeDeleted),int (*compareFunction)(const void *first,const void *second)) {
  return (List) { .deleteData = deleteFunction, .compare = compareFunction, .printData = printFunction, .length = 0 };
}

/**Function for creating a node for the linked list.
* This node contains abstracted (void *) data as well as previous and next
* pointers to connect to other nodes in the list
*@pre data should be of same size of void pointer on the users machine to avoid size conflicts. data must be valid.
*data must be cast to void pointer before being added.
*@post data is valid to be added to a linked list
*@return On success returns a node that can be added to a linked list. On failure, returns NULL.
*@param data - is a void * pointer to any data type.  Data must be allocated on the heap.
**/
Node* initializeNode(void *data) {
  if (sizeof(void*) != sizeof(data)) {
    return NULL; // Data should be the same size as void pointer to avoid size conflicts
  }

  if (!data) {
    return NULL;
  }

  Node* newNode = malloc(sizeof(Node)); //Allocate memory for the new node
  if (!newNode) {
    return NULL; // If malloc failed for whatever reason then return NULL
  }
  //Ensure the previous and next nodes are not pointing to anything
  newNode->previous = NULL;
  newNode->next = NULL;
  newNode->data = (void*) data; //Assign the data

  return newNode;
}

/**Inserts a Node at the front of a linked list.  List metadata is updated
* so that head and tail pointers are correct.
*@pre 'List' type must exist and be used in order to keep track of the linked list.
*@param list pointer to the dummy head of the list
*@param toBeAdded a pointer to data that is to be added to the linked list
**/
void insertFront(List *list, void *toBeAdded) {
  if (!list) {
    return; // If the list is NULL dont do anything
  }
  Node* newNode = initializeNode(toBeAdded);
  if (!newNode) {
    return; // If the new node is NULL then dont insert
  }
  Node *currentHead = list->head;
  if (currentHead) {
    newNode->next = currentHead; //The previous node of the new node is the current head
    currentHead->previous = newNode; //The next node to the current head will be the new node
  }

  //Assign new head variable in the list
  list->head = newNode;
  //If the tail is null, then this new node must be the only node in the list
  if (!(list->tail)) {
    list->tail = newNode;
  }
  list->length ++;
}

/**Inserts a Node at the back of a linked list.
*List metadata is updated so that head and tail pointers are correct.
*@pre 'List' type must exist and be used in order to keep track of the linked list.
*@param list pointer to the dummy head of the list
*@param toBeAdded a pointer to data that is to be added to the linked list
**/
void insertBack(List *list, void *toBeAdded) {
  if (!list) {
    return; // If the list is NULL dont do anything
  }
  Node* newNode = initializeNode(toBeAdded);
  if (!newNode) {
    return; // If the new node is NULL then dont insert
  }
  Node *currentTail = list->tail;
  if (currentTail) {
    newNode->previous = currentTail; //The previous node of the new node is the current tail in the list
    currentTail->next = newNode; //The next node to the current tail will be the new node
  }

  //Assign new tail variable in the list
  list->tail = newNode;
  //If the head is null, then this new node must be the only node in the list
  if (!(list->head)) {
    list->head = newNode;
  }
  list->length ++;
}

/**Returns a pointer to the data at the front of the list. Does not alter list structure.
 *@pre The list exists and has memory allocated to it
 *@param list pointer to the dummy head of the list containing the head of the list
 *@return pointer to the data located at the head of the list
 **/
void* getFromFront(List list) {
  return ((list.head) != NULL) ? list.head->data : NULL; // If the list doesnt exist, return NULL
}

/**Returns a pointer to the data at the back of the list. Does not alter list structure.
 *@pre The list exists and has memory allocated to it
 *@param list pointer to the dummy head of the list containing the tail of the list
 *@return pointer to the data located at the tail of the list
 **/
void* getFromBack(List list) {
  return ((list.tail) != NULL) ? list.tail->data : NULL; // If the list doesnt exist, return NULL
}

/** Clears the contents linked list, freeing all memory asspociated with these contents.
* uses the supplied function pointer to release allocated memory for the data
*@pre 'List' type must exist and be used in order to keep track of the linked list.
*@param list pointer to the List-type dummy node
**/
void clearList(List *list) {
  if (!list) {
    return; // If the list is NULL dont do anything
  }
  Node* currentNode = list->head;
  while (currentNode != NULL) {
    list->deleteData(currentNode->data); //Release the contents of this node
    Node* next = currentNode->next; //Store the node we will be moving to
    free(currentNode); //Free this node
    currentNode = next; //Move to the next node
    list->head = currentNode; //The new node is now the head of the list
  }
  list->tail = NULL; //List is empty so set the tail to NULL
}

/** Uses the comparison function pointer to place the element in the
* appropriate position in the list.
* should be used as the only insert function if a sorted list is required.
*@pre List exists and has memory allocated to it. Node to be added is valid.
*@post The node to be added will be placed immediately before or after the first occurrence of a related node
*@param list a pointer to the dummy head of the list containing function pointers for delete and compare, as well
as a pointer to the first and last element of the list.
*@param toBeAdded a pointer to data that is to be added to the linked list
**/
void insertSorted(List *list, void *toBeAdded) {
  if (!list) {
    return; // If the list is NULL dont do anything
  }
  Node* currentNode = list->head; //Iterate over the nodes starting from the head
  if (!currentNode) {
    insertBack(list, toBeAdded);
    return; // If the head is NULL then the node the user is trying to add is the only node
  }
  do {
    Node* nextNode = currentNode->next;
    int compareValue = list->compare(toBeAdded, currentNode->data); //Compare toBeAdded with the value of this node

    if (compareValue <= 0) { //If the value is less than or equal to 0 toBeAdded should come before the current node
      Node* newNode = initializeNode(toBeAdded);
      if (!newNode) {
        return; // If the new node is NULL then dont insert
      }
      newNode->next = currentNode; // The next node to the new node is the next node of the current node

      Node* previousNode = currentNode->previous; // Grab the previous node to the current node
      newNode->previous = previousNode; // The previous node of the new node will be the current node's previous

      if (previousNode) { // If the previous node to the current node exists
        previousNode->next = newNode; // Set the next node of the previous node to be the new node
      } else { // If the previous node is NULL
        list->head = newNode; // The new node will be the new head of the list
      }
      currentNode->previous = newNode; // The previous node of the current node will become the new node
      break; // Node has been inserted so we can break out of the loop
    } else if (!nextNode) {
      // If the next node is NULL then we have reached the end of the list and can insert toBeAdded at the end of the list
      insertBack(list, toBeAdded);
      list->length ++;
    }

    currentNode = nextNode; //Move to the next node
  } while (currentNode != NULL);
}

/** Removes data from from the list, deletes the node and frees the memory,
 * changes pointer values of surrounding nodes to maintain list structure.
 * returns the data
 *@pre List must exist and have memory allocated to it
 *@post toBeDeleted will have its memory freed if it exists in the list.
 *@param list pointer to the dummy head of the list containing deleteFunction function pointer
 *@param toBeDeleted pointer to data that is to be removed from the list
 *@return on success: void * pointer to data  on failure: NULL
 **/
void* deleteDataFromList(List *list, void *toBeDeleted) {
  if (!toBeDeleted) {
    return NULL;
  }
  Node* currentNode = list->head; //Iterate over the nodes starting from the head
  while (currentNode != NULL) {
    Node* nextNode = currentNode->next; //Store the next node incase the current node must be freed
    if (list->compare(toBeDeleted, currentNode->data) == 0) { // If the data at the current node equals toBeDeleted
      void* data = currentNode->data;

      Node* previousNode = currentNode->previous;
      if (nextNode) {
        nextNode->previous = previousNode; // The next node's new previous node is the current node's previous node
      } else {
        list->tail = previousNode; // If the next node is NULL, the next node is the new list tail
      }
      if (previousNode) {
        previousNode->next = nextNode; // The previous node's new next node is the current node's next node
      } else {
        list->head = nextNode; // If the previous node is NULL, the next node is the new list head
      }
      free(currentNode); // Free this node
      list->length --;
      return data; // Return pointer to data
    }
    currentNode = nextNode; //Move to the next node
  }
  return NULL; // Return NULL if we have made it to the end of the list
}

/**Returns a string that contains a string representation of
the list traversed from  head to tail. Utilize the list's printData function pointer to create the string.
returned string must be freed by the calling function.
 *@pre List must exist, but does not have to have elements.
 *@param list Pointer to linked list dummy head.
 *@return on success: char * to string representation of list (must be freed after use).  on failure: NULL
 **/
char* toString(List list) {
  int numChars = strlen("HEAD<-->"); //Start keeping track of the number of chars that need to be allocated

  ListIterator iter = createIterator(list); // Create an iterator to loop through the list
  void* element; // Variable to store the data

  while ((element = nextElement(&iter)) != NULL) {
    char* elementString = list.printData(element);
    numChars += strlen(elementString); //Add the number of chars for the string representation of this data
    if (elementString) {
      free(elementString);
    }
    numChars += strlen("<-->"); //Add the number of chars for the arrow symbol inbetween the nodes
  }
  numChars += strlen("TAIL"); //Allocate memory for the TAIL identifier

  char* string = malloc(numChars*sizeof(char) + 1); //Allocate enough memory for all the chars
  if (string == NULL) {
    return NULL; // If the allocation failed return NULL
  }

  strcpy(string, "HEAD<-->"); //Concatenate the HEAD identifier

  iter = createIterator(list); // Make a new iterator to loop over the list again
  while ((element = nextElement(&iter)) != NULL) {
    char* elementString = list.printData(element);
    strcat(string, elementString); //Concatenate the string representation of this data
    if (elementString) {
      free(elementString);
    }
    strcat(string, "<-->"); //Concatenate the link identifier
  }
  strcat(string, "TAIL"); //Concatenate the TAIL identifier

  return string;
}

/** Function for creating an iterator for the linked list.
 * This node contains abstracted (void *) data as well as previous and next
 * pointers to connect to other nodes in the list
 *@pre List exists and is valid
 *@post List remains unchanged.  The iterator has been allocated and points to the head of the list.
 *@return The newly created iterator object.
 *@param list - a pointer to the list to iterate over.
**/
ListIterator createIterator(List list) {
  return (ListIterator) { list.head };
}

/** Function that returns the next element of the list through the iterator.
* This function returns the head of the list the first time it is called after.
* the iterator was created. Every subsequent call returns the next element.
*@pre List exists and is valid.  Iterator exists and is valid.
*@post List remains unchanged.  The iterator points to the next element on the list.
*@return The data associated with the list element that the iterator pointed to when the function was called.
*@param iter - an iterator to a list.
**/
void* nextElement(ListIterator* iter) {
  Node* current  = iter->current;
  if (!current) {
    return NULL;
  }
  iter->current = current->next; // Point to the new nextnode

  return current->data; // Return the data
}

/**Returns the number of elements in the list.
 *@pre List must exist, but does not have to have elements.
 *@param list - the list struct.
 *@return on success: number of eleemnts in the list (0 or more).  on failure: -1 (e.g. list not initlized correctly)
 **/
int getLength(List list) {
  return list.length;
}

/** Function that searches for an element in the list using a comparator function.
 * If an element is found, a pointer to the data of that element is returned
 * Returns NULL if the element is not found.
 *@pre List exists and is valid.  Comparator function has been provided.
 *@post List remains unchanged.
 *@return The data associated with the list element that matches the search criteria.  If element is not found, return NULL.
 *@param list - a list sruct
 *@param compare - a pointer to comparator fuction for customizing the search
 **/
void* findElement(List list, bool (*compare)(const void* first,const void* second), const void* search) {
  ListIterator iter = createIterator(list);
  void* element;
  while ((element = nextElement(&iter))) { // Iterate over the list
    if (compare(element, search) == true) { // Compare the data
      return element; // If they match return the data
    }
  }
  return NULL; // No match
}
