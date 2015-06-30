#include <stdio.h>
#include <stdlib.h>

/*
test_linked_list.c
Public Domain

gcc -g -o test_linked_list test_linked_list.c

*/


/* Create a linked list to store last received info 
    in order to avoid duplication
*/

struct t_node {
    char *group_id;
    char *node_id;
    long lastReceived;
    struct t_node *next;
};

struct t_node *head;

int updateInfo(char * group_id, char * node_id, long lastReceived){
    struct t_node  *currentNode = head;
    //struct t_node  *previousNode = head;
    //struct t_node  *tempNode = 0;

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
            //previousNode = currentNode;
            currentNode = currentNode->next;
        }
    } // end while

    if(updated == 0){
        //create a node to add at the head of the list
        currentNode = (struct t_node *) malloc (sizeof(struct t_node));
        currentNode->group_id = group_id;
        currentNode->node_id = node_id;
        currentNode->lastReceived = lastReceived;
        currentNode->next = head;
        head = currentNode;
        /*
        if(previousNode == 0) {
            //create head !
            head = tempNode;
        } else {
            previousNode->next = tempNode;
        }
        */
    }

    return 0;
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
        printf("group_id %s\n", currentNode->group_id);
        printf("node_id %s\n", currentNode->node_id);
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
void clearListKeep(int nbNodesToKeep){
    struct t_node  *currentNode = head;
    int nodeIdx = 0;

    //First, count nb Elements
    int nbNodes = countNodes();

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
void clearListClear(int nbNodesToClear){
    struct t_node  *currentNode = head;
    int nodeIdx = 0;

    while(nodeIdx < nbNodesToClear && currentNode != 0) {
        currentNode = currentNode->next;
        free(head);
        head = currentNode;
        nodeIdx++;
    }
}

int main(int argc, char *argv[]) {
    //First link of the list;
    //struct t_node *head;

    head = 0;
    printList();
    
    updateInfo("2","1",1334);
    printList();

    updateInfo("2","1",1334);
    printList();

    updateInfo("2","1",1336);
    updateInfo("2","2",1340);
    printList();

    updateInfo("2","1",1500);
    updateInfo("2","2",1340);
    printList();

    printf("Start cleaning list\n");
    clearListKeep(3);
    printList();

    clearListKeep(2);
    printList();

    printf("clearListKeep(1);\n");
    clearListKeep(1);
    printList();

    clearListKeep(1);
    printList();

    clearListKeep(0);
    printList();

    updateInfo("2","2",1);
    updateInfo("2","2",2);
    updateInfo("2","2",3);
    updateInfo("2","2",4);
    updateInfo("2","2",5);
    updateInfo("2","2",5);
    updateInfo("2","2",5);
    updateInfo("2","2",5);
    printList();

    updateInfo("2","1",5);
    updateInfo("2","3",5);
    updateInfo("2","4",5);
    updateInfo("2","5",5);
    printList();
    clearListClear(2);
    printList();
    clearListClear(10);
    /*
    head = (struct t_node *) malloc (sizeof(struct t_node));
    head->group_id = -1;
    head->node_id = -1;
    head->next = 0;
    */
}


