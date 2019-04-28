#include "wm_internal_flash.h"
#include "wm_flash.h"
#include "wm_crypto_hard.h"
#include "wm_flash_map.h"
#include "wm_fwup.h"

#define CODE_UPD_HEADER_ADDR    0x80FC000
#define WM_UPDATE_BASE  (0x8090000)

void tls_fwup_img_update_header(T_BOOTER* img_param)
{
	unsigned char current_img;	
	psCrcContext_t	crcContext;
	T_BOOTER imgheader[2];

	tls_fls_read(CODE_RUN_HEADER_ADDR, (unsigned char *)&imgheader[0], sizeof(T_BOOTER));
	tls_fls_read(CODE_UPD_HEADER_ADDR, (unsigned char *)&imgheader[1], sizeof(T_BOOTER));

	//将两个upd_no中较大的那个值取出来，再将其加1后赋值给 CODE_UPD_HEADER_ADDR 处的header；
	if (tls_fwup_img_header_check(&imgheader[1]))
	{
		current_img = (imgheader[1].upd_no > imgheader[0].upd_no);
	}
	else
	{
		current_img = 0;
	}
	img_param->upd_no = imgheader[current_img].upd_no + 1;

	tls_crypto_crc_init(&crcContext, 0xFFFFFFFF, CRYPTO_CRC_TYPE_32, 3);
	tls_crypto_crc_update(&crcContext, (unsigned char *)img_param, sizeof(T_BOOTER)-4);
	tls_crypto_crc_final(&crcContext, &img_param->hd_checksum);
	tls_fls_write(CODE_UPD_HEADER_ADDR,  (unsigned char *)img_param,  sizeof(T_BOOTER));
}

int tls_fwup_img_write(unsigned int offset, void *buf, int size)
{
    uint32_t addr = WM_UPDATE_BASE + offset;

    return tls_fls_write(addr, buf, size);
}

int tls_fwup_flash_erase(unsigned int offset)
{
    uint32_t addr = WM_UPDATE_BASE + offset;

    return tls_fls_erase(addr/4096);
}
