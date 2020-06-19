#include "main.h"

/* SY协议需要的全局变量 */
SPT _g_spt;
/* 算法库需要的全局变量 */
unsigned char FPDATA[TPFS_Work_Size];
/* 调试信息打印 */
void debug_printf(char *time, char *file, int line, char *msg, int32_t value, uint8_t decode)
{
    #ifdef DEBUG_PRINTF
        printf("> [%s]", time), printf("(%s)", file), printf("<%d>", line);
        switch (decode)
        {
            case 0:
                printf(" %s\r\n", msg);
                break;
            case 10:
                printf(" %s: %ld\r\n", msg, value);
                break;
            case 16:
                printf(" %s: 0x%08X\r\n", msg, value);
                break;
            default:
                break;
        }
    #endif
}
/* 字符串比对函数 */
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length--){
        if(*src++ != *dst++){
            return ERROR;
        }
    }
    return SUCCESS;
}
#ifdef USB_DOWNLOAD
    /* 生产测试函数 */
    int GDSWUSBTestMain(uint8_t* fpWorkArea, uint8_t mode);
#endif
/*!
    \brief      main routine
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    uint8_t ret = 0;
    uint16_t CheckSums;
    uint16_t i;

#ifdef USB_DOWNLOAD
    RCU_APB2RST =0xffffffff;
    RCU_APB1RST =0xffffffff;
    RCU_APB2RST =0x00000000;
    RCU_APB1RST =0x00000000;
    nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0x4000);
    __enable_irq();
#endif
    /* set the priority group */
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    /* initilize systick */
    systick_config();
    /* 初始化系统参数表 */
    FPSReadSpt(FMC_WRITE_START_ADDR, (uint32_t*)&_g_spt);
    if(_g_spt.Part3.ParaTableFlag != 0x1234)
        FPSInitSpt(FMC_WRITE_START_ADDR, TFPS_REC_MAX, (uint8_t*)"GDemo", (uint8_t*)"SW9351", (uint8_t*)"v1.0", (uint8_t*)"tyw");
    FPSReadSpt(FMC_WRITE_START_ADDR, (uint32_t*)&_g_spt);
    /* 修改预备数组设备地址 */
    Modify_DeviceAddress(_g_spt.Part2.DeviceAddress);
    /* 初始化串口 */
    nvic_irq_enable(EVAL_COM0_IRQn, 0, 1);
    com_init(EVAL_COM0, 9600 * _g_spt.Part2.CFG_BoundRate, USART_STB_2BIT);
    /* 设置算法库日志等级 */
    g_log_level =  3;
    /* 初始化算法库 */
    ret = TFPSLib_Init(FPDATA, TPFS_Work_Size);
    debug_printf(__TIME__, __FILE__, __LINE__, "TFPSLib_Init RET", ret, 16);
    /* 初始化传感器 */
    ret = TFPSLib_CapInit(FPDATA, 0);
    debug_printf(__TIME__, __FILE__, __LINE__, "TFPSLib_CapInit RET", ret, 16);
    /* 检查授权码 */
    ret = TFPSLib_CheckLic(FPDATA);
    debug_printf(__TIME__, __FILE__, __LINE__, "TFPSLib_CheckLic RET", ret, 16);
#ifdef USB_DOWNLOAD
    /* 生产测试 */
    GDSWUSBTestMain(FPDATA, 0);
#endif
    /* 发送上电应答包 */
    if (ret == 0x00)
        sy_boot_rep(0x55);
    else
        sy_boot_rep(ret);
    /* 初始化LED灯对应的IO口 */
    led_init(GREEN), led_init(BLUE), led_init(RED);
    /* 初始化定时器2 */
    nvic_irq_enable(TIMER2_IRQn, 0, 2);
    timer2_config();
    /* 打开各中断 */
    timer_interrupt_enable(TIMER2,TIMER_INT_UP);
    usart_interrupt_enable(EVAL_COM0, USART_INT_RBNE);
    while(1)
    {
        if(USART_RX_FLG == 1)
        {
            usart_interrupt_disable(EVAL_COM0, USART_INT_RBNE);
            /* 校验设备地址 */
            if((USART_RX_BUF[2] == ((_g_spt.Part2.DeviceAddress >> 24) & 0xFF))&&
                    (USART_RX_BUF[3] == ((_g_spt.Part2.DeviceAddress >> 16) & 0xFF))&&
                        (USART_RX_BUF[4] == ((_g_spt.Part2.DeviceAddress >> 8) & 0xFF))&&
                            (USART_RX_BUF[5] == (_g_spt.Part2.DeviceAddress & 0xFF)))
            {
                /* 计算校验和 */
                CheckSums = 0;
                for (i = 6; i < (8 + USART_LEN - 1); i++)
                {
                    CheckSums = (CheckSums + USART_RX_BUF[i]) & 0xFFFF;
                }
                if (CheckSums != ((USART_RX_BUF[8 + USART_LEN - 1] << 8) | USART_RX_BUF[8 + USART_LEN]))
                {
                    /* 校验和错误 */
                    PS_Check_Error(USART_RX_BUF);
                }
                else
                {
                    if (USART_RX_BUF[6] == 0x01)
                    {
                        /* CMD Packet */
                        switch(USART_RX_BUF[9])
                        {
                            case 0x01:
                                /* 验证用获取图像 */
                                PS_GetImage(USART_RX_BUF);
                                break;
                            case 0x02:
                                /* 根据原始图像生成指纹特征存于模板缓冲区 */
                                PS_GenChar(USART_RX_BUF);
                                break;
                            case 0x03:
                                /* 精确比对模板缓冲区中的特征文件 */
                                break;
                            case 0x04:
                                /* 以模板缓冲区中的特征文件搜索整个或部分指纹库 */
                                PS_Search(USART_RX_BUF);
                                break;
                            case 0x05:
                                /* 将特征文件合并生成模板存于模板缓冲区 */
                                PS_RegModel(USART_RX_BUF);
                                break;
                            case 0x06:
                                /* 将模板缓冲区中的文件储存到 flash 指纹库中 */
                                PS_StoreChar(USART_RX_BUF);
                                break;
                            case 0x07:
                                /* 从 flash 指纹库中读取一个模板到模板缓冲区 */
                                break;
                            case 0x08:
                                /* 将模板缓冲区中的文件上传给上位机 */
                                break;
                            case 0x09:
                                /* 从上位机下载一个特征文件到模板缓冲区 */
                                break;
                            case 0x0A:
                                /* 上传原始图像 */
                                PS_UpImage(USART_RX_BUF);
                                break;
                            case 0x0B:
                                /* 下载原始图像 */
                                break;
                            case 0x0C:
                                /* 删除 flash 指纹库中的一个特征文件 */
                                PS_DeletChar(USART_RX_BUF);
                                break;
                            case 0x0D:
                                /* 清空 flash 指纹库 */
                                PS_Empty(USART_RX_BUF);
                                break;
                            case 0x0E:
                                /* 写 SOC 系统寄存器 */
                                PS_WriteReg(USART_RX_BUF);
                                break;
                            case 0x0F:
                                /* 读系统基本参数 */
                                PS_ReadSysPara(USART_RX_BUF);
                                break;
                            case 0x12:
                                /* 设置设备握手口令 */
                                PS_SetPwd(USART_RX_BUF);
                                break;
                            case 0x13:
                                /* 验证设备握手口令 */
                                PS_VfyPwd(USART_RX_BUF);
                                break;
                            case 0x14:
                                /* 采样随机数 */
                                break;
                            case 0x15:
                                /* 设置芯片地址 */
                                PS_SetChipAddr(USART_RX_BUF);
                                break;
                            case 0x16:
                                /* 读取 FLASH Information Page 内容 */
                                PS_ReadINFpage(USART_RX_BUF);
                                break;
                            case 0x17:
                                /* 通讯端口（UART/USB）开关控制 */
                                break;
                            case 0x18:
                                /* 写记事本 */
                                break;
                            case 0x19:
                                /* 读记事本 */
                                break;
                            case 0x1A:
                                /* 烧写片外 FLASH */
                                break;
                            case 0x1B:
                                /* 高速搜索 FLASH */
                                break;
                            case 0x1C:
                                /* 生成二值化指纹图像 */
                                break;
                            case 0x1D:
                                /* 读有效模板个数 */
                                PS_ValidTempleteNum(USART_RX_BUF);
                                break;
                            case 0x1E:
                                /* 用户 GPIO 控制命令 */
                                break;
                            case 0x1F:
                                /* 读索引表 */
                                PS_ReadIndexTable(USART_RX_BUF);
                                break;
                            case 0x29:
                                /* 注册用获取图像 */
                                PS_GetEnrollImage(USART_RX_BUF);
                                break;
                            case 0x30:
                                /* 取消指令 */
                                break;
                            case 0x31:
                                /* 自动注册模块指令 */
                                break;
                            case 0x32:
                                /* 自动验证指纹指令 */
                                break;
                            case 0x33:
                                /* 休眠指令 */
                                break;
                            case 0x34:
                                /* 获取芯片唯一序列号 */
                                PS_GetChipSN(USART_RX_BUF);
                                break;
                            case 0x35:
                                /* 握手指令 */
                                PS_HandShake(USART_RX_BUF);
                                break;
                            case 0x36:
                                /* 校验传感器 */
                                PS_CheckSensor(USART_RX_BUF);
                                break;
                            default:
                                break;
                        }
                    }
                    else if (USART_RX_BUF[6] == 0x02)
                    {
                        /* 数据包, 且有后续包 */
                        PS_Check_Error(USART_RX_BUF);
                    }
                    else if (USART_RX_BUF[6] == 0x08)
                    {
                        /* 最后一个数据包, 即结束包 */
                        PS_Check_Error(USART_RX_BUF);
                    }
                    else
                    {
                        /* 未知数据包格式 */
                        PS_Check_Error(USART_RX_BUF);
                    }
                }
            }
            else
            {
                /* 设备地址错误 */
                PS_DeviceAddress_Error(USART_RX_BUF);
            }
            /* 清空标记 */
            USART_RX_FLG = 0; 
            usart_interrupt_enable(EVAL_COM0, USART_INT_RBNE);
        }
    }
}
