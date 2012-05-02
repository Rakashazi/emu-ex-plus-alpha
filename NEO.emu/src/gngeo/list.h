#ifndef _LIST_H_
#define _LIST_H_

typedef struct LIST {
    void *data;
    struct LIST *next;
}LIST;

LIST *list_append(LIST *l,void *data);
LIST *list_prepend(LIST *l,void *data);
void list_foreach(LIST *l,void (*func)(void *data));
int list_nb_item(LIST *list);
LIST *list_get_item_by_index(LIST *list,int index);
void list_erase_all(LIST *list,void (*erasedata)(void *data));
LIST* list_insert_sort(LIST *list,void *data,int (*cmpdata)(void *a,void *b));
LIST* list_insert_sort_unique(LIST *list,void *data,int (*cmpdata)(void *a,void *b));

#endif
