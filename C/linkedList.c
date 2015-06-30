#include <stdio.h>
#include <stdlib.h>
#include "linkedList.h"


/* Create a linked list to store last received info 
    in order to avoid duplication
*/
/*
struct t_node {
    char *group_id;
    char *node_id;
    long lastReceived;
    struct t_node *next;
};

struct t_node *head;
*/
int updateInfo(LinkedList *head, char * group_id, char * node_id, long lastReceived){
    LinkedList *currentNode = head;

    int updated = 0;
    while(updated == 0 && currentNode != 0) {
        //check current node
        if(currentNode->group_id == group_id && currentNode->node_id == node_id){
            if(currentNode->lastReceived != lastReceived){
                //store information !
                currentNode->lastReceived = lastReceived;
                updated = 1;
            } else {
                //do nothing, the information has already been received !
                updated = 1;
            }
        } else {
            //not the node I want to update
            currentNode = currentNode->next;
        }
    } // end while

    if(updated == 0){
        //create a node to add at the head of the list
        currentNode = (LinkedList *) malloc (sizeof(LinkedList));
        currentNode->group_id = group_id;
        currentNode->node_id = node_id;
        currentNode->lastReceived = lastReceived;
        currentNode->next = head;
        head = currentNode;
        
    }

    return 0;
}

void printList(LinkedList *head){
    LinkedList *currentNode = head;
    int nbNodes = 0;
    if(currentNode == 0){
        printf("List is empty\n");
        printf("---------------\n");
    }
    while(currentNode != 0){
        printf("Node #%i\n", ++nbNodes);
        printf("group_id %s\n", currentNode->group_id);
        printf("node_id %s\n", currentNode->node_id);
        printf("lastReceived %li\n", currentNode->lastReceived);
        printf("---------------\n");
        currentNode = currentNode->next;
    }
    printf("####################\n\n");
}
int countNodes(LinkedList *head){
    int result = 0;
    LinkedList *currentNode = head;
    while(currentNode != 0){
        result++;
        currentNode = currentNode->next;
    }
    return result;
}
void clearListKeep(LinkedList *head, int nbNodesToKeep){
    LinkedList *currentNode = head;
    int nodeIdx = 0;

    //First, count nb Elements
    int nbNodes = countNodes(head);

    while( (nodeIdx++ + nbNodesToKeep < nbNodes) && currentNode != 0){
        if(currentNode->next != 0){
            head = currentNode->next;
            free(currentNode->group_id);
            free(currentNode->node_id);
            free(currentNode);
            currentNode = head;
        } else {
            free(currentNode);
            currentNode = 0;
            head = 0;
        }
    }
}
void clearListClear(LinkedList *head, int nbNodesToClear){
    LinkedList *currentNode = head;
    int nodeIdx = 0;

    while(nodeIdx < nbNodesToClear && currentNode != 0) {
        currentNode = currentNode->next;
        free(head);
        head = currentNode;
        nodeIdx++;
    }
}

int main (int argc, char* argv[]){
	//...
	return 0;
}