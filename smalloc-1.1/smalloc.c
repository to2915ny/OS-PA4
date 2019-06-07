#include <unistd.h>
#include <stdio.h>
#include "smalloc.h" 

sm_container_ptr sm_first = 0x0 ;
sm_container_ptr sm_last = 0x0 ;
sm_container_ptr sm_unused_containers = 0x0 ;

void sm_container_split(sm_container_ptr hole, size_t size)
{
	sm_container_ptr remainder = hole->data + size ;

	remainder->data = ((void *)remainder) + sizeof(sm_container_t) ;
	remainder->dsize = hole->dsize - size - sizeof(sm_container_t) ;
	remainder->status = Unused ;
	remainder->next = hole->next ;
	hole->next = remainder ;

	if (hole == sm_last)
		sm_last = remainder ;
}

void * sm_retain_more_memory(int size)
{
	sm_container_ptr hole ;
	int pagesize = getpagesize() ;
	int n_pages = 0 ;
//	printf("page size = %d\n",pagesize);
	n_pages = (sizeof(sm_container_t) + size + sizeof(sm_container_t)) / pagesize  + 1 ;
	hole = (sm_container_ptr) sbrk(n_pages * pagesize) ;
	if (hole == 0x0)
		return 0x0 ;

	hole->data = ((void *) hole) + sizeof(sm_container_t) ;
	hole->dsize = n_pages * getpagesize() - sizeof(sm_container_t) ;
	hole->status = Unused ;

	return hole ;
}

void * smalloc(size_t size) 
{
	sm_container_ptr hole = 0x0 ;
	int best = 0;
		
	sm_container_ptr itr = 0x0 ;
	

	for(itr = sm_first ; itr != 0x0 ; itr = itr->next){
		if(itr->status == Busy)
			continue;

		if(size == itr->dsize){ //if size is the same go for it
			itr->status =Busy;
			return itr->data;
		}
	}

	for (itr = sm_first ; itr != 0x0 ; itr = itr->next) {
		if (itr->status == Busy)
			continue ;


		 if (size + sizeof(sm_container_t) < itr->dsize) {
			// a hole large enought to split
			if(best == 0){
				hole = itr;
				best = 1;
			} 
			else if(hole->dsize > itr->dsize){
				hole = itr ;
			 }
		}
	}
	if (hole == 0x0) {//init?
		hole = sm_retain_more_memory(size) ;

		if (hole == 0x0)
			return 0x0 ;

		if (sm_first == 0x0) {
			sm_first = hole ;
			sm_last = hole ;
			hole->next = 0x0 ;
		}
		else {
			sm_last->next = hole ;
			sm_last = hole ;
			hole->next = 0x0 ;
		}
	}
	sm_container_split(hole, size) ;
	hole->dsize = size ;
	hole->status = Busy ;
	return hole->data ;
}



void sfree(void * p)
{
	sm_container_ptr itr ;
	for (itr = sm_first ; itr->next != 0x0 ; itr = itr->next) {
		if (itr->data == p) {
			itr->status = Unused ;
			break ;
		}
	}
}

void print_sm_containers()
{
	sm_container_ptr itr ;
	int i = 0 ;

	printf("==================== sm_containers ====================\n") ;
	for (itr = sm_first ; itr != 0x0 ; itr = itr->next, i++) {
		char * s ;
		printf("%3d:%p:%s:", i, itr->data, itr->status == Unused ? "Unused" : "  Busy") ;
		printf("%8d:", (int) itr->dsize) ;
		for (s = (char *) itr->data ;
			 s < (char *) itr->data + (itr->dsize > 8 ? 8 : itr->dsize) ;
			 s++) 
			printf("%02x ", *s) ;
		printf("\n") ;
	}
	printf("=======================================================\n") ;

}
void print_sm_uses(){

	sm_container_ptr itr;
	char buffer[100];
	int i =0;
	int mem=0,used=0,unused=0;

	for(itr = sm_first ; itr != 0x0 ; itr = itr->next, i++){
		mem += itr->dsize + sizeof(sm_container_t);
		if(itr->status == Busy){
			used += itr->dsize;}		
		else if(itr->status == Unused){
			unused += itr->dsize;}
	}
//	printf("%d\n",mem);
//	printf("%d\n",used);
//	printf("%d\n",unused);
	snprintf(buffer,100,"Total Retained memory = %d\n",mem);
	fputs(buffer,stderr);
	snprintf(buffer,100,"Total Amount of memory allocated(Used) = %d\n", used);
	fputs(buffer,stderr);
	snprintf(buffer,100,"Total Amount of memory retained but not allocated(Unused) = %d\n",unused);
	fputs(buffer,stderr);
	

}
