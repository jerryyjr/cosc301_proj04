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


void list_append(ucontext_t *thread,struct node **head){
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

struct ucontext *list_pull(struct node **head){
	struct node *tmp=NULL;
	tmp=*head;
	ucontext_t *new=NULL;
	if(tmp==NULL){
		return NULL;
	}else{
		*head=(tmp->next);
		tmp->next=NULL;
		new=tmp->thd;
		free(tmp);
		return new;
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


struct node *fulllist=NULL;
struct node *ready=NULL;
static int waitallreturn=0;

static ucontext_t main_thread;
static ucontext_t current_context;

void ta_libinit(void) {
	
    return;
}



void ta_create(void (*func)(void *), void *arg) {
	
	ucontext_t *ctx;
	ctx=malloc(sizeof(ucontext_t));
	unsigned char *stack=(unsigned char*)malloc(STACKSIZE);
	getcontext(ctx);
	(*ctx).uc_stack.ss_sp=stack;
	(*ctx).uc_stack.ss_size=STACKSIZE;
	(*ctx).uc_link=&main_thread;
	makecontext(ctx, (void(*)(void))func,1,arg);
	list_append(ctx,&fulllist);
	current_context=*ctx;
	list_append(ctx, &ready);
	
	
    return;
}

void ta_yield(void) {
	ucontext_t current=current_context;
	if (ready!=NULL){
		ucontext_t *new=NULL;
		new=(list_pull(&ready));
		list_append(&current,&ready);
		current_context=*new;
		
		swapcontext(&current, new);
	}else{
		current_context=main_thread;
		list_append(&current,&ready);
		swapcontext(&current,&main_thread);
		
	}
	
    return;
}

int ta_waitall(void) {
	if(fulllist!=NULL){
		while(ready!=NULL){
			ucontext_t *new=NULL;
			new=list_pull(&ready);
			current_context=*new;
			swapcontext(&main_thread,new);
		}
		list_destroy(fulllist);
		return waitallreturn==0?0:-1;
	}
	
    return -1;
}


/* ***************************** 
     stage 2 library functions
   ***************************** */

void ta_sem_init(tasem_t *sem, int value) {
	sem->block=NULL;
	sem->num=value;
	return;
}

void ta_sem_destroy(tasem_t *sem) {
	list_destroy(sem->block);
	return;
}

void ta_sem_post(tasem_t *sem) {
	sem->num++;
	if((sem->block)!=NULL){
		ucontext_t *new=NULL;
		new=list_pull(&(sem->block));
		waitallreturn--;
		list_append(new,&ready);
		//ta_yield();
	}
	return;
}

void ta_sem_wait(tasem_t *sem) {
	while((sem->num)<=0){
		ucontext_t curr=current_context;
		if(ready==NULL){
			current_context=main_thread;
			waitallreturn++;
			swapcontext(&curr,&main_thread);
		}else{
			
			list_append(&current_context,&(sem->block));
			waitallreturn++;
			ucontext_t *new=NULL;
			new=list_pull(&ready);
			current_context=*new;
			swapcontext(&curr,new);
		}
	}
	(sem->num)--;
	return;
}

void ta_lock_init(talock_t *mutex) {
	tasem_t *s=NULL;
	s=malloc(sizeof(tasem_t));
	mutex->sem=s;
	ta_sem_init((mutex->sem),1);
	return;
}

void ta_lock_destroy(talock_t *mutex) {
	ta_sem_destroy(mutex->sem);
	free(mutex->sem);
	return;
}

void ta_lock(talock_t *mutex) {
	ta_sem_wait(mutex->sem);
	return;
}

void ta_unlock(talock_t *mutex) {
	ta_sem_post(mutex->sem);
}


/* ***************************** 
     stage 3 library functions
   ***************************** */

void ta_cond_init(tacond_t *cond) {
	cond->block=NULL;
	return;
}

void ta_cond_destroy(tacond_t *cond) {
	free(cond->block);
	return;
}

void ta_wait(talock_t *mutex, tacond_t *cond) {
	ta_unlock(mutex);
	ucontext_t curr=current_context;
	if(ready==NULL){
		current_context=main_thread;
		waitallreturn++;
		swapcontext(&curr,&main_thread);
	}else{
			
		list_append(&current_context,&(cond->block));
		waitallreturn++;
		ucontext_t *new=NULL;
		new=list_pull(&ready);
		current_context=*new;
		swapcontext(&curr,new);
	}
	ta_lock(mutex);
}

void ta_signal(tacond_t *cond) {
	if(cond->block!=NULL){
		ucontext_t *new=list_pull(&(cond->block));
		waitallreturn--;
		list_append(new,&ready);
	}
}

