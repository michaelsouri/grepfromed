/* GREP FROM ED - MICHAEL SOURI - 3/29/20 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <unistd.h>
#include <fcntl.h>
void readfile(const char *c);
void ungetch_(int c);
void commands(void);
void search(const char* c);
void process_dir(const char* dir, const char* searchfor, void(*fp)(const char*, const char*));
void search_file(const char* filename, const char* searhfor);
void print(void);
int  advance(char *lp, char *ep);
int  append(int (*f)(void), unsigned int *a);
void blkio(int b, char *buf, long (*iofcn)(int, void*, unsigned long));
int  cclass(char *set, int c, int af);
void compile(int eof);
void error(char *s);
int  execute(unsigned int *addr);
char *getblock(unsigned int atl, int iof);
int  getchr(void);
int  getfile(void);
char *getline_blk(unsigned int tl);
void global(int k);
void newline(void);
void putchr(int ac);
void putd(void);
int  putline(void);
void puts_(char *sp);
