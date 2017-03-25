#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define uncompressedfile "original.txt"
#define compressedfile "compressed.txt"
#define coutfile "cout.txt"
#define doutfile "dout.txt"

#define MAX_DICT_SIZE 16
#define MAX_BITS 32
#define RLE_THRESHOLD 9
#define COMPRESSION_END_MARKER "xxxx"

typedef bool (*encodefn)(uint32_t, uint32_t *, uint8_t *, FILE *);
typedef bool (*decodefn) (uint32_t *, uint8_t *, FILE *, uint32_t *);

enum enoding_scheme {
    EORIG = 0,
    ERLE,
    EBITMASK,
    EONEMISMATCH,
    ETWOMISMATCH,
    EFOURMISMATCH,
    EANYTWOMISMATCH,
    EDIRECT,
    EMAX
};

struct code_param {
    uint8_t len;
    uint32_t mask;
};

const struct code_param codeparam[EMAX] = { {32, 0xFFFFFFFF},   {6, 0x3F},      {16, 0xFFFF},   {12, 0xFFF},
                                            {12, 0xFFF},        {12, 0xFFF},    {17, 0x1FFFF},  {7, 0x7F}  };
// Compression constants
//const uint32_t compParam[EMAX][2] = {{35,}}
const char *nibble_bit_rep[16] = {
	[ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
	[ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
	[ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
	[12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111"
};

struct node {
    uint32_t freq;
    uint32_t val;
    struct node *next;
};

uint32_t dict[MAX_DICT_SIZE];
uint32_t dict_size = MAX_DICT_SIZE;

void bubbleksort(struct node **freq_list, uint32_t k)
{
    struct node *outer = *freq_list;
    struct node *inner = NULL;
    uint32_t temp_freq, temp_val;

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


void addnode (struct node **freq_list, uint32_t val) {
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

struct node* getklastnodes (struct node *freq_list, uint32_t k) {
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

uint32_t listlength(struct node *freq_list) {
    uint32_t len = 0;
    struct node *curr = freq_list;

    for(; curr != NULL; curr=curr->next, len++);

    return len;
}

void populate_dict_from_freq_list(struct node *freq_list, uint32_t *dict, uint32_t dict_size) {
    /* Size of ll is shortened to dict size so just populate */
    struct node *curr = freq_list;
    int32_t index = dict_size-1;
    for(; (curr != NULL) && (index > -1); curr = curr->next, index--)
        dict[index] = curr->val;
}

uint32_t bstr_to_int(const char *str) {
    uint32_t result=0;
    uint32_t i=0;

    for(; (str[i] == '0') || (str[i] == '1'); i++) {
        result = (result << 1) + str[i] - '0';
    }

    return result;
}

void create_dictionary(FILE *fp) {
    struct node *frequencylist = NULL;
    struct node *tempnode = NULL;
	char line[50];
	uint32_t i;


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

void write_to_file(uint32_t data, FILE *fout) {
    int i;
    char wbuff[MAX_BITS + 1]; //to hold null char as well
    for (i=7; i>=0; i--)
        strncpy((wbuff+((7-i)*4)), nibble_bit_rep[((data>>(4*i))&0xF)], 4);
    wbuff[MAX_BITS] = '\0';
    fprintf(fout, "%s\n", wbuff);
}

void fit_compressed_integer_and_flush(uint32_t code, uint8_t codelen, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    int8_t shift = *bitptr;
    uint32_t spillover = code; //save code so that we can write spill overs
    shift -= codelen;
    code = (shift > 0) ? code << shift : (code >> (0-shift))&(~(~0 << *bitptr));
    *pdata |= code;
    if(shift <= 0) {
        // Buffer full - flush and realign the bit pointer
        write_to_file(*pdata, fout);
        *pdata = 0;
        shift += MAX_BITS;
        if (shift < MAX_BITS) {
            code = spillover << shift;
            *pdata |= code;
        }
    }
    *bitptr = shift;
}

bool encode_direct_match(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    uint32_t i=0;
    uint32_t code = (EDIRECT << 4) & codeparam[EDIRECT].mask;
    bool handled = false;

    for(i=0; (i < dict_size) && dict[i] != currdata; i++);

    if(i<dict_size) {
        //data found - encode
        handled = true;
        code |= (i&0xF);
        fit_compressed_integer_and_flush(code, codeparam[EDIRECT].len, pdata, bitptr, fout);
    }
    return handled;
}

bool rle(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    static uint32_t prevdata, cnt = 0;
    bool handled = false;
    uint32_t code = (ERLE << 3) & codeparam[ERLE].mask;

    if(cnt == 0) {
        //Compression just started
        prevdata = currdata;
        cnt = 1;
    }
    else if(prevdata == currdata) {
        if(cnt < RLE_THRESHOLD) {
            cnt++;
            handled = true;
        }
        else {
            //Encode data and check flush
            code |= ((cnt-2) & 7);
            fit_compressed_integer_and_flush(code, codeparam[ERLE].len, pdata, bitptr, fout);
            cnt = 1;
        }
    }
    else { //cnt > 0 and prevdata != currdata
        if(cnt > 1) {
            //Encode prevdata and check flush
            code |= ((cnt-2) & 7);
            fit_compressed_integer_and_flush(code, codeparam[ERLE].len, pdata, bitptr, fout);
        }
        prevdata = currdata;
        cnt = 1;
    }

    return handled;
}

uint32_t getsetbits (uint32_t num) {
    uint32_t setbits = 0;

    while(num) {
        setbits++;
        num &= (num - 1);
    }
    return setbits;
}

uint8_t get_first_set_bit_from_lsb(uint32_t num) {
    return (log2(num & ~(num-1))); //first set bit counting from lsb
}

uint8_t get_last_set_bit_from_lsb(uint32_t num) {
    uint8_t res = 0;

    while (num >>= 1) {
        res++;
    }
    return res;
}

bool is_consecutive_ones(uint32_t num, uint8_t numones, uint8_t *startloc) {
    bool retval = false;
    *startloc = get_first_set_bit_from_lsb(num); //first set bit counting from lsb

    if(num == (uint32_t)((~((~0) << numones)) << (*startloc))) {
        retval = true;
        *startloc = MAX_BITS - *startloc - numones;
    }

    return retval;
}

bool bitmask_encoding(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    bool handled = false;
    uint32_t i = 0, code = (EBITMASK << 13) & codeparam[EBITMASK].mask;
    uint8_t first=0, last=0, mask;
    for(i=0; i< dict_size; i++) {
        first = get_first_set_bit_from_lsb(dict[i]^currdata);
        last = get_last_set_bit_from_lsb(dict[i]^currdata);
        if((last - first) <= 3) {
            //Match found
            if(last >= 3)
                mask = ((dict[i]^currdata) >> (last-3)) & 0xF; //bitmask field is 4 bit from last set bit so shift left if last is at least 4 bit away from LSB
            else
                mask = ((dict[i]^currdata) << (3-last)) & 0xF; // last is less than 4 bit away from LSB then shift left to make it 4 bit
            last = MAX_BITS - last -1; //location in code is from right so adjust
            code = (code | (last << 8) | (mask << 4) |(i & 0xF));
            fit_compressed_integer_and_flush(code, codeparam[EBITMASK].len, pdata, bitptr, fout);
            handled = true;
            break;
        }
    }

    return handled;
}

bool anytwo_mismatch_encoding(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    bool handled = false;
    uint32_t i = 0, code = (EANYTWOMISMATCH << 14) & codeparam[EANYTWOMISMATCH].mask;
    uint8_t first=0, last=0;
    for(i=0; i< dict_size; i++) {
        if(getsetbits(dict[i]^currdata) == 2) {
            //Match found
            // This encoding always comes after 2 consective bit change
            // and bitmask so we don't make checks for no presence of them
            first = MAX_BITS - get_first_set_bit_from_lsb(dict[i]^currdata) - 1; //adjust for bit addressing from right, MSB
            last = MAX_BITS - get_last_set_bit_from_lsb(dict[i]^currdata) - 1; //adjust for bit addressing from right, MSB
            code = (code | (last << 9) | (first << 4) |(i & 0xF));
            fit_compressed_integer_and_flush(code, codeparam[EANYTWOMISMATCH].len, pdata, bitptr, fout);
            handled = true;
            break;
        }
    }
    return handled;
}

bool encodeNmismatch(uint32_t currdata, uint8_t nummismatch, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    uint32_t i=0;
    uint8_t startloc = 0;
    bool handled = false;
    uint32_t code = ((EONEMISMATCH + (nummismatch << 1)) << 9) & codeparam[EONEMISMATCH].mask;

    for(i=0; i< dict_size; i++) {
        if(is_consecutive_ones(dict[i]^currdata, nummismatch, &startloc)) {
            //Match found
            code = (code | (startloc << 4) | (i & 0xF));
            fit_compressed_integer_and_flush(code, codeparam[EONEMISMATCH].len, pdata, bitptr, fout);
            handled = true;
            break;
        }
    }

    return handled;
}

bool encode1mismatch(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    return encodeNmismatch(currdata, 1, pdata, bitptr, fout);
}

bool encode2mismatch(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    return encodeNmismatch(currdata, 2, pdata, bitptr, fout);
}

bool encode4mismatch(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    return encodeNmismatch(currdata, 4, pdata, bitptr, fout);
}

bool default_encoding(uint32_t currdata, uint32_t *pdata, uint8_t *bitptr, FILE *fout) {
    /* There is no encoding scheme to encode this data so we need to just add this data as it is
     * but we also need to add a 3 bit code for this. However our intermediate encoding buffer is
     * just 32 bit. So, to encode this 35 bit data, use two passes one with 3 bit code and other
     * with 32 bit original data as is */
     uint32_t code1 = EORIG, code2 = currdata;
     fit_compressed_integer_and_flush(code1, 3, pdata, bitptr, fout);
     fit_compressed_integer_and_flush(code2, 32, pdata, bitptr, fout);
     return true; //always returns true
}

void dump_dictionary_to_output(FILE *fout) {
    uint8_t i = 0;
    for(i=0; i< dict_size; i++) {
        write_to_file(dict[i], fout);
    }
}

void encodefn_dispatcher(FILE *fin, FILE *fout) {
 	char line[50];
 	uint32_t data=0, intd=0, i;
 	uint8_t bitptr = MAX_BITS;
 	bool handled = false;
    // Populate encompression function arrays based on space priority
    encodefn enc_fn_array[EMAX] = {rle, encode_direct_match, encode1mismatch, encode2mismatch, encode4mismatch,
                            bitmask_encoding, anytwo_mismatch_encoding, default_encoding};

    while (fgets(line, sizeof(line), fin)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        //And remove the trailing \r for dos format input files
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        if(strlen(line) == MAX_BITS) {//valid 32 bit-char string
            data = bstr_to_int(line);
            // Start compression beginning with least size
            handled = false;
            for(i=0; (i < EMAX) && (!handled); i++) {
                handled = (*enc_fn_array[i])(data, &intd, &bitptr, fout);
            }
        }
        else //Invalid 32-bit string
            printf("Invalid string %s\n", line);
    }

    // Flush any remaining bytes in intermediate write buffer
    if(bitptr != 0) {
        fit_compressed_integer_and_flush(0, bitptr, &intd, &bitptr, fout);
    }
    fprintf(fout, "%s\n", COMPRESSION_END_MARKER);
    dump_dictionary_to_output(fout);
}

/******************************************** Decompression code ******************************************/

void read_dictionary(FILE *fp) {
    /* Find marker seprating data and dictionary */
	char line[50];
	uint32_t i=0;
	bool dict_start = false;

    while (fgets(line, sizeof(line), fp)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        //And remove the trailing \r for dos format input files
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';
        if((strlen(line) == MAX_BITS) && (dict_start)) //valid 32 bit-char string
            dict[i++] = bstr_to_int(line);
        if (!(strcmp(COMPRESSION_END_MARKER, line)))
            dict_start = true;
    }
    dict_size = i;
}

bool readnextword(FILE *fp, uint32_t *pdata) {
    /* Find marker seprating data and dictionary */
	char line[50];
	bool endofread = false;

    if (fgets(line, sizeof(line), fp)) {
        //remove the trailing \n
        if (line[strlen(line)-1] == '\n')
            line[strlen(line)-1] = '\0';
        //And remove the trailing \r for dos format input files
        if (line[strlen(line)-1] == '\r')
            line[strlen(line)-1] = '\0';

        if (!(strcmp(COMPRESSION_END_MARKER, line))) {
            endofread = true;
        } else if((strlen(line) == MAX_BITS)) //valid 32 bit-char string
            *pdata = bstr_to_int(line);
        else
            printf("Error: Invalid line - %s\n", line); //Should we set endof read - Any way this should never happen
    }
    return endofread;
}

bool read_data_from_offset(FILE *fin, uint32_t *pdata, uint8_t *bitptr, uint8_t len, uint32_t *dataread) {
    int8_t shift = *bitptr - len;
    bool endreached = false;
    if(shift > 0) {
        *dataread = ((*pdata >> shift) & (~(~0 << len)));
        *bitptr = shift;
    }
    else {
        /* First copy from bitptr-1 to end of curr data */
        *dataread = ((*pdata) & (~(~0 << (*bitptr))));
        if(!readnextword(fin, pdata)) {
            *bitptr = MAX_BITS + shift;
            if(shift < 0)
                *dataread = (*dataread << (0-shift)) | (*pdata >> *bitptr);
        }
        else //we reached end
            endreached = true;
    }

    return endreached;
}

bool default_decode(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    // No encoding is applied so just read 32 bits and output
    return read_data_from_offset(fin, pdata, bitptr, codeparam[EORIG].len, decodeddata);
}

bool decode_rle(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    return read_data_from_offset(fin, pdata, bitptr, (codeparam[ERLE].len - 3), decodeddata);
}

bool decode_bitmask(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    uint32_t intd, mask;
    uint8_t shift;
    bool endofdecode = read_data_from_offset(fin, pdata, bitptr, (codeparam[EBITMASK].len - 3), &intd);
    if(!endofdecode) {
        mask = (intd >> 4) & 0xF;
        shift = MAX_BITS - ((intd >> 8) & 0x1F) - 4;
        mask = (shift > 0) ? (mask << shift) : (mask >> (0-shift));
        *decodeddata = dict[intd & 0xF] ^ mask;
    }
    return endofdecode;
}

bool decode1mismatch(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    uint32_t intd, mask;
    uint8_t shift;
    bool endofdecode = read_data_from_offset(fin, pdata, bitptr, (codeparam[EONEMISMATCH].len - 3), &intd);
    if(!endofdecode) {
        shift = MAX_BITS - ((intd >> 4) & 0x1F) - 1;
        mask = 1<<shift;
        *decodeddata = dict[intd & 0xF] ^ mask;
    }
    return endofdecode;
}

bool decode2mismatch(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    uint32_t intd, mask;
    uint8_t shift;
    bool endofdecode = read_data_from_offset(fin, pdata, bitptr, (codeparam[ETWOMISMATCH].len - 3), &intd);
    if(!endofdecode) {
        shift = MAX_BITS - ((intd >> 4) & 0x1F) - 2;
        mask = 3<<shift;
        *decodeddata = dict[intd & 0xF] ^ mask;
    }
    return endofdecode;
}

bool decode4mismatch(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    uint32_t intd, mask;
    uint8_t shift;
    bool endofdecode = read_data_from_offset(fin, pdata, bitptr, (codeparam[EFOURMISMATCH].len - 3), &intd);
    if(!endofdecode) {
        shift = MAX_BITS - ((intd >> 4) & 0x1F) - 4;
        mask = 0xF<<shift;
        *decodeddata = dict[intd & 0xF] ^ mask;
    }
    return endofdecode;
}

bool decode_anytwo_mismatch(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    uint32_t intd, mask1, mask2, mask;
    uint8_t shift1, shift2;
    bool endofdecode = read_data_from_offset(fin, pdata, bitptr, (codeparam[EANYTWOMISMATCH].len - 3), &intd);
    if(!endofdecode) {
        shift1 = MAX_BITS - ((intd >> 4) & 0x1F) - 1;
        shift2 = MAX_BITS - ((intd >> 9) & 0x1F) - 1;
        mask1 = 1<<shift1;
        mask2 = 1<<shift2;
        mask = mask1 | mask2;
        *decodeddata = dict[intd & 0xF] ^ mask;
    }
    return endofdecode;
}

bool decode_direct_match(uint32_t *pdata, uint8_t *bitptr, FILE *fin, uint32_t *decodeddata) {
    uint32_t intd;
    bool endofdecode = read_data_from_offset(fin, pdata, bitptr, (codeparam[EDIRECT].len - 3), &intd);
    if(!endofdecode)
        *decodeddata = dict[intd & 0xF];
    return endofdecode;
}

void decodefn_dispatcher(FILE *fin, FILE *fout) {
    uint32_t data, decoded_data = 0, scheme, prevdata, i;
    uint8_t bitptr = MAX_BITS;

    /* Order same as enoding_scheme enum */
    decodefn fn_array[EMAX] = {default_decode, decode_rle, decode_bitmask, decode1mismatch, decode2mismatch,
                                decode4mismatch, decode_anytwo_mismatch, decode_direct_match};
    bool endofdecode = readnextword(fin, &data);

    while(!endofdecode) {
        endofdecode = read_data_from_offset(fin, &data, &bitptr, 3, &scheme);
        if(!endofdecode) {
            endofdecode = fn_array[scheme](&data, &bitptr, fin, &decoded_data);
            if(!endofdecode) {
                if(scheme != ERLE) {
                    // Flush data to output file
                    write_to_file(decoded_data, fout);
                    prevdata = decoded_data;
                }
                else {
                    // Flush previous data for number of times mentioned
                    for(i=0; i <= decoded_data; i++)
                        write_to_file(prevdata, fout);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
	FILE *fin = NULL;
	FILE *fout = NULL;

    if((argc == 2) && !(strcmp("1", argv[1]))) {
        fin = fopen (uncompressedfile, "r");
        fout = fopen (coutfile, "w");
        if ((NULL == fin) || (NULL == fout)){
            perror("Program encountered error exiting..\n");
            exit(1);
        }

        /* Create the dictionary */
        create_dictionary(fin);

        rewind(fin);

        // launch dispatcher
        encodefn_dispatcher(fin, fout);
    }
    else if ((argc == 2) && !(strcmp("2", argv[1]))) {
        // Decompression
        fin = fopen (compressedfile, "r");
        fout = fopen (doutfile, "w");
        if ((NULL == fin) || (NULL == fout)){
            perror("Program encountered error exiting..\n");
            exit(1);
        }
        read_dictionary(fin);
        rewind(fin);
        decodefn_dispatcher(fin, fout);
    }
    else
        printf("Usage: ./SIM 1 to Compress and ./SIM 2 to decompress\n");

    if ((NULL != fin) && (NULL != fout)){
        fclose(fout);
        fclose(fin);
    }

    return 0;
}
