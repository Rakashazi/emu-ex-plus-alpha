#ifndef _PBAR_H_
#define _PBAR_H_

void create_progress_bar(const char *desc);
void update_progress_bar(Uint32 current_pos,Uint32 size);
void terminate_progress_bar(void);
#ifdef GP2X
void init_progress_bar(char *desc);
#endif
#endif
