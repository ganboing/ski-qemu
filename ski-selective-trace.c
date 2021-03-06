/*
 * SKI - Systematic Kernel Interleaving explorer (http://ski.mpi-sws.org)
 *
 * Copyright (c) 2013-2015 Pedro Fonseca
 *
 *
 * This work is licensed under the terms of the GNU GPL, version 3.  See
 * the GPL3 file in SKI's top-level directory.
 *
 */


#include "ski-selective-trace.h"

#ifdef SKI_SELECTIVE_TRACE_ENABLED
                                                 
void ski_selective_trace_init(ski_selective_trace *id){
    memset(id, 0, sizeof(ski_selective_trace));
}


inline static int ski_selective_trace_add_entry(ski_selective_trace *id, unsigned int eip){
    ski_selective_trace_entry* entry = 0;

	//printf("ski_selective_trace_entry\n");

    HASH_FIND_INT(id->instructions_hash, &eip, entry);

    if(entry == 0){
        assert(id->instructions_n < SKI_INSTRUCTION_DETECTOR_ENTRIES_MAX);
        entry = &id->instructions[id->instructions_n];
        id->instructions_n++;

        entry->eip_address = eip;
        HASH_ADD_INT(id->instructions_hash, eip_address, entry);
        return 1;
    }
	printf("[WARNING] ski_selective_trace_add_entry: trying to add a duplicate entry?\n");
	assert(0);
    return 0;
}

#include <errno.h>
extern int errno;

void ski_selective_trace_load(ski_selective_trace *id){
	static FILE* fp_input = 0;

	if(!strlen(ski_init_options_instructions_detector_filename)){
		return;
	}

	if(!fp_input){
		fp_input = fopen(ski_init_options_instructions_detector_filename, "r");
		assert(fp_input);
	}
	rewind(fp_input);

	ski_selective_trace_reset_count(id);

	unsigned int eip_address;
	while(1){
        int res;
        char buffer[512];

        res = fgets(buffer, 512-1, fp_input);
        if(!res){
            break;
        }
        if(buffer[strlen(buffer)-1]=='\n')
            buffer[strlen(buffer)-1] = 0;

		res = sscanf(buffer, "%x", &eip_address);
		//printf("Reading entry, res = %d, errno = %d, eip_address=%x\n", res, errno, eip_address);
		if(res != 1){
			break;
		}
		ski_selective_trace_add_entry(id, eip_address);
	}
	printf("ski_selective_trace_load: Loaded %d addresses\n", id->instructions_n);
	assert(id->instructions_n);
}



void ski_selective_trace_reset_count(ski_selective_trace *id){
	int i;

	// Reset the ip count values, but keep the ip values and the hash structure
	for(i=0;i<id->instructions_n;i++){
		id->instructions[i].count = 0;
	}
	id->total_instructions = 0;
	id->total_distinct_instructions = 0;
}
                                            

#define SKI_INSTRUCTION_DETECTOR_HITS_MAX (64*1024) 

void ski_selective_trace_print(ski_selective_trace *id, char* trace_filename, int seed, int input1, int input2, FILE *fp_id){
    int i;

    printf("ski_selective_trace_print: %d/%d\n", id->total_instructions, id->total_distinct_instructions);
	
	if(id->total_instructions==0)
		return;

	char hits[SKI_INSTRUCTION_DETECTOR_HITS_MAX];
	char *hits_ptr = hits;
	hits[0] = 0;

    for(i=0; i<id->instructions_n; i++){
		ski_selective_trace_entry *ide = &id->instructions[i];
		if(ide->count){
			hits_ptr += sprintf(hits_ptr, "%x ", ide->eip_address); 
			if(hits_ptr + 20 > hits + sizeof(hits)){
				printf("ski_selective_trace_entry: not printing all the entries\n");
				break;
			}
		}
	}

	fprintf(fp_id, "T: %s S: %d I1: %d I2: %d TI: %d TDI: %d HITS: %s\n",
		trace_filename, seed, input1, input2,
		id->total_instructions, id->total_distinct_instructions,
		hits);
}




#endif // SKI_SELECTIVE_TRACE_ENABLED


