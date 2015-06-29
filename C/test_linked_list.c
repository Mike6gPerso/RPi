#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/*
test_array.c
Public Domain

gcc -g -o test_array test_array.c

*/


/* Create a linked list to store last received info 
    in order to avoid duplication
*/

struct t_node {
    int group_id;
    int node_id;
    long lastReceived;
    struct t_node *next;
};

struct t_node *head;

int updateInfo(int group_id, int node_id, long lastReceived){
    struct t_node  *currentNode = head;
    struct t_node  *previousNode = head;
    struct t_node  *tempNode = 0;

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
            previousNode = currentNode;
            currentNode = currentNode->next;
        }
    } // end while

    if(updated == 0){
        //create a node to add at the head of the list --> previousNode->next;
        tempNode = (struct t_node *) malloc (sizeof(struct t_node));
        tempNode->group_id = group_id;
        tempNode->node_id = node_id;
        tempNode->lastReceived = lastReceived;
        tempNode->next = 0;
        if(previousNode == 0) {
            //create head !
            head = tempNode;
        } else {
            previousNode->next = tempNode;
        }
    }
}

void printList(){
    struct t_node  *currentNode = head;
    int nbNodes = 0;
    if(currentNode == 0){
        printf("List is empty\n");
        printf("---------------\n");
    }
    while(currentNode != 0){
        printf("Node #%i\n", ++nbNodes);
        printf("group_id %i\n", currentNode->group_id);
        printf("node_id %i\n", currentNode->node_id);
        printf("lastReceived %li\n", currentNode->lastReceived);
        printf("---------------\n");
        currentNode = currentNode->next;
    }
    printf("####################\n\n");
}
int countNodes(){
    int result = 0;
    struct t_node  *currentNode = head;
    while(currentNode != 0){
        result++;
        currentNode = currentNode->next;
    }
    return result;
}
void clearList(int nbNodesToKeep){
    struct t_node  *currentNode = head;
    int nodeIdx = 0;

    //First, count nb Elements
    int nbNodes = countNodes();

    while( (nodeIdx++ + nbNodesToKeep < nbNodes) && currentNode != 0){
        if(currentNode->next != 0){
            head = currentNode->next;
            free(currentNode);
            currentNode = head;
        } else {
            free(currentNode);
            currentNode = 0;
            head = 0;
        }
    }
    
}

int main(int argc, char *argv[]) {
    //First link of the list;
    //struct t_node *head;

    head = 0;
    printList();
    
    updateInfo(2,1,1334);
    printList();

    updateInfo(2,1,1334);
    printList();

    updateInfo(2,1,1336);
    updateInfo(2,2,1340);
    printList();

    updateInfo(2,1,1500);
    updateInfo(2,2,1340);
    printList();

    printf("Start cleaning list\n");
    clearList(3);
    printList();

    clearList(2);
    printList();

    printf("clearList(1);\n");
    clearList(1);
    printList();

    clearList(1);
    printList();

    clearList(0);
    printList();

    updateInfo(2,2,1);
    updateInfo(2,2,2);
    updateInfo(2,2,3);
    updateInfo(2,2,4);
    updateInfo(2,2,5);
    updateInfo(2,2,5);
    updateInfo(2,2,5);
    updateInfo(2,2,5);
    printList();
    clearList(0);
    /*
    head = (struct t_node *) malloc (sizeof(struct t_node));
    head->group_id = -1;
    head->node_id = -1;
    head->next = 0;
    */
}


