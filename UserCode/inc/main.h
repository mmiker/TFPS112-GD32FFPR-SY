#ifndef MAIN_H
#define MAIN_H

#include "gd32f30x.h"
#include "systick.h"
#include "uart.h"
#include "timer.h"
/* 应用代码头文件 */
#include "leds_app.h"
#include "sy_app.h"
/* 算法库头文件 */
#include "tfpslib.h"
/* 算法库需要的全局变量 */
extern unsigned char FPDATA[TPFS_Work_Size];
/* SY协议需要的全局变量 */
extern SPT _g_spt;
/* 算法库日志等级 */
extern int g_log_level;

#endif
