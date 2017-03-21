#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define uncompressedfile "original.txt"
#define MAX_DICT_SIZE 16

typedef unsigned int uint32;
typedef int int32;

struct node {
    uint32 freq;
    uint32 val;
    struct node *next;
};

uint32 dict[MAX_DICT_SIZE];
uint32 dict_size = MAX_DICT_SIZE;

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
    struct node *newnode = NULL;

    /*while(curr && curr->val != val) {
        prev = curr;
        curr = curr->next;
    }*/
    for(;(curr != NULL) && (curr->val != val); curr = curr->next);
    if(curr != NULL) {
        curr->freq++;
        return;
    }
    newnode = (struct node*)malloc(sizeof(struct node));
    newnode->val = val;
    newnode->freq = 1;
    newnode->next = NULL; // Covers head = NULL scenario
    if(*freq_list)
        //insert at beginning to keep the late comers early to help sorting later
        newnode->next = *freq_list;
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

void deleteallnodes(struct node **freq_list) {
    struct node *curr = *freq_list;
    struct node *next;

    while(curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }
    *freq_list = NULL;
}

uint32 listlength(struct node *freq_list) {
    uint32 len = 0;
    struct node *curr = freq_list;

    for(; curr != NULL; curr=curr->next, len++);

    return len;
}

void populate_dict_from_freq_list(struct node *freq_list, uint32 *dict, uint32 dict_size) {
    /* Size of ll is shortened to dict size so just populate */
    struct node *curr = freq_list;
    int32 index = dict_size-1;
    for(; (curr != NULL) && (index > -1); curr = curr->next, index--)
        dict[index] = curr->val;
}

uint32 bstr_to_int(const char *str) {
    uint32 result=0;
    uint32 i=0;

    for(; (str[i] == '0') || (str[i] == '1'); i++) {
        result = (result << 1) + str[i] - '0';
    }

    return result;
}

void create_dictionary(FILE *fp) {
    struct node *frequencylist = NULL;
    struct node *tempnode = NULL;
	char line[50];
	int i;


    while (fgets(line, sizeof(line), fp)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        //And remove the trailing \r for dos format input files
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        if(strlen(line) == 32) //valid 32 bit-char string
            addnode(&frequencylist, bstr_to_int(line));
        else //Invalid 32-bit string
            printf("Invalid string %s\n", line);
    }


    printf("Before\n");
    printnodes(frequencylist);

    bubbleksort(&frequencylist, MAX_DICT_SIZE);

    printf("\nAfter\n");

    printnodes(frequencylist);

    printf("\n%d nodes\n", MAX_DICT_SIZE);

    tempnode = getklastnodes(frequencylist, MAX_DICT_SIZE);
    printnodes(tempnode);

    dict_size = listlength(tempnode);
    populate_dict_from_freq_list(tempnode, dict, dict_size);

    printf("\n\n");
    for(i=0; i < dict_size; i++)
        printf("dict[%d] = %u\n", i, dict[i]);

    deleteallnodes(&frequencylist);
}

int main() {
	FILE *fin = fopen (uncompressedfile, "r");

    if (NULL == fin) {
        perror("Program encountered error exiting..\n");
        exit(1);
    }

    /* Create the dictionary */
    create_dictionary(fin);

    fclose(fin);

    return 0;
}
