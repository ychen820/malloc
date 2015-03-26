#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
size_t chunk_size; 
size_t min_blk_size=8;
size_t blk_hdr_size=16;
int max_blk_bit=31;
int mylog2(size_t size)
{
	int bits=0;
	while(size!=1)
	 {size>>=1;
		 bits++;
		 
	 }
	 return bits;
}

int init_status=0;

struct node
{
	size_t log2_capacity;
	struct node *next;
};

struct node *freelist[31];

extern void * cs550_mmap_wrapper(size_t sz);
static void
split(char *const begin, char *const end, char *const allocated) {

    const size_t sz = end - begin;
    if (allocated <= begin) {
        // dbg_print("Free: %lu, %lu\n", (size_t) begin, (size_t) end);	
        struct node *b = (struct node *) begin;
        b->log2_capacity = mylog2(sz);
        b->next = freelist[b->log2_capacity];
        freelist[b->log2_capacity] = b;
    // If allocated region completely overlaps the current region, then there
    // is no part of the current region that is free, so we stop recursion.
    // Otherwise, split each half.
    } else if (allocated < end) {
        char *half_way = begin + sz/2;
        split(begin, half_way, allocated);
        split(half_way, end, allocated);
    } else if (allocated == end) {
      // dbg_print("finished");
    } else {
        //cs550_assert(0);
    }
}
size_t getsize(int size)
{
	size_t i=2;
	
	for (int j=1;j<size;j++)
	{
		i*=2;
	}
	return i;
}

static inline uint64_t
round_up_pow2(uint64_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}
extern void cs550_set_chunk_size(size_t size)
{

	chunk_size=size;
}

void init()
{
	for(int i=3;i<=max_blk_bit;i++)
	{
		freelist[i]=NULL;
	}
	init_status=1;	
}



void * malloc(size_t si)
{

	if(init_status==0)
	      init();
	struct node *blk;
	//int found=0;
	size_t size=round_up_pow2(si+blk_hdr_size);
	cs550_print("mallocing for size%lu\n",size);
	size_t bits=mylog2(size);
	printf("freelist[%lu]:0x%02x\n",bits,freelist[bits]); 
	/*size_t i;
	if(bits>31)
		return NULL;
	for (i=31;i>bits;i--)
	{
		if(freelist[i]&&!freelist[bits])
		{found=1;
		//printf("found free%d\n",i);
		break;
	}
	}		
	 if(!found)
	{*/
      if(!freelist[bits]){ 
		if(!chunk_size)
		cs550_set_chunk_size(1024*1024*512);
		cs550_print("nooooooooooooooooooooooooooooo_free chunk_size%lu\n",chunk_size);
		void *begin=cs550_mmap_wrapper(chunk_size);
		void *end=(char *)begin+chunk_size;
		blk=begin;
		blk->log2_capacity=bits;
		char *allocated=(char *)begin+size;
			//cs550_print("bits%lu",blk->log2_capacity);	
		split(begin,end,allocated);	
		
			//cs550_print("freelist%lu\n",freelist[bits]);	
		return (char *)blk+blk_hdr_size;
	}
	
	else if(freelist[bits])
	{
	
		blk=freelist[bits];
		blk->log2_capacity=bits;
	//	printf("freelist[%lu]->next:0x%02x\n",bits,freelist[bits]->next); 
		if(freelist[bits]->next)
			freelist[bits]=freelist[bits]->next;
		else if(!freelist[bits]->next) freelist[bits]=NULL;
		return (char*)blk+blk_hdr_size;
	}
	
	
/*	else if(found)
		{
			void *begin=freelist[i];
			void *end=(char *)begin+getsize(i);
			struct node *head=begin;
			head->log2_capacity=bits;
			char *allocated=(char *)begin+getsize(bits);
			freelist[i]=freelist[i]->next;
				//cs550_print("bits%lu",blk->log2_capacity);	
			split(begin,end,allocated);	
		//	head->log2_capacity=bits_request;
	 		return (char *)begin+blk_hdr_size;
		}	*/
		
}
void free(void *p)
{
	//cs550_print("calling free\n");
	
	if(!p)
		return;
	 struct node *head=(struct node *)((char *)p-blk_hdr_size);
	size_t cap=head->log2_capacity;
	//printf("freelist[%lu]:0x%02x\n",cap,freelist[cap]); 
	//printf("head:0x%02x\n",head);
	if(head==freelist[cap])
	return;
	
	if(!freelist[cap])
	{
		
		freelist[cap]=head;
		freelist[cap]->next=0;
		//printf("freelist=0,,freelist[%lu]->next:0x%02x\n",cap,freelist[cap]->next);
	}
	
	else if(freelist[cap])
	{
		struct node *temp;
                temp=head;
		temp->next=freelist[cap];
		
		freelist[cap]=temp;
		//printf("freelist=1,,freelist[%lu]->next:0x%02x\n",cap,freelist[cap]->next);
	}
}

void *calloc(size_t nmemb, size_t size)
{
	//cs550_print("calling calloc\n");
	if(!nmemb&&!size)
		return NULL;
	else return malloc(nmemb*size);
}


void *realloc(void *ptr, size_t size)
{
	//cs550_print("calling realloc\n");
		struct node *head=(struct node *)((char *)ptr-blk_hdr_size);

	size_t bits_request=mylog2(round_up_pow2(size+blk_hdr_size));
	size_t bits_ori=head->log2_capacity;
	  //cs550_print("bits_ori:%lu |||bits_request%lu\n",bits_ori,bits_request);
        
	if(bits_request<=bits_ori)
	{
		void *begin=head;
		void *end=(char *)begin+getsize(bits_ori);
		head->log2_capacity=bits_request;
		char *allocated=(char *)begin+getsize(bits_request);
			//cs550_print("bits%lu",blk->log2_capacity);	
		split(begin,end,allocated);	
	//	head->log2_capacity=bits_request;
 		return ptr;
		}
	else if(bits_request>bits_ori)
	{
		
		void *new=malloc(size);
		size_t actsz = getsize(bits_ori)-blk_hdr_size;
		memcpy(new,ptr,actsz);
		free(ptr);
		return new;
		}		
}
