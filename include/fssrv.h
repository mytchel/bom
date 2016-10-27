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

#define ROOTFID		0 /* Of any binding. */

enum {
  REQ_getfid, REQ_clunk, REQ_stat,
  REQ_open, REQ_close,
  REQ_create, REQ_remove,
  REQ_read, REQ_write,
};


struct request_head {
  uint32_t rid;
  uint32_t fid;
  uint32_t type;
};

struct response_head {
  uint32_t rid;
  int32_t ret;
};


struct request_getfid_b {
  char name[NAMEMAX];
};
struct response_getfid_b {
  uint32_t fid;
  uint32_t attr;
};


struct request_clunk_b {
  uint32_t fid;
};
struct response_clunk_b {};


struct request_stat_b {};
struct response_stat_b {
  struct stat stat;
};


struct request_open_b {};
struct response_open_b {
  /* Min and max size of reads and writes */
  uint32_t minchunk;
  uint32_t maxchunk;
};


struct request_close_b {};
struct response_close_b {};


struct request_create_b {
  uint32_t attr;
  char name[NAMEMAX];
};
struct response_create_b {
  uint32_t fid;
};


struct request_remove_b {};
struct response_remove_b {};


struct request_read_b {
  uint32_t offset;
  uint32_t len;
};
struct response_read_b {
  /* Data should be sent in a seperate pipe write after head. */
  uint8_t *data;
  /* Used internally, do not send */
  uint32_t len;

  /*
   * If the file is a directory then the result should 
   * be in the format 
   *    uint8_t name length [1, NAMEMAX] including null byte.
   *    uint8_t *name... 
   * for each file in the directory. 
   *
   */
};


struct request_write_b {
  uint32_t offset;
  uint32_t len;
  /* Data should be read in a second pipe read */
  uint8_t *data;
};
struct response_write_b {
  uint32_t len;
};


struct request {
  struct request_head head;
  union {
    struct request_getfid_b getfid;
    struct request_clunk_b clunk;
    struct request_stat_b stat;
    struct request_open_b open;
    struct request_close_b close;
    struct request_create_b create;
    struct request_remove_b remove;
    struct request_read_b read;
    struct request_write_b write;
  };
};


struct response {
  struct response_head head;
  union {
    struct response_getfid_b getfid;
    struct response_clunk_b clunk;
    struct response_stat_b stat;
    struct response_open_b open;
    struct response_close_b close;
    struct response_create_b create;
    struct response_remove_b remove;
    struct response_read_b read;
    struct response_write_b write;
  };
};

struct request_getfid {
  struct request_head head;
  struct request_getfid_b body;
};

struct response_getfid {
  struct response_head head;
  struct response_getfid_b body;
};
 
struct request_clunk {
  struct request_head head;
  struct request_clunk_b body;
};

struct response_clunk {
  struct response_head head;
  struct response_clunk_b body;
};
 
struct request_stat {
  struct request_head head;
  struct request_stat_b body;
};

struct response_stat {
  struct response_head head;
  struct response_stat_b body;
};

struct request_open {
  struct request_head head;
  struct request_open_b body;
};

struct response_open {
  struct response_head head;
  struct response_open_b body;
};

struct request_close {
  struct request_head head;
  struct request_close_b body;
};

struct response_close {
  struct response_head head;
  struct response_close_b body;
};

struct request_create {
  struct request_head head;
  struct request_create_b body;
};

struct response_create {
  struct response_head head;
  struct response_create_b body;
};

struct request_remove {
  struct request_head head;
  struct request_remove_b body;
};

struct response_remove {
  struct response_head head;
  struct response_remove_b body;
};

struct request_read {
  struct request_head head;
  struct request_read_b body;
};

struct response_read {
  struct response_head head;
  struct response_read_b body;
};

struct request_write {
  struct request_head head;
  struct request_write_b body;
};

struct response_write {
  struct response_head head;
  struct response_write_b body;
};


struct fsmount {
  uint8_t *databuf;
  size_t buflen;

  void (*getfid)(struct request_getfid *,
		 struct response_getfid *);

  void (*clunk)(struct request_clunk *,
		struct response_clunk *);

  void (*stat)(struct request_stat *,
	       struct response_stat *);

  void (*open)(struct request_open *,
	       struct response_open *);

  void (*close)(struct request_close *,
		struct response_close *);

  void (*create)(struct request_create *,
		 struct response_create *);

  void (*remove)(struct request_remove *,
		 struct response_remove *);
  
  void (*read)(struct request_read *,
	       struct response_read *);

  void (*write)(struct request_write *,
		struct response_write *);
};

int
fsmountloop(int in, int out, struct fsmount *mount);

#endif

