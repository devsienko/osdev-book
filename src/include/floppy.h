
#ifndef _FLPYDSK_DRIVER_H
#define _FLPYDSK_DRIVER_H

#include <stdlib.h>

// install floppy driver
void flpydsk_install (int irq);

// set current working drive
void flpydsk_set_working_drive (uint8 drive);

// get current working drive
uint8 flpydsk_get_working_drive ();

// read a sector
uint8* flpydsk_read_sector (int sectorLBA);

// converts an LBA address to CHS
void flpydsk_lba_to_chs (int lba, int *head, int *track, int *sector);

void flpydsk_irq ();

#endif
