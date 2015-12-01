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

#ifndef SKI_RACE_DETECTOR
#define SKI_RACE_DETECTOR

#define SKI_RACE_DETECTOR_RACES_MAX 1024
#define SKI_RD_MAX_CPU 4

typedef uint8_t linux_kstack[8192];

typedef struct {
	uint32_t esp;
	uint32_t ebp;
	linux_kstack stack;
}ski_stacktrace;

typedef struct struct_ski_rd_memory_access
{
	int cpu;
	uint32_t physical_memory_address;
	//XXX: actually the physical address is more than 32 bit
	uint32_t ip_address;
	int length; // bits or bytes?
	int is_read;
	int instruction_count;
	ski_stacktrace kernel_stack;
} ski_rd_memory_access;

typedef struct struct_ski_rd_race
{
	ski_rd_memory_access m1;
	ski_rd_memory_access m2;
} ski_rd_race;

typedef struct struct_ski_race_detector
{
	ski_rd_memory_access last_access_per_cpu[SKI_RD_MAX_CPU];	

	ski_rd_race races[SKI_RACE_DETECTOR_RACES_MAX];
	int races_n;

	int total_races;

	int last_cpu;
} ski_race_detector;

#define SKI_LINUX_TASK_SIZE 0xC0000000UL

void ski_race_detector_init(ski_race_detector *rd);
void ski_race_detector_print(ski_race_detector *rd, const char* trace_filename, int seed, int input1, int input2, FILE *fp_races);

#endif

