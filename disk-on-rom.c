/*
 * disk-on-rom.c -- FAT storage on ROM
 *
 * Copyright (C) 2013 Free Software Initiative of Japan
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

#include "config.h"

int (*p_msc_scsi_write) (uint32_t lba, const uint8_t *buf, size_t size);
int (*p_msc_scsi_read) (uint32_t lba, const uint8_t **sector_p);
void (*p_msc_scsi_stop) (uint8_t code);


int
msc_scsi_write (uint32_t lba, const uint8_t *buf, size_t size)
{
  if (p_msc_scsi_write)
    return (*p_msc_scsi_write) (lba, buf, size);
}

int
msc_scsi_read (uint32_t lba, const uint8_t **sector_p)
{
  if (p_msc_scsi_read)
    return (*p_msc_scsi_read) (lba, sector_p);
}

void
msc_scsi_stop (uint8_t code)
{
  if (p_msc_scsi_stop)
    (*p_msc_scsi_stop) (code);
}
