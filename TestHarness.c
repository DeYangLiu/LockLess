#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "set.h"

#include <stdio.h>
#define LOG(fmt, ...) \
	fprintf(stdout, "[%lx %s %d]"fmt, pthread_self(), __FUNCTION__, __LINE__,  ##__VA_ARGS__); 


#define MAX_KEY 9000

void* fuzz_add_remove(void *arg)
{
	const unsigned id = (uintptr_t)arg;
	unsigned i, key;

	for(i = 0; i < 1000; ++i){
		key = rand() % MAX_KEY;
		if(key&1){
			set_add(key);
			//assert(set_contains(key));
		}else{
			set_remove(key);
		}

		//struct timespec dtime = { 0, 1 * 1000 * 1000 };
		//nanosleep(&dtime, NULL);
	}

	LOG("%u clean beg\n", id);
	for(i = 0; i < MAX_KEY; ++i){
		set_remove(i);
	}
	LOG("%u clean end\n", id);

	return NULL;
}


void test_harness(int n, void *fn(void *))
{
	int i;

	pthread_t *th = malloc(sizeof(pthread_t)*n);
	if(!th){
		return;
	}
	

	for(i = 0; i < n; ++i){
		pthread_create(th+i, NULL, fn, (void*)(uintptr_t)i);
	}
	
	for(i = 0; i < n; ++i){
		pthread_join(th[i], NULL);
	}
	
	free(th);
}

int main(int ac, char **av)
{
	if(ac < 2){
		return fprintf(stderr, "args: number_of_threads [fixed_seed]");
	}
	
	set_init();

	time_t seed = time(0);
	if(ac >= 3){
		seed = 0x5855e3f7;// for 4 threads, MAX_KEY=10000
	}
	
	LOG("seed 0x%lx\n", seed);
	
	srand(seed);
	
	
	test_harness(atoi(av[1]), fuzz_add_remove);
	set_term();
	
	return 0;
}
