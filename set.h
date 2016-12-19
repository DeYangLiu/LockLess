#ifndef _SET_H_
#define _SET_H_

int set_init(void);
void set_term(void);


int set_add(int key);
int set_remove(int key);
int set_contains(int key);

#endif
