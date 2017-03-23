#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define uncompressedfile "original.txt"
#define compressedfile "compressed.txt"
#define coutfile "cout.txt"
#define doutfile "dout.txt"

#define MAX_DICT_SIZE 16
#define MAX_BITS 32

typedef unsigned int uint32;
typedef int int32;
typedef unsigned char uint8;

enum enoding_scheme {
    EORIG = 0,
    ERLE,
    EBITMASK,
    EONEMISMATCH,
    ETWOMISMATCH,
    EFOURMISMATCH,
    ETWOMISANY,
    EDIRECT,
    EMAX
};

// Compression constants
//const uint32 compParam[EMAX][2] = {{35,}}
const char *nibble_bit_rep[16] = {
	[ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
	[ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
	[ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
	[12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111"
};

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
	uint32 i;


    while (fgets(line, sizeof(line), fp)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        //And remove the trailing \r for dos format input files
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        if(strlen(line) == MAX_BITS) //valid 32 bit-char string
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

void write_to_compressed_file(uint32 data, FILE *fout) {
    int i;
    char wbuff[MAX_BITS + 1]; //to hold null char as well
    for (i=7; i>=0; i--)
        strncpy((wbuff+((7-i)*4)), nibble_bit_rep[((data>>(4*i))&0xF)], 4);
    wbuff[MAX_BITS] = '\0';
    fprintf(fout, "%s\n", wbuff);
}

void fit_compressed_integer_and_flush(uint32 code, uint8 codelen, uint32 *pdata, int32 *bitptr, FILE *fout) {
    int32 shift = *bitptr;
    uint32 spillover = code; //save code so that we can write spill overs
    shift -= 7;
    code = (shift > 0) ? code << shift : (code >> (0-shift))&(~(~0 << *bitptr));
    *pdata |= code;
    if(shift <= 0) {
        // Buffer full - flush and realign the bit pointer
        write_to_compressed_file(*pdata, fout);
        *pdata = 0;
        shift += MAX_BITS;
        code = spillover << shift; // check if spillover needs to be truncated - may not be as shifts to beginning take care of it
        *pdata |= code;
    }
    *bitptr = shift;
}

bool encode_direct_match(uint32 currdata, uint32 *pdata, int32 *bitptr, FILE *fout) {
    uint32 i=0;
    uint32 code = (EDIRECT << 4) & 0x7F;
    bool handled = false;

    for(i=0; (i < dict_size) && dict[i] != currdata; i++);

    if(i<dict_size) {
        //data found - encode
        handled = true;
        code |= (i&0xF);
        fit_compressed_integer_and_flush(code, 7, pdata, bitptr, fout);
    }
    return handled;
}

bool rle(uint32 currdata, uint32 *pdata, int32 *bitptr, FILE *fout) {
    static uint32 prevdata, cnt = 0;
    bool handled = false;
    uint32 code = (ERLE << 3) & 0x3F;

    if(cnt == 0) {
        //Compression just started
        prevdata = currdata;
        cnt = 1;
    }
    else if(prevdata == currdata) {
        cnt++;
        handled = true;
    }
    else { //cnt > 0 and prevdata != currdata
        if(cnt > 1) {
            //Encode prevdata and check flush
            code |= ((cnt-2) & 7);
            fit_compressed_integer_and_flush(code, 6, pdata, bitptr, fout);
        }
        prevdata = currdata;
        cnt = 1;
    }

    return handled;
}

bool encode_onemismatch(uint32 currdata, uint32 *pdata, int32 *bitptr, FILE *fout) {
    uint32 i = 0, code = (EONEMISMATCH << 9) & 0xFFF;
    bool handled = false;
    uint32 temp=0;
    uint8 loc;

    for(i=0; i < dict_size; i++) {
        temp = dict[i] ^ currdata;
        if(!(temp & (temp-1))) {
            loc = MAX_BITS - log2(temp) - 1;
            code |= (loc << 4);
            code |= i&0xF;
            fit_compressed_integer_and_flush(code, 12, pdata, bitptr, fout);
            handled = true;
            break;
        }
    }

    return handled;
}

int main() {
	FILE *fin = fopen (uncompressedfile, "r");
	FILE *fout = fopen (coutfile, "w");
 	char line[50];
 	uint32 data=0, intd=0;
 	int32 bitptr = MAX_BITS;

    if ((NULL == fin) || (NULL == fout)){
        perror("Program encountered error exiting..\n");
        exit(1);
    }

    /* Create the dictionary */
    create_dictionary(fin);

    rewind(fin);

    while (fgets(line, sizeof(line), fin)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        //And remove the trailing \r for dos format input files
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        if(strlen(line) == MAX_BITS) {//valid 32 bit-char string
            data = bstr_to_int(line);
            encode_direct_match(data, &intd, &bitptr, fout);
        }
        else //Invalid 32-bit string
            printf("Invalid string %s\n", line);
    }

    fclose(fout);
    fclose(fin);

    return 0;
}
