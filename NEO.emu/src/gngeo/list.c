#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "list.h"

LIST *list_get_item_by_index(LIST *list,int index) {
    int i=0;
    LIST *l;
    for (l=list;l;l=l->next) {
	if (i==index) return l;
	i++;
    }
    return NULL;
}

void list_erase_all(LIST *list,void (*erasedata)(void *data)) {
    LIST *l=list;
    do {
	l=list->next;
	//free(list->data);
	if (erasedata)
	    erasedata(list->data);
	free(list);
	list=l;
    } while(l);
}

static LIST* insert_sort(LIST *list,void *data,int (*cmpdata)(void *a,void *b),int unique) {
        LIST *t=list,*pt=NULL,*res=list;
    LIST *tmp=malloc(sizeof(list));
    tmp->data=data;

    for(t=list;t && cmpdata(data,t->data)>0;pt=t,t=t->next);
    if (t && unique && cmpdata(data,t->data)==0) return res;
    if (t) { 
	if (pt) { /* insersion inside the list */
	    tmp->next=t;
	    pt->next=tmp;
	} else { /* insersion at the begining of the list */
	    tmp->next=t;
	    res=tmp;
	}
    } else {
	if (pt) { /* insertion at the end of the list */
	    pt->next=tmp;
	    tmp->next=NULL;
	} else { /* first element */
	    res=tmp;
	    tmp->next=NULL;
	}
    }
    return res;
}

LIST* list_insert_sort(LIST *list,void *data,int (*cmpdata)(void *a,void *b)) {
    return insert_sort(list,data,cmpdata,0);
}
LIST* list_insert_sort_unique(LIST *list,void *data,int (*cmpdata)(void *a,void *b)) {
    return insert_sort(list,data,cmpdata,1);
}

int list_nb_item(LIST *list) {
    LIST *l;
    int i=0;
    for(l=list;l;l=l->next)
	i++;
    return i;
}

LIST *list_append(LIST *l,void *data) {
    LIST *t=malloc(sizeof(LIST));
    LIST *i;
    t->data=data;
    t->next=NULL;

    if (l==NULL)
	l=t;
    else {
	for(i=l;i->next!=NULL;i=i->next);
	i->next=t;
    }  
    return l;
}

LIST *list_prepend(LIST *l,void *data) {
    LIST *t=malloc(sizeof(LIST));
    t->data=data;
    t->next=l;
    l=t;
    return l;
}

void list_foreach(LIST *l,void (*func)(void *data)) {
    LIST *i;
    for(i=l;i;i=i->next)
	func(i->data);
}

#ifdef TEST_LIST

void print_data(void *data) {
    char *name=data;
    printf("DATA: %s\n",name);
}

int main(void) {
    LIST *list=NULL;
    char *a="hello";
    char *b="bye";
    char *c="toto";

    list=list_prepend(list,"yo!");
    list=list_append(list,a);
    list=list_append(list,b);
    list=list_append(list,c);
    list=list_prepend(list,"plop");
    list_foreach(list,print_data);
    return 0;
}
#endif
