/*
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <ucontext.h>

#include "threadsalive.h"

#define STACKSIZE 16384

/* ***************************** 
     stage 1 library functions
   ***************************** */
struct node{
	int thd;
	struct node *next;
};

void list_append(int thread,struct node **head){
	struct node *new=malloc(sizeof(struct node));
	new->thd=thread;
	new->next=NULL;
	struct node *tmp=*head;

	if(tmp==NULL){
		*head=new;
	}else{
		while(tmp->next!=NULL){
			tmp=tmp->next;
		}
		tmp->next=new;
	}
	return;
}

struct node *list_pull(struct node **head){
	struct node *tmp=*head;
	
	if(tmp==NULL){
		return NULL;
	}else{
		*head=(tmp->next);
		tmp->next=NULL;
		return tmp;
	}
}



void list_destroy(struct node *head){
	while (head!=NULL){
		struct node *tmp=head;
		head=head->next;
		free(tmp);
	}
}

/*  queue helper function end*/


//struct node *fulllist=NULL;
struct node *ready=NULL;


int *arraylen=1;
int *curr=0;

ucontext_t ctxarray[1];
int current_thd=0;
//static ucontext_t main_thread;
//static ucontext_t current_context;

void ta_libinit(void) {
	//list_append(&main_thread,&ready);
    return;
}

/*void update_ctx(ucontext_t *ctxarray,ucontext_t *ctx, int *arraylen){
	int a=(*arraylen)++;
	ucontext_t *tmp=NULL;
	tmp=ctxarray;
	ucontext_t new[a];
	for (int i=0; i<*arraylen-1;i++){
		new[i]=ctxarray[i];
	}
	new[*arraylen]=*ctx;
	ctxarray=new;
	free(tmp);
	return;
}*/

void ta_create(void (*func)(void *), void *arg) {
	
	ucontext_t ctx;
	ucontext_t *contx=&ctx;
	unsigned char *stack=(unsigned char*)malloc(STACKSIZE);
	getcontext(&ctx);
	(ctx).uc_stack.ss_sp=stack;
	(ctx).uc_stack.ss_size=STACKSIZE;
	(ctx).uc_link=&ctxarray[0];
	makecontext(&ctx, (void(*)(void))func,1,arg);
	//list_append(&ctx,&fulllist);
	//current_context=ctx;
	list_append(*arraylen, &ready);
	update_ctx(ctxarray,contx,arraylen);
	
	printf("JJ");
	//swapcontext(&main_thread, &ctx);
	
    return;
}

void ta_yield(void) {
	int current=*curr;
	//if (ready!=NULL){
		
		int new=(list_pull(&ready)->thd);
		list_append(new,&ready);
		*curr=new;
		
		swapcontext(&ctxarray[current],&ctxarray[new] );
	/*}else{
		current_context=main_thread;
		list_append(&current,&ready);
		swapcontext(&current,&main_thread);
		
	}*/
	printf("SWAP");
    return;
}

int ta_waitall(void) {
	if(*arraylen>1){
		while(ready!=NULL){
			
			
			struct node *tmp;
			tmp=list_pull(&ready);
			int new=(tmp->thd);
			*curr=new;
			printf("SWA");
			swapcontext(&ctxarray[0],&ctxarray[*curr]);
		
		}
		//list_destroy(fulllist);
		return 0;
	}
	
    return -1;
}


/* ***************************** 
     stage 2 library functions
   ***************************** */

void ta_sem_init(tasem_t *sem, int value) {
}

void ta_sem_destroy(tasem_t *sem) {
}

void ta_sem_post(tasem_t *sem) {
}

void ta_sem_wait(tasem_t *sem) {
}

void ta_lock_init(talock_t *mutex) {
}

void ta_lock_destroy(talock_t *mutex) {
}

void ta_lock(talock_t *mutex) {
}

void ta_unlock(talock_t *mutex) {
}


/* ***************************** 
     stage 3 library functions
   ***************************** */

void ta_cond_init(tacond_t *cond) {
}

void ta_cond_destroy(tacond_t *cond) {
}

void ta_wait(talock_t *mutex, tacond_t *cond) {
}

void ta_signal(tacond_t *cond) {
}

