/* GREP FROM ED - MICHAEL SOURI - 3/29/20 */
#include "grep.h"
const int BLKSIZE = 40960, NBLK = 2047, FNSIZE = 128, LBSIZE = 40960, ESIZE = 256, GBSIZE = 256, NBRA = 5;
const int KSIZE = 9, CBRA = 1, CCHR = 2, ADOT = 4, CCL = 6, NCCL = 8, CDOL = 10, CEOF = 11, CKET = 12, CBACK = 14;
const int CCIRC = 15, STAR = 01, READ = 0, WRITE = 1, BUFSIZE = 100, CDOT = 4;
int  peekc, lastc, given, ninbuf, io, pflag, vflag = 1, oflag, listf, listn, col, tfile = -1, tline, iblock  = -1;
int  oblock  = -1, ichanged, nleft, names[26], anymarks, nbra, subnewa, subolda, fchange, wrapp, bpagesize = 20, bufp = 0;
unsigned nlall = 128;
unsigned int *addr1, *addr2, *dot, *dol, *zero;
long count;
char Q[] = "", inputbuf[GBSIZE], T[] = "TMP", savedfile[FNSIZE], file[FNSIZE], linebuf[LBSIZE], rhsbuf[LBSIZE/2], expbuf[ESIZE+4], genbuf[LBSIZE];
char *nextip, *linebp, *globp, *mktemp(char *), tmpXXXXX[50] = "/tmp/eXXXXX", *tfname, *loc1, *loc2, ibuff[BLKSIZE], obuff[BLKSIZE];
char WRERR[]  = "WRITE ERROR", *braslist[NBRA], *braelist[NBRA], line[70],  *linp = line, grepbuf[GBSIZE], buf[BUFSIZE];
int main(int argc, const char *argv[]) {
  zero = (unsigned *)malloc(nlall * sizeof(unsigned));
  tfname = mkdtemp(tmpXXXXX);
  if (argc < 3) {
    printf("Usage: ./grep [OPTION]... PATTERNS [FILE]...\n");
    exit(1);
  }
  for (int i = 2; i < argc; ++i) { process_dir(argv[i], argv[1], search_file); }
  //quit(0);
  exit(0);
  return 0;
}
void commands(void) {
  int c; char lastsep;
  for (;;) {
    unsigned int* a1;
    if (pflag) {
      pflag = 0;
      addr1 = addr2 = dot;
      print();
    }
    c = '\n';
    for (addr1 = 0;;) {
      lastsep = c;
      a1 = 0;
      c = getchr();
      if (c != ',' && c != ';') { break; }
      if (a1==0) {
        a1 = zero+1;
        if (a1 > dol) { a1--; }
      }
      addr1 = a1;
      if (c == ';') { dot = a1; }
    }
    if (lastsep != '\n' && a1 == 0) { a1 = dol; }
    if ((addr2 = a1) == 0) { given = 0;  addr2 = dot;  } else { given = 1; }
    if (addr1 == 0) { addr1 = addr2; }
    switch(c) {
      case 'p':
      case 'P': newline(); print(); break;
      case EOF: default: return;
    }
  }
}
void readfile(const char* c) {
  strcpy(file, c);
  strcpy(savedfile, c);
  memset(inputbuf, 0, sizeof(inputbuf));
  tfile = open(tfname, 2);  dot = dol = zero;
  addr2 = zero;
  if ((io = open((const char*)file, 0)) < 0) { lastc = '\n'; }
  append(getfile, addr2);
  close(io);
  io = -1;
  fchange = *c;
}
void search(const char* c) {
  char buf[GBSIZE];
  snprintf(buf, sizeof(buf), "/%s\n", c);
  const char* p = buf + strlen(buf) - 1;
  while (p >= buf) { ungetch_(*p--); }
  global(1);
}
void process_dir(const char* dir, const char* searchfor, void(*fp)(const char*, const char*)) {
  if (strchr(dir, '*') == NULL) { search_file(dir, searchfor); return; }
  glob_t results;
  memset(&results, 0, sizeof(results));
  glob(dir, 0, NULL, &results);
  for (int i = 0; i < results.gl_pathc; ++i) {
    const char* filename = results.gl_pathv[i];
    fp(filename, searchfor);
  }
  globfree(&results);
}
void search_file(const char* filename, const char* searchfor) { readfile(filename); search(searchfor); }
int  getch_(void) { return (bufp > 0) ? buf[--bufp] : getchar(); }
void ungetch_(int c) {
  if (bufp >= BUFSIZE) { printf("ungetch: too many chars\n"); }
  else { buf[bufp++] = c; }
}
void puts_nonewline(char *sp) {
  while (*sp) { putchr(*sp++); }
}
void print(void) {
  unsigned int *a1 = addr1;
  char buf[BUFSIZ];
  while (a1 <= addr2) {
    snprintf(buf, sizeof(buf), "%s: ", file); puts_nonewline(buf);
    puts_(getline_blk(*a1++));
  }
}
int advance(char *lp, char *ep) {
  char *curlp;
  int i;
  for (;;) {
    switch (*ep++) {
      case CCHR:  if (*ep++ == *lp++) { continue; } return(0);
      case CDOT:  if (*lp++) { continue; }    return(0);
      case CDOL:  if (*lp==0) { continue; }  return(0);
      case CEOF:  loc2 = lp;  return(1);
      case CCL:   if (cclass(ep, *lp++, 1)) {  ep += *ep;  continue; }  return(0);
      case NCCL:  if (cclass(ep, *lp++, 0)) { ep += *ep;  continue; }  return(0);
      case CBRA:  braslist[*ep++] = lp;  continue;
      case CKET:  braelist[*ep++] = lp;  continue;
      case CBACK:
      case CBACK|STAR:
        curlp = lp;
        while (lp >= curlp) {
          if (advance(lp, ep)) { return(1); }
          lp -= braelist[i] - braslist[i];
        } continue;
      case CDOT|STAR:  curlp = lp;
      case CCHR|STAR:  curlp = lp; ++ep;
      case CCL|STAR:
      case NCCL|STAR:  curlp = lp; ep += *ep;
      default: return 1;
    }
  }
}
int append(int (*f)(void), unsigned int *a) {
  unsigned int *a1, *a2, *rdot;
  int tl, nline = 0;
  dot = a;
  while ((*f)() == 0) {
    if ((dol-zero)+1 >= nlall) {
      unsigned *ozero = zero;
      nlall += 1024;
      dot += zero - ozero;
      dol += zero - ozero;
    }
    tl = putline();
    nline++;
    a1 = ++dol;
    a2 = a1+1;
    rdot = ++dot;
    while (a1 > rdot) { *--a2 = *--a1; }
    *rdot = tl;
  }
  return(nline);
}
void blkio(int b, char *buf, long (*iofcn)(int, void*, unsigned long)) {
  lseek(tfile, (long)b*BLKSIZE, 0);
}
void compile(int eof) {
  int c, cclcnt;
  char *ep = expbuf, *lastep, bracket[NBRA], *bracketp = bracket;
  if ((c = getchr()) == '\n') { peekc = c;  c = eof; }
  if (c == eof)  { return;}
  nbra = 0;
  if (c=='^') {
    c = getchr();
    *ep++ = CCIRC;
  }
  peekc = c;
  lastep = 0;
  for (;;) {
    c = getchr();
    if (c == '\n') {
      peekc = c;
      c = eof;
    }
    if (c==eof) { *ep++ = CEOF;  return; }
    if (c!='*') { lastep = ep; }
    switch (c) {
      case '\\': if ((c = getchr())=='(') {
                    *bracketp++ = nbra;
                    *ep++ = CBRA;
                    *ep++ = nbra++;
                    continue;
                 }
                 if (c == ')') {
                    *ep++ = CKET;
                    *ep++ = *--bracketp;
                    continue;
                 }
                 if (c>='1' && c<'1'+NBRA) {
                    *ep++ = CBACK;
                    *ep++ = c-'1';
                    continue;
                 }
                 *ep++ = CCHR;
                 *ep++ = c;
                 continue;
      case '.': *ep++ = CDOT; continue;
      case '\n':
      case '*':  *lastep |= STAR; continue;
      case '$':  *ep++ = CDOL;  continue;
      case '[':  *ep++ = CCL;  *ep++ = 0;  cclcnt = 1;
                 if ((c=getchr()) == '^') {
                   c = getchr();
                   ep[-2] = NCCL;
                 }
                 do {
                   if (c=='-' && ep[-1]!=0) {
                     if ((c=getchr())==']') {
                       *ep++ = '-';
                       cclcnt++;
                       break;
                     }
                     while (ep[-1] < c) {
                       *ep = ep[-1] + 1;
                       ep++;
                       cclcnt++;
                     }
                   }
                   *ep++ = c;
                   cclcnt++;
                 } while ((c = getchr()) != ']');
                 lastep[1] = cclcnt;
                 continue;
      default:  *ep++ = CCHR;  *ep++ = c;
    }
  }
}
int execute(unsigned int *addr) {
  char *p1, *p2 = expbuf;
  int c;
  for (c = 0; c < NBRA; c++) {
    braslist[c] = 0;
    braelist[c] = 0;
  }
  if (addr == (unsigned *)0) {
    if (*p2 == CCIRC) { return(0); }
    p1 = loc2;
  } else if (addr == zero) { return(0); }
    else { p1 = getline_blk(*addr); }
  if (*p2 == CCIRC) {
    loc1 = p1;
    return(advance(p1, p2+1));
  }
  if (*p2 == CCHR) {
    c = p2[1];
    do {  if (*p1 != c) { continue; }
    if (advance(p1, p2)) {
      loc1 = p1;
      return(1);
    }
    } while (*p1++);
    return(0);
  }
  do {
    if (advance(p1, p2)) {
      loc1 = p1;
      return(1);
    }
  } while (*p1++);
  return(0);
}
char* getblock(unsigned int atl, int iof) {
  int off = (atl<<1) & (BLKSIZE-1) & ~03, bno = (atl/(BLKSIZE/2));
  if (bno >= NBLK) {
    lastc = '\n';
  }
  nleft = BLKSIZE - off;
  if (bno==iblock) {
    ichanged |= iof;
    return(ibuff+off);
  }
  if (bno==oblock) { return(obuff+off); }
  if (iof==READ) {
    if (ichanged) { blkio(iblock, ibuff, (long (*)(int, void*, unsigned long))write); }
    ichanged = 0;
    iblock = bno;
    blkio(bno, ibuff, read);
    return(ibuff+off);
  }
  if (oblock>=0) { blkio(oblock, obuff, (long (*)(int, void*, unsigned long))write); }
  oblock = bno;
  return(obuff+off);
}
int getchr(void) {
  int c;
  if ((lastc=peekc)) {
    peekc = 0;
    return(lastc);
  }
  if (globp) {
    if ((lastc = *globp++) != 0) { return(lastc); }
    globp = 0;
    return(EOF);
  }
  if ((c = getch_()) <= 0) { return(lastc = EOF); }
  lastc = c&0177;
  return(lastc);
}
int getfile(void) {
  int c;
  char *lp = linebuf, *fp = nextip;
  do {
    if (--ninbuf < 0) {
      if ((ninbuf = (int)read(io, genbuf, LBSIZE)-1) < 0) {
        if (lp>linebuf) {
          *genbuf = '\n';
        } else { return(EOF); }
      }
      fp = genbuf;
      while(fp < &genbuf[ninbuf]) {
        if (*fp++ & 0200) { break; }
      }
      fp = genbuf;
    }
    c = *fp++;
    if (c=='\0') { continue; }
    if (c&0200 || lp >= &linebuf[LBSIZE]) { lastc = '\n'; }
    *lp++ = c;
    count++;
  } while (c != '\n');
  *--lp = 0;
  nextip = fp;
  return(0);
}
char* getline_blk(unsigned int tl) {
  char *bp, *lp;
  int nl;
  lp = linebuf;
  bp = getblock(tl, READ);
  nl = nleft;
  tl &= ~((BLKSIZE/2)-1);
  while ((*lp++ = *bp++)) {
    if (--nl == 0) {
      bp = getblock(tl+=(BLKSIZE/2), READ);
      nl = nleft;
    }
  }
  return(linebuf);
}
void global(int k) {
  char *gp;
  int c;
  unsigned int *a1;
  char globuf[GBSIZE];
  if (!given) { addr1 = zero + (dol > zero);  addr2 = dol; }
  if ((c = getchr()) == '\n') { }
  compile(c);
  gp = globuf;
  while ((c = getchr()) != '\n' && (c = getchr()) != EOF) {
    *gp++ = c;
  }
  if (gp == globuf) { *gp++ = 'p'; }
  *gp++ = '\n';
  *gp++ = 0;
  for (a1 = zero; a1 <= dol; a1++) {
    *a1 &= ~01;
    if (a1 >= addr1 && a1 <= addr2 && execute(a1) == k) { *a1 |= 01; }
  }
  for (a1 = zero; a1 <= dol; a1++) {
    if (*a1 & 01) {
      *a1 &= ~01;
      dot = a1;
      globp = globuf;
      commands();
      a1 = zero;
    }
  }
}
void newline(void) {
  int c;
  if ((c = getchr()) == '\n' || c == EOF) { return; }
  if (c == 'p' || c == 'l' || c == 'n') {
    pflag++;
    if (c == 'l') { listf++; }
    else if (c == 'n') { listn++; }
    if ((c = getchr()) == '\n') { return; }
  }
}
int cclass(char *set, int c, int af) {
  int n;
  n = *set++;
  while (--n)
    if (*set++ == c) { return(af); }
  return(!af);
}
void putchr(int ac) {
  char *lp = linp;
  int c = ac;
  if (listf) {
    if (c == '\n') {
      if (linp != line && linp[-1]==' ') {
        *lp++ = '\\';
        *lp++ = 'n';
      }
    } else {
      if (col > (72 - 4 - 2)) {
        col = 8;
        *lp++ = '\\';
        *lp++ = '\n';
        *lp++ = '\t';
      }
      col++;
      if (c == '\b' || c == '\t' || c == '\\') {
        *lp++ = '\\';
        if (c == '\b') { c = 'b'; }
        else if (c == '\t') { c = 't'; }
        col++;
      } else if (c < ' ' || c == '\177') {
        *lp++ = '\\';
        *lp++ =  (c >> 6) +'0';
        *lp++ = ((c >> 3) & 07) + '0';
        c = (c & 07) + '0';
        col += 3;
      }
    }
  }
  *lp++ = c;
  if (c == '\n' || lp >= &line[64]) {
    linp = line;
    write(oflag ? 2 : 1, line, lp - line);
    return;
  }
  linp = lp;
}
int putline(void) {
  char *bp, *lp;
  int nl, tl;
  lp = linebuf;
  tl = tline;
  bp = getblock(tl, WRITE);
  nl = nleft;
  tl &= ~((BLKSIZE/2)-1);
  while ((*bp = *lp++)) {
    if (*bp++ == '\n') {
      *--bp = 0;
      linebp = lp;
      break;
    }
    if (--nl == 0) {
      bp = getblock(tl += (BLKSIZE/2), WRITE);
      nl = nleft;
    }
  }
  nl = tline;
  tline += (((lp - linebuf) + 03) >> 1) & 077776;
  return(nl);
}
void puts_(char *sp) {
  col = 0;
  while (*sp) { putchr(*sp++); }
  putchr('\n');
}
