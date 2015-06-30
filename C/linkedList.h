// linkedList.h

#ifndef LINKEDLIST_H   /* Include guard */
#define LINKEDLIST_H

typedef struct LinkedList LinkedList;

struct LinkedList
{
	char *group_id;
    char *node_id;
    long lastReceived;
    LinkedList *next;
};

/**
 *	Print the content of the list	
 */
void printList(LinkedList *head);

/**
 *
 */
int updateInfo(LinkedList *head, char * group_id, char * node_id, long lastReceived);

void clearListKeep(LinkedList *head, int nbNodesToKeep);

void clearListClear(LinkedList *head, int nbNodesToClear);

#endif // LINKEDLIST_H