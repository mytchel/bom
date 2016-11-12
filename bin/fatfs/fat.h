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

struct fat_bs_ext32 {
  uint8_t spf[4];
  uint8_t extflags[2];
  uint8_t version[2];
  uint8_t rootcluster[4];
  uint8_t info[2];
  uint8_t backup[2];
  uint8_t res_0[12];
  uint8_t drvn;
  uint8_t res_1;
  uint8_t sig;
  uint8_t volid[4];
  uint8_t vollabel[11];
  uint8_t fattypelabel[8];
  uint8_t bootcode[420];
}__attribute__((packed));


struct fat_bs_ext16
{
  uint8_t drvn;
  uint8_t res_1;
  uint8_t sig;
  uint8_t volid[2];
  uint8_t vollabel[11];
  uint8_t fattypelabel[8];
  uint8_t bootcode[448];
}__attribute__((packed));


struct fat_bs {
  uint8_t jmp[3];
  uint8_t oem[8];
  uint8_t bps[2];
  uint8_t spc;
  uint8_t res[2];
  uint8_t nft;
  uint8_t rde[2];
  uint8_t sc16[2];
  uint8_t mdt;
  uint8_t spf[2];
  uint8_t spt[2];
  uint8_t heads[2];
  uint8_t hidden[4];
  uint8_t sc32[4];

  union {
    struct fat_bs_ext32 high;
    struct fat_bs_ext16 low;
  } ext;
  
  uint8_t boot_signature[2];

}__attribute__((packed));


#define FAT_ATTR_read_only        0x01
#define FAT_ATTR_hidden           0x02
#define FAT_ATTR_system           0x04
#define FAT_ATTR_volume_id        0x08
#define FAT_ATTR_directory        0x10
#define FAT_ATTR_archive          0x20
#define FAT_ATTR_lfn \
  (FAT_ATTR_read_only|FAT_ATTR_hidden| \
   FAT_ATTR_system|FAT_ATTR_volume_id)


struct fat_lfn {
  uint8_t order;
  uint8_t first[10];
  uint8_t attr;
  uint8_t type;
  uint8_t chksum;
  uint8_t next[12];
  uint8_t zero[2];
  uint8_t final[4];
}__attribute__((packed));


struct fat_dir_entry {
  uint8_t name[8];
  uint8_t ext[3];
  uint8_t attr;
  uint8_t reserved1;
  uint8_t create_dseconds;
  uint8_t create_time[2];
  uint8_t create_date[2];
  uint8_t last_access[2];
  uint8_t cluster_high[2];
  uint8_t mod_time[2];
  uint8_t mod_date[2];
  uint8_t cluster_low[2];
  uint8_t size[4];
}__attribute__((packed));


struct fat_file {
  uint8_t refs;
  char name[NAMEMAX];
  
  uint32_t attr;
  uint32_t size;
  uint32_t dsize;

  uint32_t startcluster;

  uint32_t direntrysector;
  uint32_t direntryoffset;

  struct fat_file *parent;
};


#define BUFSMAX 16

struct buf {
  uint8_t *addr;
  uint32_t sector;
  size_t n;
  size_t uses;
};

typedef enum { FAT16, FAT32 } fat_t;

#define FAT16_END 0xfff8
#define FAT32_END 0x0ffffff8

#define FIDSMAX 32

struct fat {
  int fd;
  fat_t type;

  struct fat_file files[FIDSMAX];

  uint32_t bps;
  uint32_t spc;

  uint32_t spf;
  uint8_t nft;

  uint32_t nclusters;
  uint32_t nsectors;
  uint32_t reserved;

  uint32_t rde;
  uint32_t rootdir;
  uint32_t dataarea;

  struct buf bufs[BUFSMAX];
};

#define clustertosector(fat, cluster) \
  (fat->dataarea + ((cluster - 2) * fat->spc))

struct fat *
fatinit(int fd);

void
fatinitbufs(struct fat *fat);

uint32_t
fatfilefind(struct fat *fat, struct fat_file *parent,
	    char *name, int *err);

void
fatfileclunk(struct fat *fat, struct fat_file *file);

int
fatfileread(struct fat *fat, struct fat_file *file,
	    uint8_t *buf, uint32_t offset, uint32_t len,
	    int *err);
int
fatfilewrite(struct fat *fat, struct fat_file *file,
	    uint8_t *buf, uint32_t offset, uint32_t len,
	    int *err);

int
fatdirread(struct fat *fat, struct fat_file *file,
	   uint8_t *buf, uint32_t offset, uint32_t len,
	   int *err);

int
fatfileremove(struct fat *fat, struct fat_file *file);

uint32_t
fatfilecreate(struct fat *fat, struct fat_file *parent,
	      char *name, uint32_t attr);

/* n can not be larger that fat->spc */
struct buf *
readsectors(struct fat *fat, uint32_t sector, size_t n);

bool
forcerereadbuf(struct fat *fat, struct buf *buf);

bool
writesectors(struct fat *fat, struct buf *buf, size_t n);

uint32_t
nextcluster(struct fat *fat, uint32_t prev);

uint32_t
findfreecluster(struct fat *fat);

uint32_t
tableinfo(struct fat *fat, uint32_t prev);

bool
writetableinfo(struct fat *fat, uint32_t cluster, uint32_t v);

struct fat_dir_entry *
copyfileentryname(struct fat_dir_entry *start, char *name);

uint32_t
fatfilefromentry(struct fat *fat, struct buf *buf,
		 struct fat_dir_entry *entry,
		 char *name, struct fat_file *parent);

uint32_t
fatfindfreefid(struct fat *fat);

bool
fatupdatedirentry(struct fat *fat, struct fat_file *file);

uint32_t
fatdirsize(struct fat *fat, struct fat_file *file);

uint32_t
fatfilesize(struct fat *fat, struct fat_file *file);
 
