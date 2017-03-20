#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint32;

struct node {
    uint32 freq;
    uint32 val;
    struct node *next;
};

void bubbleksort(struct node **freq_list, uint32 k)
{
    struct node *outer = *freq_list;
    struct node *inner = NULL;
    uint32 temp_freq, temp_val;

    for(;(outer != NULL) && (k > 0); outer = outer->next, k--) {
        for(inner = *freq_list; (inner != NULL) && (inner->next != NULL); inner = inner->next) {
            /* Compair pair wise */
            if(inner->freq > inner->next->freq) {
                temp_val = inner->val;
                temp_freq = inner->freq;
                inner->val = inner->next->val;
                inner->freq = inner->next->freq;
                inner->next->val = temp_val;
                inner->next->freq = temp_freq;
            }
        }
    }
}


void addnode (struct node **freq_list, uint32 val) {
    struct node *curr = *freq_list;
    struct node *prev = curr;
    struct node *newnode = NULL;

    while(curr && curr->val != val) {
        prev = curr;
        curr = curr->next;
    }
    if(curr != NULL) {
        curr->freq++;
        return;
    }
    newnode = (struct node*)malloc(sizeof(struct node));
    newnode->val = val;
    newnode->freq = 1;
    newnode->next = NULL;
    if(*freq_list)
        prev->next = newnode;
    else //head null
        *freq_list = newnode;
}

void printnodes(struct node *freq_list) {
    struct node *curr = freq_list;

    while(curr != NULL) {
        printf("val=%u freq=%u\n", curr->val, curr->freq);
        curr = curr->next;
    }
}

struct node* getklastnodes (struct node *freq_list, uint32 k) {
    struct node *first = freq_list, *second = freq_list;

    while((k > 0) && (second != NULL)) {
        second = second->next;
        k--;
    }

    while(second != NULL) {
        first = first->next;
        second = second->next;
    }
    return first;
}

int main() {
    struct node *frequencylist = NULL;
    struct node *tempnode = NULL;
    int i=0;

    for(i=0; i<5; i++)
        addnode(&frequencylist, 10);
    for(i=0; i<5; i++)
        addnode(&frequencylist, 4);
    for(i=0; i<3; i++)
        addnode(&frequencylist, 10);
    for(i=0; i<3; i++)
        addnode(&frequencylist, 2);
    for(i=0; i<3; i++)
        addnode(&frequencylist, 12);
    for(i=0; i<9; i++)
        addnode(&frequencylist, 6);
    for(i=0; i<1; i++)
        addnode(&frequencylist, 5);
    for(i=0; i<6; i++)
        addnode(&frequencylist, 2);
    for(i=0; i<7; i++)
        addnode(&frequencylist, 5);

    printf("Before\n");
    printnodes(frequencylist);

    bubbleksort(&frequencylist, 3);

    printf("\nAfter\n");

    printnodes(frequencylist);

    printf("\nK nodes\n");

    tempnode = getklastnodes(frequencylist, 3);
    printnodes(tempnode);

    return 0;
}
