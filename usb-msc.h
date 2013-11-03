void msc_init (void);
void msc_media_insert_change (int available);
int msc_scsi_write (uint32_t lba, const uint8_t *buf, size_t size);
int msc_scsi_read (uint32_t lba, const uint8_t **sector_p);
void msc_scsi_stop (uint8_t code);
