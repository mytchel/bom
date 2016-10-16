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

struct fat_extBS_32 {
  uint8_t fat_size[4];
  uint8_t extended_flags[2];
  uint8_t fat_version[2];
  uint8_t root_cluster[4];
  uint8_t fat_info[2];
  uint8_t backup_BS_sector[2];
  uint8_t reserved_0[12];
  uint8_t drive_number;
  uint8_t reserved_1;
  uint8_t boot_signature;
  uint8_t volume_id[4];
  uint8_t volume_label[11];
  uint8_t fat_type_label[8];
  uint8_t boot_code[420];
  uint8_t boot_part_signature[2];
}__attribute__((packed));


struct fat_bs {
  uint8_t jmp[3];
  uint8_t oem_identifier[8];
  uint8_t sector_size[2];
  uint8_t cluster_size;
  uint8_t reserved_sectors[2];
  uint8_t nfats;
  uint8_t dir_entries[2];
  uint8_t sector_count[2];
  uint8_t mdt;
  uint8_t sectors_per_fat[2];
  uint8_t sectors_per_track[2];
  uint8_t heads[2];
  uint8_t hidden_sectors[4];
  uint8_t sector_count_lg[4];

  struct fat_extBS_32 ext;

}__attribute__((packed));


#define FAT_ATTR_read_only        0x01
#define FAT_ATTR_hidden           0x02
#define FAT_ATTR_system           0x04
#define FAT_ATTR_volume_id        0x08
#define FAT_ATTR_directory        0x10
#define FAT_ATTR_archive          0x20
#define FAT_ATTR_lfn \
  (FAT_read_only|FAT_hidden|FAT_system|FAT_volume_id)


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
  uint8_t name[11];
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


struct fat {
  int fd;

  struct fat_bs bs;
};

extern struct fat fat;

int
fatinit(int fd);

