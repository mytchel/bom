/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _FSSRV_H_
#define _FSSRV_H_

#include <fs.h>

#define FSBUFMAX        1024

#define ROOTFID		0 /* Of any binding. */

enum {
  REQ_fid, REQ_clunk, REQ_stat,
  REQ_open, REQ_close,
  REQ_create, REQ_remove,
  REQ_read, REQ_write,
};

/* 
 * request/response->buf should be written/read after request/response 
 * has been written/read for any request/response that has lbuf > 0.
 */

/*
 * All names will be at most NAMEMAX-1 bytes long and will include a
 * terminating null byte. They may be shorted than NAMEMAX and so
 * requests and responses should take care to stop when they get to 
 * a null byte.
 */

struct request {
  uint32_t rid;
  uint32_t fid;
  uint8_t type;
  union {
    struct request_fid fid;
    struct request_clunk clunk;
    struct request_stat stat;
    struct request_open open;
    struct request_close close;
    struct request_create create;
    struct request_remove remove;
    struct request_read read;
    struct request_write write;
  } data;
};

struct response {
  uint32_t rid;
  int32_t ret;
  union {
    struct response_fid fid;
    struct response_clunk clunk;
    struct response_stat stat;
    struct response_open open;
    struct response_close close;
    struct response_create create;
    struct response_remove remove;
    struct response_read read;
    struct response_write write;
  } data;
};


struct request_fid {
  char name[NAMEMAX];
};

struct response_fid {
  uint32_t fid;
  uint32_t attr;
};


struct request_clunk {
  uint32_t fid;
};

struct response_clunk {};


struct request_stat {};
struct response_stat {
  struct stat stat;
};


struct request_open {};
struct response_open {};


struct request_close {};
struct response_close {};


struct request_create {
  uint32_t attr;
  char name[NAMEMAX];
};

struct response_create {
  /* ret is set to the created fid */
};


struct request_remove {};
struct response_remove {};


struct request_read {
  uint32_t offset;
  uint32_t len;
};

struct response_read {
  uint32_t len;
  uint8_t buf[FSBUFMAX]; /* Of len length. */
  /*
   * If the file is a directory then the result should 
   * be in the format 
   *    uint8_t name length [1, NAMEMAX] including null byte.
   *    uint8_t *name... 
   * for each file in the directory. 
   *
   */
};


struct request_write {
  uint32_t offset;
  uint32_t len;
  /* Followed by len bytes of data to write. */
  uint8_t data[FSBUFMAX];
};

struct response_write {
  /* ret is set to the number of bytes written. */
};

struct fsmount {
  void (*fid)(struct request *, struct response *);
  void (*clunk)(struct request *, struct response *);
  void (*stat)(struct request *, struct response *);
  void (*open)(struct request *, struct response *);
  void (*close)(struct request *, struct response *);
  void (*create)(struct request *, struct response *);
  void (*remove)(struct request *, struct response *);
  void (*read)(struct request *, struct response *);
  void (*write)(struct request *, struct response *);
};

int
fsmountloop(int, int, struct fsmount *);

#endif

