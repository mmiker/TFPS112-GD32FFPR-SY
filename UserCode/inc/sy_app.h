#ifndef SY_APP_H
#define SY_APP_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "gd32f30x.h"
#include "systick.h"
#include "uart.h"
/* 宏定义 */
#define SY_REC_LEN    300
/* 算法库头文件 */
#include "tfpslib.h"
/* 算法库需要的全局变量 */
extern unsigned char FPDATA[TPFS_Work_Size];
/* SY协议通讯端口 */
#define sy_rep_com EVAL_COM0
/***************************************
                系统参数表
***************************************/
#pragma pack(push,1)                    // 一字节对齐，紧凑存储
typedef struct _FVFW_SPT{
    struct _PART1{
        uint16_t SSR;                   // 状态寄存器
        uint16_t SensorType;            // 传感器类型
        uint16_t DataBaseSize;          // 指静脉库大小
    } Part1;
    struct _PART2{
        uint16_t SecurLevel;            // 安全等级
        uint32_t DeviceAddress;         // 设备地址
        // 以下 CFG_ 为 系统配置表
        uint16_t CFG_PktSize;           // 数据包大小编码
        uint16_t CFG_BoundRate;         // 波特率系数
        uint16_t CFG_VID;               // VID
        uint16_t CFG_PID;               // PID
        uint16_t CFG_Reserved[4];       // 保留
        // 以上 CFG_ 为 系统配置表
        // 以下为设备描述符
        uint8_t ProductSN[8];           // 产品型号
        uint8_t SoftwareVersion[8];     // 软件版本号
        uint8_t ManuFacturer[8];        // 厂家名称
        uint8_t SensorName[8];          //传感器名称
        // 另一种表示法
        //uint16_t ProductSN[4];        // 产品型号
        //uint16_t SoftwareVersion[4];  // 软件版本号
        //uint16_t ManuFacturer[4];     // 厂家名称
        //uint16_t SensorName[4];       //传感器名称
        // 以上为设备描述符
        uint32_t PassWord;              // 密码
        uint32_t JtagLockFlag;          // Jtag 锁定标志
        uint16_t SensorInitEntry;       // 传感器初始化程序入口
        uint16_t SensorGetImageEntry;   // 录入图像程序入口
        uint16_t Reserved[27];          // 保留
    } Part2;
    struct _PART3{
        uint16_t ParaTableFlag;         // 参数表有效标志
    } Part3;
} SPT;
#pragma pack(pop)                       //恢复对齐方式
/* SY协议需要的全局变量 */
extern SPT _g_spt;
/* 系统参数表起始地址 */
#define FMC_WRITE_START_ADDR    0x08078000U
/* 初始化系统参数表 */
void FPSInitSpt(uint32_t WRITE_START_ADDR, uint16_t DataBaseSize, uint8_t* ProductSN, uint8_t* SensorName, uint8_t* SoftwareVersion, uint8_t* ManuFacturer);
/* 读系统参数表 */
void FPSReadSpt(uint32_t WRITE_START_ADDR, uint32_t* data);
/* 上电数据包 */
void sy_boot_rep(uint8_t rep);
/* 修改预备数组设备地址 */
void Modify_DeviceAddress(uint32_t DeviceAddress);
/* 校验和错误 */
void PS_Check_Error(uint8_t *libdata);
/* 设备地址错误 */
void PS_DeviceAddress_Error(uint8_t *libdata);
/* 验证用获取图像 */
void PS_GetImage(uint8_t *libdata);
/* 根据原始图像生成指纹特征存于模板缓冲区 */
void PS_GenChar(uint8_t *libdata);
/* 以模板缓冲区中的特征文件搜索整个或部分指纹库 */
void PS_Search(uint8_t *libdata);
/* 将特征文件合并生成模板存于模板缓冲区 */
void PS_RegModel(uint8_t *libdata);
/* 将模板缓冲区中的文件储存到 flash 指纹库中 */
void PS_StoreChar(uint8_t *libdata);
/* 上传原始图像 */
void PS_UpImage(uint8_t *libdata);
/* 删除 flash 指纹库中的一个特征文件 */
void PS_DeletChar(uint8_t *libdata);
/* 清空 flash 指纹库 */
void PS_Empty(uint8_t *libdata);
/* 写 SOC 系统寄存器 */
void PS_WriteReg(uint8_t *libdata);
/* 读系统基本参数 */
void PS_ReadSysPara(uint8_t *libdata);
/* 设置设备握手口令 */
void PS_SetPwd(uint8_t *libdata);
/* 验证设备握手口令 */
void PS_VfyPwd(uint8_t *libdata);
/* 设置芯片地址 */
void PS_SetChipAddr(uint8_t *libdata);
/* 读取 FLASH Information Page 内容 */
void PS_ReadINFpage(uint8_t *libdadta);
/* 读有效模板个数 */
void PS_ValidTempleteNum(uint8_t *libdata);
/* 读索引表 */
void PS_ReadIndexTable(uint8_t *libdata);
/* 注册用获取图像 */
void PS_GetEnrollImage(uint8_t *libdata);
/* 获取芯片唯一序列号 */
void PS_GetChipSN(uint8_t *libdata);
/* 握手指令 */
void PS_HandShake(uint8_t *libdata);
/* 校验传感器 */
void PS_CheckSensor(uint8_t *libdata);

#endif
