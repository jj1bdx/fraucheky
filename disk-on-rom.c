/*
 * disk-on-rom.c -- FAT storage (GPL, README, and INDEX.HTM) on ROM
 *
 * Copyright (C) 2013, 2015 Free Software Initiative of Japan
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of Fraucheky, GNU GPL in a USB thumb drive
 *
 * Fraucheky is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Fraucheky is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <string.h>
#include <chopstx.h>

#include "disk-on-rom.h"
#include "msc.h"
#include "mcu/sys-stm32f103.h"

extern uint8_t fraucheky_main_done;
extern int fraucheky_enabled (void);

extern uint8_t _binary_COPYING_start;
extern uint8_t _binary_COPYING_end;
extern uint8_t _binary_README_start;
extern uint8_t _binary_README_end;
extern uint8_t _binary_INDEX_start;
extern uint8_t _binary_INDEX_end;

int (*p_msc_scsi_write) (uint32_t lba, const uint8_t *buf, size_t size);
int (*p_msc_scsi_read) (uint32_t lba, const uint8_t **sector_p);
void (*p_msc_scsi_stop) (uint8_t code);

#define TOTAL_SECTORS 128	/* Maximum 64KB.  */
#define SECTOR_SIZE 512

/*
 * blk=0: master boot record sector
 * blk=1: fat0
 * blk=2: fat1
 * blk=3: root directory
 * blk=4: fat cluster #2
 * ...
 * blk=4+123: fat cluster #2+123
 */

static const uint8_t d0_0_sector[] = {
  0xeb, 0x3c,             /* Jump instruction */
  0x90,                   /* NOP instruction */
  0x6d, 0x6b, 0x64, 0x6f, 0x73, 0x66, 0x73, 0x00, /* "mkdosfs" */
  0x00, 0x02,             /* Bytes per sector: 512 */
  0x01,                   /* sectors per cluster: 1 */
  0x01, 0x00,             /* reserved sector count: 1 */
  0x02,                   /* Number of FATs: 2 */
  0x10, 0x00,             /* Max. root directory entries: 16 (1 sector) */
  TOTAL_SECTORS, 0x00,    /* total sectors: 128 */
  0xf8,                   /* media descriptor: fixed disk */
  0x01, 0x00,             /* sectors per FAT: 1 */
  0x20, 0x00,             /* sectors per track: 32 */
  0x40, 0x00,             /* number of heads: 64 */
  0x00, 0x00, 0x00, 0x00, /* hidden sectors: 0 */
  0x00, 0x00, 0x00, 0x00, /* total sectors (long) */
  0x00,                   /* drive number */
  0x00,                   /* reserved */
  0x29,                   /* extended boot signature */
  0x9e, 0x5a, 0x2b, 0x68, /* Volume ID (serial number) (Little endian) */
  /* Volume label: Fraucheky */
  'F', 'r', 'a', 'u', 'c', 'h', 'e', 'k', 'y', ' ', ' ',
  0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, /* FAT12 */
  0x0e,                   /*    push cs */
  0x1f,                   /*    pop ds */
  0xbe, 0x5b, 0x7c,       /*    mov si, offset message_txt */
  0xac,                   /* 1: lodsb */
  0x22, 0xc0,             /*    and al, al */
  0x74, 0x0b,             /*    jz 2f */
  0x56,                   /*    push si */
  0xb4, 0x0e,             /*    mov ah, 0eh */
  0xbb, 0x07, 0x00,       /*    mov bx, 0007h */
  0xcd, 0x10,             /*    int 10h ; output char color=white */
  0x5e,                   /*    pop si */
  0xeb, 0xf0,             /*    jmp 1b */
  0x32, 0xe4,             /* 2: xor ah, ah */
  0xcd, 0x16,             /*    int 16h; key input */
  0xcd, 0x19,             /*    int 19h; load OS */
  0xeb, 0xfe,             /* 3: jmp 3b */
  /* "This is not a bootable disk... \r\n" */
  0x54, 0x68, 0x69, 0x73, 0x20,
  0x69, 0x73, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x61,
  0x20, 0x62, 0x6f, 0x6f, 0x74, 0x61, 0x62, 0x6c,
  0x65, 0x20, 0x64, 0x69, 0x73, 0x6b, 0x2e, 0x20,
  0x20, 0x50, 0x6c, 0x65, 0x61, 0x73, 0x65, 0x20,
  0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x20, 0x61,
  0x20, 0x62, 0x6f, 0x6f, 0x74, 0x61, 0x62, 0x6c,
  0x65, 0x20, 0x66, 0x6c, 0x6f, 0x70, 0x70, 0x79,
  0x20, 0x61, 0x6e, 0x64, 0x0d, 0x0a, 0x70, 0x72,
  0x65, 0x73, 0x73, 0x20, 0x61, 0x6e, 0x79, 0x20,
  0x6b, 0x65, 0x79, 0x20, 0x74, 0x6f, 0x20, 0x74,
  0x72, 0x79, 0x20, 0x61, 0x67, 0x61, 0x69, 0x6e,
  0x20, 0x2e, 0x2e, 0x2e, 0x20, 0x0d, 0x0a, 0x00,
};


/* Sector no for Root:       3 */
#define DROPHERE_SECTOR      4
#define COPYING_SECTOR_START 5
#define COPYING_SECTOR_END   (COPYING_SECTOR_START+COPYING_BLOCKS-1)
#define README_SECTOR_START  (COPYING_SECTOR_END+1)
#define README_SECTOR_END    (README_SECTOR_START+README_BLOCKS-1)
#define INDEX_SECTOR_START   (README_SECTOR_END+1)
#define INDEX_SECTOR_END     (INDEX_SECTOR_START+INDEX_BLOCKS-1)

#define CLSTR_NO(sec_no) (sec_no-2)

static const uint8_t d0_fat0_sector[] = {
  0xf8, 0xff, 0xff,  /* Media descriptor: fixed disk *//* EOC */
  CLUSTER_MAP
};

static const uint8_t d0_rootdir_sector[] = {
  'F', 'r', 'a',  'u',  'c',  'h',  'e',  'k',  'y',  ' ',  ' ', 
  /* "Fraucheky  " */
  0x08, /* Volume label */
  0x00,
  0x00, 0xe7, 0x63, 0x65, 0x43, /* Create Time */
  0x65, 0x43,  /* last access */
  0x00, 0x00,
  0xe7, 0x63, 0x65, 0x43,  /* last modified */
  0x00, 0x00, /* cluster # */
  0x00, 0x00, 0x00, 0x00, /* file size */

  'C', 'O', 'P',  'Y',  'I',  'N',  'G',  ' ',  ' ',  ' ',  ' ', 
  /* "COPYING     " */
  COPYING_ATTRIBUTES,
  CLSTR_NO (COPYING_SECTOR_START), 0x00,  /* cluster # */
  COPYING_FILE_SIZE,

  'R', 'E', 'A',  'D',  'M',  'E',  ' ',  ' ',  ' ',  ' ',  ' ', 
  /* "README      " */
  README_ATTRIBUTES,
  CLSTR_NO (README_SECTOR_START), 0x00,  /* cluster # */
  README_FILE_SIZE,

  'I', 'N', 'D', 'E', 'X', ' ', ' ', ' ', 'H', 'T', 'M', /* INDEX.HTM */
  INDEX_ATTRIBUTES,
  CLSTR_NO (INDEX_SECTOR_START), 0x00, /* cluster # */
  INDEX_FILE_SIZE,

  'D', 'R', 'O', 'P', 'H', 'E', 'R', 'E', ' ', ' ', ' ', /* DROPHERE */
  0x10, /* Sub directory */
  0x00,
  0x64, 0x10, 0x64, 0x65, 0x43,  /* Create Time */
  0x63, 0x43, /* last access */
  0x00, 0x00,
  0xe4, 0x74, 0x16, 0x43, /* last modified */
  CLSTR_NO (DROPHERE_SECTOR), 0x00,  /* cluster # */
  0x00, 0x00, 0x00, 0x00  /* file size */
};

static const uint8_t d0_drophere_sector[] = {
  '.', ' ', ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', 
  /* ".          " */
  0x10, /* directory */
  0x00,
  0x00, 0xe7, 0x63, 0x65, 0x43, /* Create Time */
  0x65, 0x43,  /* last access */
  0x00, 0x00,
  0xe7, 0x63, 0x65, 0x43,  /* last modified */
  CLSTR_NO (DROPHERE_SECTOR), 0x00, /* cluster # */
  0x00, 0x00, 0x00, 0x00, /* file size */

  '.', '.', ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', 
  /* "..         " */
  0x10, /* directory */
  0x00,
  0x00, 0xe7, 0x63, 0x65, 0x43, /* Create Time */
  0x65, 0x43,  /* last access */
  0x00, 0x00,
  0xe7, 0x63, 0x65, 0x43,  /* last modified */
  0x00, 0x00, /* cluster # */
  0x00, 0x00, 0x00, 0x00, /* file size */
};

static uint8_t the_sector[512];

const uint16_t rom_var = { 0xffff };

int
msc_scsi_write (uint32_t lba, const uint8_t *buf, size_t size)
{
  if (p_msc_scsi_write)
    return (*p_msc_scsi_write) (lba, buf, size);

  if (fraucheky_enabled () && lba == DROPHERE_SECTOR)
    {
      flash_unlock ();
      flash_program_halfword ((uint32_t)&rom_var, 0);
    }

  return 0;
}

int
msc_scsi_read (uint32_t lba, const uint8_t **sector_p)
{
  uint32_t offset;
  uint32_t size;

  if (p_msc_scsi_read)
    return (*p_msc_scsi_read) (lba, sector_p);

  if (lba >= TOTAL_SECTORS)
    return SCSI_ERROR_ILLEAGAL_REQUEST;

  *sector_p = the_sector;

  switch (lba)
    {
    case 0:			/* MBR */
      memcpy (the_sector, d0_0_sector, sizeof d0_0_sector);
      memset (the_sector + sizeof d0_0_sector, 0, 512 - sizeof d0_0_sector);
      the_sector[510] = 0x55;
      the_sector[511] = 0xaa;
      return 0;

    case 1:
    case 2:			/* FAT */
      memcpy (the_sector, d0_fat0_sector, sizeof d0_fat0_sector);
      memset (the_sector + sizeof d0_fat0_sector, 0,
	      512 - sizeof d0_fat0_sector);
      return 0;

    case 3:			/* Root directory.  */
      memcpy (the_sector, d0_rootdir_sector, sizeof d0_rootdir_sector);
      memset (the_sector + sizeof d0_rootdir_sector, 0,
	      512 - sizeof d0_rootdir_sector);
      return 0;

    case 4:			/* DROPHERE directory.  */
      memcpy (the_sector, d0_drophere_sector, sizeof d0_drophere_sector);
      memset (the_sector + sizeof d0_drophere_sector, 0,
	      512 - sizeof d0_drophere_sector);
      return 0;

    default:
      if (lba >= COPYING_SECTOR_START && lba <= COPYING_SECTOR_END)
	{
	  offset = (lba - COPYING_SECTOR_START) * SECTOR_SIZE;
	  size = SECTOR_SIZE;

	  if (lba == COPYING_SECTOR_END)
	    {
	      size = &_binary_COPYING_end - &_binary_COPYING_start
		- (COPYING_BLOCKS - 1) * SECTOR_SIZE;
	      if (size != SECTOR_SIZE)
		memset (the_sector + size, 0, SECTOR_SIZE - size);
	    }

	  memcpy (the_sector, &_binary_COPYING_start + offset, size);
	}
      else if (lba >= README_SECTOR_START && lba <= README_SECTOR_END)
	{
	  offset = (lba - README_SECTOR_START) * SECTOR_SIZE;
	  size = SECTOR_SIZE;

	  if (lba == README_SECTOR_END)
	    {
	      size = &_binary_README_end - &_binary_README_start
		- (README_BLOCKS - 1) * SECTOR_SIZE;
	      if (size != SECTOR_SIZE)
		memset (the_sector + size, 0, SECTOR_SIZE - size);
	    }

	  memcpy (the_sector, &_binary_README_start + offset, size);
	}
      else if (lba >= INDEX_SECTOR_START && lba <= INDEX_SECTOR_END)
	{
	  offset = (lba - INDEX_SECTOR_START) * SECTOR_SIZE;
	  size = SECTOR_SIZE;

	  if (lba == INDEX_SECTOR_END)
	    {
	      size = &_binary_INDEX_end - &_binary_INDEX_start
		- (INDEX_BLOCKS - 1) * SECTOR_SIZE;
	      if (size != SECTOR_SIZE)
		memset (the_sector + size, 0, SECTOR_SIZE - size);
	    }

	  memcpy (the_sector, &_binary_INDEX_start + offset, size);
	}
      else
	memset (the_sector, 0, 512);
      return 0;
    }
}

void
msc_scsi_stop (uint8_t code)
{
  if (p_msc_scsi_stop)
    (*p_msc_scsi_stop) (code);
  else
    fraucheky_main_done = 1;
}
