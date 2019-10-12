#include "aworks.h"
#include "aw_emwin_fb.h"
#include "aw_ts.h"
#include "aw_vdebug.h"
#include "aw_delay.h"
#include "string.h"
#include "aw_mem.h"
#include "aw_prj_params.h"

/* ��ɫ����*/
uint16_t color_group[5] = {0xFF80, 0x051D,0xE8C4,0xC618,0x2589};


/******************************************************************************/
/* ���㺯��*/
aw_err_t app_fbuf_pixel (uint8_t *screen_buf,
                         int  x,
                         int y,
                         uint16_t color,
                         aw_emwin_fb_info_t* p_fb)
{
    int                 idx;

    if ((x > p_fb->x_res) || (y > p_fb->y_res)) {
        return -AW_EINVAL;
    }

    idx = (y * p_fb->x_res + x) * 2;

    memcpy(&screen_buf[idx], &color, sizeof(color));

    return AW_OK;
}

/* ���ߺ���*/
aw_err_t app_fbuf_line (int x,
                        int y,
                        uint8_t *screen_buf,
                        uint16_t color,
                        aw_emwin_fb_info_t* p_fb)
{
    int  i;

    if ((x - 10 < 0) || (x + 10 >= p_fb->x_res) ||
        (y - 10 < 0) || (y + 10 > p_fb->y_res)) {
        return -AW_EINVAL;
    }

    //len = p_rect->x1 - p_rect->x0 + 1;
    for (i = 0; i < 20; i++) {
        app_fbuf_pixel(screen_buf, x - 10 + i, y, color, p_fb);
    }

    //len = p_rect->y1 - p_rect->y0 + 1;
    for (i = 0; i < 20; i++) {
        app_fbuf_pixel(screen_buf, x, y - 10 + i, color, p_fb);
    }

    return AW_OK;
}

/********************************************************************************/
/* У׼����*/
int ts_calibrate (uint8_t          *screen_buf,
                  aw_ts_id                 id,
                  aw_ts_lib_calibration_t *p_cal,
                  aw_emwin_fb_info_t* p_fb)
{
    int                 i;
    struct aw_ts_state  sta;

    /* ��ʼ����㴥������ */
    p_cal->log[0].x = 60 - 1;
    p_cal->log[0].y = 60 - 1;
    p_cal->log[1].x = p_fb->x_res  - 60 - 1;
    p_cal->log[1].y = 60 - 1;
    p_cal->log[2].x = p_fb->x_res - 60 - 1;
    p_cal->log[2].y = p_fb->y_res - 60 - 1;
    p_cal->log[3].x = 60 - 1;
    p_cal->log[3].y = p_fb->y_res - 60 - 1;
    p_cal->log[4].x = p_fb->x_res / 2 - 1;
    p_cal->log[4].y = p_fb->y_res / 2 - 1;

    p_cal->cal_res_x = p_fb->x_res;
    p_cal->cal_res_y = p_fb->y_res;

    /* ��㴥�� */
    for (i = 0; i < 5; i++) {
        /* ������ͼ��*/
        app_fbuf_line (p_cal->log[i].x, p_cal->log[i].y, screen_buf, 0xFFFF, p_fb);

        while (1) {
            /* �ȴ���ȡ�������λ�ô������� */
            if ((aw_ts_get_phys(id, &sta, 1) > 0) &&
                (sta.pressed == TRUE)) {
                p_cal->phy[i].x = sta.x;
                p_cal->phy[i].y = sta.y;

                aw_kprintf("\n x=%d, y=%d \r\n", (uint32_t)sta.x, (uint32_t)sta.y);
                while(1) {
                    aw_mdelay(500);
                    if ((aw_ts_get_phys(id, &sta, 1) == AW_OK) &&
                        (sta.pressed == FALSE)) {
                        break;
                    }
                }
                break;
            }
            aw_mdelay(10);
        }
        /* ����ϸ���ͼ��*/
        app_fbuf_line (p_cal->log[i].x, p_cal->log[i].y, screen_buf, 0, p_fb);
    }
    return 0;
}


void ts_init (aw_emwin_fb_info_t* p_fb)
{
    aw_ts_id                sys_ts;
    aw_ts_lib_calibration_t cal;
    uint8_t                 *p_buf = NULL;

    /* ��ȡ�����豸 */
    sys_ts = aw_ts_serv_id_get(SYS_TS_ID, 0, 0);
    if (sys_ts == NULL) {
        return;
    }


    /* �������ͷ֡�ṹ��*/
    p_buf = (uint8_t *)p_fb->v_addr;   /*emwin֡�����ַ*/
    /* ���ñ���Ϊ��ɫ*/
    memset(p_buf, 0, p_fb->x_res * p_fb->y_res *2);



    /* �ж��Ƿ��豸֧�ִ���У׼ */
    if (aw_ts_calc_flag_get(sys_ts)) {
        /* ����ϵͳ�������� */
        if (aw_ts_calc_data_read(sys_ts) != AW_OK) {
            /* û����Ч���ݣ�����У׼ */
            do {
                if (ts_calibrate(p_buf, sys_ts, &cal, p_fb) < 0) {
                    //todo
                }
            } while (aw_ts_calibrate(sys_ts, &cal) != AW_OK);
            /* У׼�ɹ������津������ */
            aw_ts_calc_data_write(sys_ts);
        }
    }else {
        /* ����������4.3��������Ҫ����XYת�� */
        aw_ts_set_orientation(sys_ts, AW_TS_SWAP_XY);
    }
}

