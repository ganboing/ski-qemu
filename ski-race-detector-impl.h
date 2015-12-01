#ifndef SKI_RACE_DETECTOR_IMPL
#define SKI_RACE_DETECTOR_IMPL

#include "ski-race-detector.h"

static inline void ski_race_detector_save_race(ski_race_detector *rd, ski_rd_memory_access *m1, ski_rd_memory_access *m2){
    rd->total_races++;

    if(rd->races_n < SKI_RACE_DETECTOR_RACES_MAX){
        ski_rd_race *race = &rd->races[rd->races_n];
        memcpy(&race->m1, m1, sizeof(ski_rd_memory_access));
        memcpy(&race->m2, m2, sizeof(ski_rd_memory_access));
        rd->races_n++;
    }
}


static inline int ski_race_detector_is_race(ski_rd_memory_access *m1, ski_rd_memory_access *m2){
	assert((m1->cpu) != (m2->cpu));

	if(m1->is_read && m2->is_read){
		return 0;
	}

	if((m1->length==0) || (m2->length == 0)){
		return 0;
	}

	if((m1->physical_memory_address <= m2->physical_memory_address) && ((m1->physical_memory_address + m1->length) > m2->physical_memory_address)){
		return 1;
	}

	if((m2->physical_memory_address <= m1->physical_memory_address) && ((m2->physical_memory_address + m2->length) > m1->physical_memory_address)){
		return 1;
	}

	return 0;
}

static inline void ski_race_detector_new_access(ski_race_detector *rd, int cpu, uint32_t eip, uint32_t vaddr, uint32_t value, int length, int is_read, int instruction_count){

	target_phys_addr_t phyaddr = cpu_get_phys_page_debug(env, vaddr);

	int i;

	assert((cpu>=0) && (cpu<SKI_RD_MAX_CPU));

	ski_rd_memory_access *ma = &rd->last_access_per_cpu[cpu];

	target_ulong old_esp = ma->kernel_stack.esp;
	target_ulong new_esp = ESP;
	target_ulong stack_size = sizeof(linux_kstack);

	if(new_esp >= SKI_LINUX_TASK_SIZE){
		if((new_esp & -stack_size) == (old_esp & -stack_size)){
			//XXX:assume that they are the same stack
			//XXX:does not handle unaligned address
			if((new_esp & -stack_size) == (vaddr & -stack_size) && !is_read){
				void* p = ma->kernel_stack.stack + (vaddr & ~-stack_size);
				switch(length){
				case sizeof(uint8_t):
						*(uint8_t*)p = value;
				break;
				case sizeof(uint16_t):
						*(uint16_t*)p = value;
				break;
				case sizeof(uint32_t):
						*(uint32_t*)p = value;
				break;
				}
			}
		}
		else{
			target_ulong base, len, i;
			for(base = new_esp & -stack_size, len = new_esp - base, i = stack_size; i > len; i-= sizeof(uint64_t)){
				uint64_t data = ldq_data(base + i - sizeof(uint64_t));
				*(uint64_t*)(ma->kernel_stack.stack + i - sizeof(uint64_t)) = data;
			}
		}
		__asm__ __volatile__ ("nop");
	}

	ma->kernel_stack.esp = ESP;
	ma->kernel_stack.ebp = EBP;
	ma->physical_memory_address = phyaddr;
	ma->ip_address = eip;
	ma->length = length;
	ma->is_read = is_read;
	ma->instruction_count = instruction_count;

	for(i=0;i<SKI_RD_MAX_CPU;i++){
		if(i!=cpu){
			ski_rd_memory_access *ma2 =  &(rd->last_access_per_cpu[i]);
			int is_race = ski_race_detector_is_race(ma, ma2);
			if(is_race){
				ski_race_detector_save_race(rd, ma, ma2);
			}
		}
	}
}

#endif
