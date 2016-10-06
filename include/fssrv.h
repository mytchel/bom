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

#define FS_LBUF_MAX     1024

#define ROOTFID		0 /* Of any binding. */

extern bool debugfs;

enum {
  REQ_fid, REQ_stat,
  REQ_open, REQ_close,
  REQ_create, REQ_remove,
  REQ_read, REQ_write,
  REQ_flush,
};

/* 
 * request/response->buf should be written/read after request/response 
 * has been written/read for any request/response that has lbuf > 0.
 */

struct request {
  uint32_t rid;
  uint32_t type;
  uint32_t fid;
  uint32_t lbuf;
  /* Followed by lbuf bytes of data for request */
  uint8_t *buf;
};

struct request_fid {
  uint8_t *name;
  /* Of length lbuf for request */
};

struct request_create {
  uint32_t attr;
  uint8_t lname;
  /* Followed by lname bytes of data for name. */
  uint8_t *name;
};

struct request_read {
  uint32_t offset;
  uint32_t len;
};

struct request_write {
  uint32_t offset;
  /* Followed by (lbuf - sizeof(uint32_t)) bytes of data to write. */
  uint8_t *buf;
};

/* Requests for open, flush, and remove have no buffer. */

struct response {
  uint32_t rid;
  int32_t ret;
	
  uint32_t lbuf;
  uint8_t *buf;
};

/*
 * Response to write has ret as err or OK. If OK then
 * buf contains the number of bytes that were written.
 * 
 * Response to read has ret as err or OK. If OK then 
 * buf contains the bytes that were read.
 * If the file is a directory then the result should 
 * be in the format 
 *    uint8_t name length [0, FS_NAME_MAX-1]
 *    uint8_t *name... (If reading from userspace you should exspect this to be
 *             up to FS_NAME_MAX bytes.)
 * for each file in the directory. You will only get
 * one response for each read.
 *
 * Respose for stat should be a stat structure for the fid.
 * 
 * Response to create has ret as err or OK. If OK then
 * buf contains the fid of the new file.
 *
 * Response to remove, open, close, and flush have no buf and
 * ret is an error or OK.
 */

struct fsmount {
  void (*fid)(struct request *, struct response *);
  void (*stat)(struct request *, struct response *);
  void (*open)(struct request *, struct response *);
  void (*close)(struct request *, struct response *);
  void (*create)(struct request *, struct response *);
  void (*remove)(struct request *, struct response *);
  void (*read)(struct request *, struct response *);
  void (*write)(struct request *, struct response *);
  void (*flush)(struct request *, struct response *);
};

int
fsmountloop(int, int, struct fsmount *);

#endif

