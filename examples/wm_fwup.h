#ifndef _WM_FWUP_H
#define _WM_FWUP_H

typedef struct __T_BOOTER
{
	unsigned int   	magic_no;
	unsigned short 	img_type;			
	unsigned short 	zip_type;				/** image type zip flag, 0: non-zip, 1:zip*/
	unsigned int   	run_img_addr;         	/** run area image start address */
	unsigned int   	run_img_len;			/** run area image length */
	unsigned int	run_org_checksum; 		/** run area image checksum */
	unsigned int    upd_img_addr;			/** upgrade area image start address*/
	unsigned int    upd_img_len;			/** upgrade area image length*/
	unsigned int 	upd_checksum;			/** upgrade area image checksum */
	unsigned int   	upd_no;
	unsigned char  	ver[16];
	unsigned int 	hd_checksum;
} T_BOOTER;

void tls_fwup_img_update_header(T_BOOTER* img_param);
int tls_fwup_img_write(unsigned int offset, void *buf, int size);
int tls_fwup_flash_erase(unsigned int offset);

#endif
