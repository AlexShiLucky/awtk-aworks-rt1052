/**
 * file:   main_loop_aworks_raw.c
 * Author: AWTK Develop Team
 * brief:  main loop for aworks
 *
 * copyright (c) 2018 - 2018 Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * license file for more details.
 *
 */

/**
 * history:
 * ================================================================
 * 2018-05-23 li xianjing <xianjimli@hotmail.com> created
 *
 */

#include "aw_ts.h"
#include "aw_sem.h"
#include "aw_task.h"
#include "aw_delay.h"
#include "aw_mem.h"
#include "aw_cache.h"
#include "aw_fb.h"
#include "base/idle.h"
#include "base/timer.h"
#include "aw_prj_params.h"
#include "lcd/lcd_mem_bgr565.h"
#include "main_loop/main_loop_simple.h"

#include <string.h>

/*----------------------------------------------------------------------------*/
/* ������������Ϣ����                                                         */
/*----------------------------------------------------------------------------*/

static struct aw_ts_state s_ts_state = {0};

#define TS_STACK_SIZE 2 * 1024
aw_local void __ts_task_entry(void *p_arg)
{
  int tsret = 0;
  struct aw_ts_state ts_state;

  while (1) {
    memset(&ts_state, 0x00, sizeof(ts_state));
    tsret = aw_ts_exec(p_arg, &ts_state, 1);

    if (tsret >= 0) {
      s_ts_state = ts_state;

      if (ts_state.pressed) {
        main_loop_post_pointer_event(main_loop(), ts_state.pressed, ts_state.x,
            ts_state.y);
      } else {
        main_loop_post_pointer_event(main_loop(), ts_state.pressed, ts_state.x,
            ts_state.y);
      }
    }

    if (ts_state.pressed) {
      aw_mdelay(2);
    } else {
      aw_mdelay(10);
    }
  }
}

static void ts_task_init(aw_ts_id sys_ts) {
  /* ��������ʵ�壬����ջ�ռ��СΪ4096  */
  AW_TASK_DECL_STATIC(ts_task, TS_STACK_SIZE);

  AW_TASK_INIT(ts_task,      /* ����ʵ�� */
               "ts_task",   /* �������� */
               4,             /* �������ȼ� */
               TS_STACK_SIZE, /* �����ջ��С */
               __ts_task_entry,  /* ������ں��� */
               sys_ts);         /* ������ڲ��� */

  AW_TASK_STARTUP(ts_task); /* �������� */
}

char *aw_get_ts_name (void);

static aw_ts_id ts_app_init(void) {
  aw_ts_id    sys_ts;
  
  char       *p_ts_name = aw_get_ts_name();
  return_value_if_fail(strcmp(p_ts_name, "none") != 0, NULL);

  sys_ts = aw_ts_serv_id_get(p_ts_name, 0, 0);
  return_value_if_fail(sys_ts != NULL, NULL);

  if ((strcmp(p_ts_name, "imx105x-ts") == 0) || 
      (strcmp(p_ts_name, "bu21029muv") == 0)) {
    /* ���败������Ҫ��ȡУ׼����, �����������У׼���� */
    return_value_if_fail(aw_ts_calc_data_read(sys_ts) == AW_OK, NULL);
  }

  ts_task_init(sys_ts);
  return sys_ts;
}

ret_t platform_disaptch_input(main_loop_t* loop) {
  static aw_ts_id ts_id = NULL;
  if (ts_id == NULL) {
    ts_id = ts_app_init();
  }

  return RET_OK;
}

/*----------------------------------------------------------------------------*/
/* frame bufferˢ�²���                                                       */
/*----------------------------------------------------------------------------*/

extern uint32_t* aworks_get_online_fb(void);
extern uint32_t* aworks_get_offline_fb(void);
extern void*     aworks_get_fb(void);
extern int       aworks_get_fb_size();
static lcd_flush_t s_lcd_flush_default = NULL;

static ret_t lcd_aworks_fb_flush(lcd_t* lcd) {
  if (s_lcd_flush_default != NULL) {
    s_lcd_flush_default(lcd);
  }

  // ���� aw_cache_flush �ܼ�����Ƹ��������⣬��������ȫȥ��
  aw_cache_flush(aworks_get_online_fb(), aworks_get_fb_size()); // max 2ms wait
  return RET_OK;
}

#ifndef WITH_THREE_FB

/*----------------------------------------------------------------------------*/
/* ˫����ģʽ                                                                 */
/*----------------------------------------------------------------------------*/

static ret_t lcd_aworks_begin_frame(lcd_t* lcd, rect_t* dirty_rect) {
  if (lcd_is_swappable(lcd)) {
    lcd_mem_t* mem = (lcd_mem_t*)lcd;
    (void)mem;

#if 0 // ������һ�����ݵ�offline fb��Ϊ����, begin_frame֮��ֻ�������������
    // ��ǰ��awtk�����ʵ�ֻ���: ÿ֡begin_frameʱ�������������һ֡������κϲ�һ��
    // ����, ��ǰ֡����ʱҲ�����һ֡��������Ҳ����һ��, ����������ִ�������memcpy(������һ�����ݵ�offline fb��Ϊ����)
    // ������Ժ�awtk�޸����������, �ͱ���ִ�������memcpy��
    memcpy(mem->offline_fb, mem->online_fb, aworks_get_fb_size());
#endif

#if 0 // �����ô���, offline fb ���հ�, �������Թ۲�ÿ�λ��Ƶ������
    memset(mem->offline_fb, 0, aworks_get_fb_size());
#endif
  }

  return RET_OK;
}

static ret_t lcd_aworks_swap(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  void     *p_fb = aworks_get_fb();

  /* ����������д�ֱͬ������ */
  aw_fb_swap_buf(p_fb);
  mem->offline_fb = (uint8_t*)aw_fb_get_offline_buf(p_fb);
  mem->online_fb = (uint8_t*)aw_fb_get_online_buf(p_fb);
  return RET_OK;
}

lcd_t* platform_create_lcd(wh_t w, wh_t h) {
    lcd_t* lcd = lcd_mem_bgr565_create_double_fb(w,
                                                 h,
                                      (uint8_t*) aworks_get_online_fb(),
                                      (uint8_t*) aworks_get_offline_fb());

  if (lcd != NULL) {
    // �Ľ�flush����, ÿ��flush�����cache_flush (��ת��Ļ��������flush����)
    s_lcd_flush_default = lcd->flush;
    lcd->flush = lcd_aworks_fb_flush;

    // ʹ��swap����(������Ļ�������swap����)
    lcd->begin_frame = lcd_aworks_begin_frame;
    lcd->swap = lcd_aworks_swap;
  }

  return lcd;
}

#else // WITH_THREE_FB

/*----------------------------------------------------------------------------*/
/* ������ģʽ                                                                 */
/*----------------------------------------------------------------------------*/

static ret_t lcd_aworks_begin_frame(lcd_t* lcd, rect_t* dirty_rect) {
  if (lcd_is_swappable(lcd)) {
    lcd_mem_t* mem = (lcd_mem_t*)lcd;
    (void)mem;
  }
  return RET_OK;
}

static ret_t lcd_aworks_swap(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
  void     *p_fb = aworks_get_fb();

  if (AW_OK == aw_fb_try_swap_buf(p_fb)) {
      mem->offline_fb = (uint8_t*)aw_fb_get_offline_buf(p_fb);
  }
  return RET_OK;
}

lcd_t* platform_create_lcd(wh_t w, wh_t h) {
    lcd_t* lcd = lcd_mem_bgr565_create_three_fb(w,                          /* ��ֱ��� */
                                                h,                          /* �߷ֱ��� */
                                     (uint8_t *)aworks_get_online_fb(),     /* ��������ַ1 */
                                     (uint8_t *)aworks_get_offline_fb(),    /* ��������ַ2 */
                                     (uint8_t *)aworks_get_offline_fb() + \
                                                aworks_get_fb_size());      /* ��������ַ3 */

  if (lcd != NULL) {
    // �Ľ�flush����, ÿ��flush�����cache_flush (��ת��Ļ��������flush����)
    s_lcd_flush_default = lcd->flush;
    lcd->flush = lcd_aworks_fb_flush;

    // ʹ��swap����(������Ļ�������swap����)
    lcd->begin_frame = lcd_aworks_begin_frame;
    lcd->swap = lcd_aworks_swap;
  }

  return lcd;
}

#endif // WITH_THREE_FB

#include "main_loop/main_loop_raw.inc"
