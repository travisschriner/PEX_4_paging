#include <stdio.h>
#include <stdlib.h>
#include "byutr.h"
#include "structures.h"





int main(int argc, char **argv)
{

        lrustack* lrus = malloc(sizeof(lrustack*));
	FILE *ifp;
	unsigned long num_accesses=0;
	p2AddrTr trace_record;
	
	// If the number of arguments is wrong then quit
	if(argc!=2 && argc!=3)
	{
		fprintf(stderr,"usage: %s input_byutr_file [frame_size]\n",argv[0]);
		fprintf(stderr,"\nwhere frame_size is a digit corresponding to the following menu:\n\t1: 512 bytes\n\t2: 1KB\n\t3: 2KB\n\t4: 4KB\n");
		exit(1);
	}
	
	// If the file cannot be opened, then quit
	if((ifp = fopen(argv[1],"rb")) == NULL)
	{
		fprintf(stderr,"cannot open %s for reading\n",argv[1]);
		exit(1);
	}

	int menu_option=0;
	if(argc==3){
		menu_option=atoi(argv[2]);
	}
	while(menu_option<1 || menu_option>4){
		char input[128];		
		fprintf(stderr,"Select a frame size:\n\t1: 512 bytes\n\t2: 1KB\n\t3: 2KB\n\t4: 4KB\n");
		fgets(input,128,stdin);
		menu_option=atoi(input);
	}
	int OFFSET_BITS;
	int MAX_FRAMES;//THIS WILL BE USED AS THE SIZE OF YOUR ARRAY
	switch(menu_option){
		case 1:
			OFFSET_BITS=9;
			MAX_FRAMES=8192;
			break;
		case 2:
			OFFSET_BITS=10;
			MAX_FRAMES=4096;
			break;
		case 3:
			OFFSET_BITS=11;
			MAX_FRAMES=2048;
			break;
		case 4:
			OFFSET_BITS=12;
			MAX_FRAMES=1024;
			break;
	}
	
	//initialized my stack and faults
	initialize(lrus, MAX_FRAMES);
	
	unsigned long faults[MAX_FRAMES+1];

	int frame=0;
	for(frame=0; frame<MAX_FRAMES; frame++){
	  faults[frame] = 0;
	}

	while(!feof(ifp))
	//while(!feof(ifp) && i < 100)  //you may want to use this to debug
	{		
		//read next trace record		
		fread(&trace_record, sizeof(p2AddrTr), 1, ifp);
		//to get the page number, we shift the offset bits off of the address
		unsigned long page_num=trace_record.addr >> OFFSET_BITS;//THIS IS THE PAGE NUMBER THAT WAS REQUESTED!!!!!

		// this next line prints the page number that was referenced.
		// Note the use of %lu as it is an unsigned long!  Might be useful when debugging.
		//printf("%lu\n",page_num);

		num_accesses++;

		// more code possibly useful for debugging... gives an indication of progress being made
		//if((num_accesses % 100000) == 0){
		//	fprintf(stderr,"%lu samples read\r", num_accesses);
		//}

		//TODO: process each page reference
		int depth = seek_and_remove(lrus, page_num);
		if(depth == -1){
		  push(lrus, page_num);
		  for(frame = 1; frame <= MAX_FRAMES; frame++){
		    faults[frame]++;
		  }
		}else{
		  for(frame = 1; frame < depth; frame++){
		    faults[frame]++;
		  }
		}
	}//while(!feof...

	//TODO: find the number of page faults for each number of allocated frames
	printf("TOTAL:,%lu\n",num_accesses);
	printf("frames, misses, miss rate \n");
	for(frame = 1; frame <= MAX_FRAMES; frame++){
	  printf("%d,%lu,%f\n",frame,faults[frame],(double)faults[frame]/num_accesses);
	}

	fclose(ifp);

	return( 0 );
}




/*initialize the LRU stack */
void initialize(lrustack* lrus, unsigned int maxsize){
  lrus->head = NULL;
  lrus->tail = NULL;
  lrus->size = 0;
  lrus->maxsize = maxsize;
}




/* use pagenum when creating a new node, which will be pushed onto
   the LRU stack; make sure to keep track of the LRU stack's size
   and free and reset the tail as necessary to limit it to max size */
void push(lrustack* lrus, unsigned long pagenum){

  //initializes a new node, inserts data, and places at the top of the stack
  node* new = malloc(sizeof(node*));
  new->pagenum = pagenum;
  new->next = lrus->head;
  new->prev = NULL;

  //gets rid of the last node on the stack if stack is too tall
  /*put first so if the new node brings the stack to its maxsize,
    it does not delete the last one even though it should still
    be there*/
  if(lrus->size == lrus->maxsize){
    node* runFree = lrus->tail;
    lrus->tail = lrus->tail->prev; //tail->prev should exist if maxsize is 2 or greater
    lrus->tail->next = NULL;
    free(runFree);
  }

  //if lrus->head == NULL, new->next is already NULL
  if(lrus->head != NULL){
    lrus->head->prev = new;
  }
  //new head 
  lrus->head = new;

  /*if size = maxsize, no need to increment since we know
    the tail was already removed. If it is less, than we need
    to increment. This is the last thing we do in case the new 
    node we added brings the size to the maxsize. That way we 
    do not remove the tail every time we add the last node that 
    still fits in the stack. This would create a perpetual state
    where the stack could never fill up.*/
  if(lrus->size < lrus->maxsize){
    lrus->size++;
  }  
}



/* seek pagenum in lrus and remove it if found; return the depth
   at which pagenum was found or -1 if not. Will place removed 
   at top of stack if found*/
int seek_and_remove(lrustack* lrus, unsigned long pagenum){
  int depth = 1;
  node* scanner = lrus->head;
    while(scanner != NULL){

      if(scanner->pagenum == pagenum){
	node* above = scanner->prev;
	node* below = scanner->next;
	//middle  
	if(above != NULL && below != NULL){
	  above->next = below;
	  below->prev = above;
	  scanner->next = lrus->head;
	  scanner->prev = NULL;
	  lrus->head->prev = scanner;
	  lrus->head = scanner;
	}else if(above == NULL){ //pulling head & single node case
	  return 1; 
	}else if(below == NULL){//pulling tail (can only reach if a node is above this)
	  above->next = below; //NULL
	  scanner->next = lrus->head;
	  scanner->prev = NULL;
	  lrus->head->prev = scanner;
	  lrus->head = scanner;	  
	}else{
	  printf("\nThis isn't where I parked my car...\n");
	}	
	return depth;
      }else{
	scanner = scanner->next;
	depth++;
      }
    }//while
  return -1; //will hit if stack overflow or stack empty
}
