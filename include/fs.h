/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _FS_H_
#define _FS_H_

#define FS_MAX_BUF_LEN  (1024*1024)
#define FS_MAX_NAME_LEN 255

#define ATTR_rd		(1<<0)
#define ATTR_wr		(1<<1)
#define ATTR_dir	(1<<2)

#define ROOTFID		0 /* Of any binding. */
#define ROOTATTR	(ATTR_wr|ATTR_rd|ATTR_dir) /* Of / */

enum {
  REQ_open, REQ_close,
  REQ_read, REQ_write,
  REQ_walk, REQ_create,
  REQ_remove, 
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

struct request_read {
  uint32_t offset;
  uint32_t len;
};

struct request_write {
  uint32_t offset;
  uint32_t len;
  /* Followed by len bytes of data to write. */
  uint8_t *buf;
};

struct request_create {
  uint32_t attr;
  uint32_t lname;
  /* Followed by lname bytes of data for name. */
  uint8_t *name;
};

struct response {
  uint32_t rid;
  int32_t ret;
	
  uint32_t lbuf;
  uint8_t *buf;
};

/* 
 * Response for walk is dir but is ordered as:
 * 	file1
 * 	filename1
 * 	file2
 * 	filename2
 * 	...
 * through to nfiles.
 *
 * Response to write has ret as err or OK. If OK then
 * buf contains the number of bytes that were written.
 * 
 * Response to read has ret as err or OK. If OK then 
 * buf contains the bytes that were read.
 *
 * Response to create has ret as err or OK. If OK then
 * buf contains the fid of the new file.
 *
 * Response to remove, open, close have no buf and
 * ret is an error or OK.
 */

struct file {
  uint32_t fid;
  uint32_t attr;
	
  uint32_t lname;
  uint8_t *name;
};

struct dir {
  uint32_t nfiles;
  struct file **files;
};

uint8_t *
dirtowalkresponse(struct dir *dir, uint32_t *size);
struct dir *
walkresponsetodir(uint8_t *buf, uint32_t len);

struct fsmount {
  void (*open)(struct request *, struct response *);
  void (*close)(struct request *, struct response *);
  void (*walk)(struct request *, struct response *);
  void (*read)(struct request *, struct response *);
  void (*write)(struct request *, struct response *);
  void (*remove)(struct request *, struct response *);
  void (*create)(struct request *, struct response *);
};

int
fsmountloop(int, int, struct fsmount *);

extern bool debugfs;

#endif
