#include "sy_app.h"

static uint8_t SY_ACK_BUF[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x07};
static uint8_t SY_DATA_BUF[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t SY_TX_BUF[SY_REC_LEN];
/* FMC擦除函数 */
static void fmc_erase_pages(uint32_t PageNum, uint32_t WRITE_START_ADDR, uint16_t PAGE_SIZE)
{
    uint32_t EraseCounter;

    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
    fmc_flag_clear(FMC_FLAG_BANK0_END);
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    
    /* erase the flash pages */
    for(EraseCounter = 0; EraseCounter < PageNum; EraseCounter++){
        fmc_page_erase(WRITE_START_ADDR + (PAGE_SIZE * EraseCounter));
        fmc_flag_clear(FMC_FLAG_BANK0_END);
        fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
        fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    }

    /* lock the main FMC after the erase operation */
    fmc_lock();
}
/* FMC编程函数 */
static void fmc_program(uint32_t WRITE_START_ADDR, uint32_t WRITE_END_ADDR, uint32_t* data)
{
    uint32_t address = 0x00000000U;

    /* unlock the flash program/erase controller */
    fmc_unlock();

    address = WRITE_START_ADDR;

    /* program flash */
    while(address < WRITE_END_ADDR){
        fmc_word_program(address, *data);
        address += 4;
        data += 1;
        fmc_flag_clear(FMC_FLAG_BANK0_END);
        fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
        fmc_flag_clear(FMC_FLAG_BANK0_PGERR);
    }

    /* lock the main FMC after the program operation */
    fmc_lock();
}
/* 校验FMC擦除结果 */
static int fmc_erase_pages_check(uint32_t WRITE_START_ADDR, uint32_t WordNum)
{
    uint32_t i;
    uint32_t *ptrd;

    ptrd = (uint32_t *)WRITE_START_ADDR;

    /* check flash whether has been erased */
    for(i = 0; i < WordNum; i++){
        if(0xFFFFFFFF != (*ptrd)){
            return 1;
        }else{
            ptrd++;
        }
    }
    return 0;
}
/* 校验FMC编程结果 */
static int fmc_program_check(uint32_t WRITE_START_ADDR, uint32_t WordNum, uint32_t* data)
{
    uint32_t i;
    uint32_t *ptrd;

    ptrd = (uint32_t *)WRITE_START_ADDR;

    /* check flash whether has been programmed */
    for(i = 0; i < WordNum; i++){
        if((*ptrd) != *data){
            return 1;
        }else{
            ptrd++;
            data++;
        }
    }
    return 0;
}
/* 初始化系统参数表 */
void FPSInitSpt(uint32_t WRITE_START_ADDR, uint16_t DataBaseSize, uint8_t* ProductSN, uint8_t* SensorName, uint8_t* SoftwareVersion, uint8_t* ManuFacturer)
{
    uint16_t i = 0;
    uint16_t PAGE_SIZE;
    uint32_t WRITE_END_ADDR;
    uint32_t PageNum;
    uint32_t WordNum;

    SPT g_spt;

    g_spt.Part1.DataBaseSize = DataBaseSize;                            // 指纹库大小
    g_spt.Part1.SensorType = 0x0000;                                    // 传感器类型
    g_spt.Part1.SSR = 0x0000;                                           // 状态寄存器
    g_spt.Part2.SecurLevel = 0x0003;                                    // 安全等级
    g_spt.Part2.DeviceAddress = 0xFFFFFFFF;                             // 设备地址
    g_spt.Part2.CFG_PktSize = 1;                                        // 数据包大小编码
    g_spt.Part2.CFG_BoundRate = 6;                                      // 波特率系数
    g_spt.Part2.CFG_PID = 0x0000;                                       // pid
    g_spt.Part2.CFG_VID = 0x0000;                                       // vid
    /*             保留             */
    g_spt.Part2.CFG_Reserved[0] = 0;
    g_spt.Part2.CFG_Reserved[1] = 0;
    g_spt.Part2.CFG_Reserved[2] = 0;
    g_spt.Part2.CFG_Reserved[3] = 0;
    /*                              */
    strcpy((char*)g_spt.Part2.ProductSN, (char*)ProductSN);             // 产品型号
    strcpy((char*)g_spt.Part2.SensorName, (char*)SensorName);           // 传感器名称
    strcpy((char*)g_spt.Part2.SoftwareVersion, (char*)SoftwareVersion); // 软件版本号
    strcpy((char*)g_spt.Part2.ManuFacturer, (char*)ManuFacturer);       // 厂家名称
    g_spt.Part2.PassWord = 0x00000000;                                  // 密码
    g_spt.Part2.JtagLockFlag = 0x00000000;                              // Jtag 标志
    g_spt.Part2.SensorInitEntry = 0x0000;                               // 传感器初始化程序入口
    g_spt.Part2.SensorGetImageEntry = 0x0000;                           // 录入图像程序入口
    /*             保留             */
    for(i = 0; i < 27; i++)
    {
        g_spt.Part2.Reserved[i] = 0x0000;
    }
    g_spt.Part3.ParaTableFlag = 0x1234;                                 // 参数表有效标志
    /*                              */
    PAGE_SIZE = (uint16_t)0x800U;
    WRITE_END_ADDR = WRITE_START_ADDR + sizeof(SPT);
    PageNum = (uint32_t)ceil((double)(WRITE_END_ADDR - WRITE_START_ADDR) / PAGE_SIZE);
    WordNum = ((WRITE_END_ADDR - WRITE_START_ADDR) >> 2);

    fmc_erase_pages(PageNum, WRITE_START_ADDR, PAGE_SIZE);
    fmc_erase_pages_check(WRITE_START_ADDR, WordNum);

    fmc_program(WRITE_START_ADDR, WRITE_END_ADDR, (uint32_t*)&g_spt);
    fmc_program_check(WRITE_START_ADDR, WordNum, (uint32_t*)&g_spt);
}
/* 读系统参数表 */
void FPSReadSpt(uint32_t WRITE_START_ADDR, uint32_t* data)
{
    uint32_t i;
    uint32_t *ptrd;
    uint32_t WRITE_END_ADDR;
    uint32_t WordNum;

    ptrd = (uint32_t *)WRITE_START_ADDR;
    WRITE_END_ADDR = WRITE_START_ADDR + sizeof(SPT);
    WordNum = ((WRITE_END_ADDR - WRITE_START_ADDR) >> 2);

    for(i = 0; i < WordNum; i++){
        *data = *ptrd;
        ptrd++;
        data++;
    }
}
/* 写系统参数表 */
static int FPSSetSpt(uint32_t WRITE_START_ADDR, uint32_t* data)
{
    uint16_t PAGE_SIZE;
    uint32_t WRITE_END_ADDR;
    uint32_t PageNum;
    uint32_t WordNum;

    PAGE_SIZE = (uint16_t)0x800U;
    WRITE_END_ADDR = WRITE_START_ADDR + sizeof(SPT);
    PageNum = (uint32_t)ceil((double)(WRITE_END_ADDR - WRITE_START_ADDR) / PAGE_SIZE);
    WordNum = ((WRITE_END_ADDR - WRITE_START_ADDR) >> 2);

    fmc_erase_pages(PageNum, WRITE_START_ADDR, PAGE_SIZE);
    if (fmc_erase_pages_check(WRITE_START_ADDR, WordNum))
        return 1;
    fmc_program(WRITE_START_ADDR, WRITE_END_ADDR, data);
    if (fmc_program_check(WRITE_START_ADDR, WordNum, data))
        return 1;

    return 0;
}
/* 上电数据包 */
void sy_boot_rep(uint8_t rep)
{
    usart_data_transmit(sy_rep_com, rep);
    while(RESET == usart_flag_get(sy_rep_com, USART_FLAG_TBE));
}
/* 发送应答包 */
static void sy_rep(void)
{
    int i;
    uint16_t len;

    len = (SY_TX_BUF[7] << 8) | SY_TX_BUF[8];
    len += 1;
    for (i = 0; i < (8 + len); i++)
    {
        usart_data_transmit(sy_rep_com, SY_TX_BUF[i]);
        while(RESET == usart_flag_get(sy_rep_com, USART_FLAG_TBE));
    }
}
/* 计算校验和 */
static uint16_t calc_check(void)
{
    uint32_t i;
    uint16_t cksums;
    uint16_t len;

    cksums = 0;
    len = (SY_TX_BUF[7] << 8) | SY_TX_BUF[8];
    len += 1;
    for (i = 6; i < (8 + len - 2); i++)
    {
        cksums = (cksums + SY_TX_BUF[i]) & 0xFFFF;
    }
    return cksums;
}
/* 修改预备数组设备地址 */
void Modify_DeviceAddress(uint32_t DeviceAddress)
{
    SY_ACK_BUF[2] = (DeviceAddress >> 24) & 0xFF;
    SY_ACK_BUF[3] = (DeviceAddress >> 16) & 0xFF;
    SY_ACK_BUF[4] = (DeviceAddress >> 8) & 0xFF;
    SY_ACK_BUF[5] = DeviceAddress & 0xFF;
    SY_DATA_BUF[2] = (DeviceAddress >> 24) & 0xFF;
    SY_DATA_BUF[3] = (DeviceAddress >> 16) & 0xFF;
    SY_DATA_BUF[4] = (DeviceAddress >> 8) & 0xFF;
    SY_DATA_BUF[5] = DeviceAddress & 0xFF;
}
/* 校验和错误 */
void PS_Check_Error(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    SY_TX_BUF[9] = 0x01;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 设备地址错误 */
void PS_DeviceAddress_Error(uint8_t *libdata)
{
        uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    SY_TX_BUF[9] = 0x20;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 验证用获取图像 */
void PS_GetImage(uint8_t *libdata)
{
    int ret = 0;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    ret = TFPSLib_Capture(FPDATA);
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            break;
        case TFPRet_NoFinger:
            SY_TX_BUF[9] = 0x02;
            break;
        case TFPRet_SensorErr:
            SY_TX_BUF[9] = 0x03;
            break; 
        default:
            SY_TX_BUF[9] = 0x01;
            break;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 根据原始图像生成指纹特征存于模板缓冲区 */
void PS_GenChar(uint8_t *libdata)
{
    int ret = 0;
    uint8_t bufNum;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    bufNum = libdata[10];
    if ((bufNum > TFPS_FEATCNT_PER_REC) || (bufNum < 1))
        ret = TFPRet_InvalidParam;
    else
        ret = TFPSLib_MkFeat(FPDATA, bufNum);
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            break;
        case TFPRet_InvalidParam:
            SY_TX_BUF[9] = 0x01;
            break;
        case TFPRet_BadImg:
            SY_TX_BUF[9] = 0x06;
            break;
        case TFPRet_MkFeatErr:
            SY_TX_BUF[9] = 0x07;
            break; 
        default:
            SY_TX_BUF[9] = 0x15;
            break;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 以模板缓冲区中的特征文件搜索整个或部分指纹库 */
void PS_Search(uint8_t *libdata)
{
    int ret = 0;
    uint8_t probeBuf;
    uint16_t StartPage;
    uint16_t PageNum;
    int nUIDArr;
    int nScoreArr;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x07;
    /* 数据 */
    probeBuf = libdata[10];
    StartPage = (libdata[11] << 8) | libdata[12];
    PageNum = (libdata[13] << 8) | libdata[14];
    if ((probeBuf > 0) && (probeBuf <= TFPS_FEATCNT_PER_REC) && ((StartPage + PageNum - 1) < TFPS_REC_MAX))
        ret = TFPSLib_Search(FPDATA, probeBuf, &nUIDArr, &nScoreArr);
    else
        ret = TFPRet_InvalidParam;

    if ((ret == TFPRet_OK) && (nUIDArr >= 0))
    {
        if ((nUIDArr < StartPage) || (nUIDArr > (StartPage + PageNum - 1)))
            ret = TFPRet_NotMatched;
    }
    else if (nUIDArr < 0)
        ret = TFPRet_NotMatched;
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            SY_TX_BUF[10] = (nUIDArr >> 8) & 0xFF;
            SY_TX_BUF[11] = nUIDArr & 0xFF;
            SY_TX_BUF[12] = (nScoreArr >> 8) & 0xFF;
            SY_TX_BUF[13] = nScoreArr & 0xFF;
            break;
        case TFPRet_NotMatched:
            SY_TX_BUF[9] = 0x09;
            break;
        default:
            SY_TX_BUF[9] = 0x01;
            break;
    }
    if (nScoreArr >= 344)
        SY_TX_BUF[9] = 0x17;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[14] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[15] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 将特征文件合并生成模板存于模板缓冲区 */
void PS_RegModel(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    SY_TX_BUF[9] = 0x00;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 将模板缓冲区中的文件储存到 flash 指纹库中 */
void PS_StoreChar(uint8_t *libdata)
{
    int ret = 0;
    uint16_t nRegID;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    nRegID = (libdata[11] << 8) | libdata[12];
    if (nRegID < TFPS_REC_MAX)
        ret = TFPSLib_RegTemplate(FPDATA, nRegID);
    else
        ret = TFPRet_InvalidParam;
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            break;
        case TFPRet_InvalidParam:
            SY_TX_BUF[9] = 0x0B;
            break;
        case TFPRet_DBAccessErr:
            SY_TX_BUF[9] = 0x18;
            break;
        default:
            SY_TX_BUF[9] = 0x01;
            break;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 上传原始图像 */
void PS_UpImage(uint8_t *libdata)
{
    int i,j;
    int PackLen;
    int PackNum;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    SY_TX_BUF[9] = 0x00;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
    /* 计算数据包长度和数量 */
    PackLen = (_g_spt.Part2.CFG_PktSize + 1) * 32;
    PackNum = (112 * 112) / PackLen;
    /* 制作数据包 */
    for(i = 0; i < PackNum; i++)
    {
        /* 初始化数组 */
        memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
        memcpy(SY_TX_BUF, SY_DATA_BUF, sizeof(SY_DATA_BUF));
        /* 命令字 */
        if (i == (PackNum - 1))
            SY_TX_BUF[6] = 0x08;
        else
            SY_TX_BUF[6] = 0x02;
        /* 数据长度 */
        SY_TX_BUF[7] = ((PackLen + 2) >> 8) & 0xFF;
        SY_TX_BUF[8] = (PackLen + 2) & 0xFF;
        /* 数据 */
        for (j = 0; j < PackLen; j++)
        {
            SY_TX_BUF[j + 9] = FPDATA[j + (i * PackLen)] & 0xFF;
        }
        /* 校验和 */
        cksums = calc_check();
        SY_TX_BUF[9 + PackLen] = (cksums >> 8) & 0xFF;
        SY_TX_BUF[10 + PackLen] = cksums & 0xFF;
        /* 发送数据包 */
        sy_rep();
    }

}
/* 删除 flash 指纹库中的一个特征文件 */
void PS_DeletChar(uint8_t *libdata)
{
    int ret = 0;
    int i = 0;
    uint16_t PageID;
    uint16_t PageNum;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    PageID = libdata[10] << 8 | libdata[11];
    PageNum = libdata[12] << 8 | libdata[13];
    if ((PageID + PageNum) > TFPS_REC_MAX)
        ret = TFPRet_InvalidParam;
    else
    {
        for (i = 0; i < PageNum; i++)
        {
            ret = TFPSLib_RemoveTemplate(FPDATA, PageID + i);
            if (ret != TFPRet_OK)
                break;
        }
    }
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            break;
        default:
            SY_TX_BUF[9] = 0x10;
            break;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 清空 flash 指纹库 */
void PS_Empty(uint8_t *libdata)
{
    int ret = 0;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    ret = TFPSLib_Empty(FPDATA);
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            break;
        default:
            SY_TX_BUF[9] = 0x11;
            break;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 写 SOC 系统寄存器 */
void PS_WriteReg(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    switch(libdata[10])
    {
        case 0x04:
            _g_spt.Part2.CFG_BoundRate = libdata[11];
            break;
        case 0x05:
            _g_spt.Part2.SecurLevel = libdata[11];
            break;
        case 0x06:
            _g_spt.Part2.CFG_PktSize = libdata[11];
            break;
        default:
            SY_TX_BUF[9] = 0x1A;
            break;
    }
    if (FPSSetSpt(FMC_WRITE_START_ADDR, (uint32_t*)&_g_spt))
        SY_TX_BUF[9] = 0x01;
    else
        SY_TX_BUF[9] = 0x00;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 读系统基本参数 */
void PS_ReadSysPara(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x13;
    /* 数据 */
    SY_TX_BUF[9] = 0x00;
    SY_TX_BUF[10] = (_g_spt.Part1.SSR >> 8) & 0xFF;
    SY_TX_BUF[11] = _g_spt.Part1.SSR & 0xFF;
    SY_TX_BUF[12] = (_g_spt.Part1.SensorType >> 8) & 0xFF;
    SY_TX_BUF[13] = _g_spt.Part1.SensorType & 0xFF;
    SY_TX_BUF[14] = (_g_spt.Part1.DataBaseSize >> 8) & 0xFF;
    SY_TX_BUF[15] = _g_spt.Part1.DataBaseSize & 0xFF;
    SY_TX_BUF[16] = (_g_spt.Part2.SecurLevel >> 8) & 0xFF;
    SY_TX_BUF[17] = _g_spt.Part2.SecurLevel & 0xFF;
    SY_TX_BUF[18] = (_g_spt.Part2.DeviceAddress >> 24) & 0xFF;
    SY_TX_BUF[19] = (_g_spt.Part2.DeviceAddress >> 16) & 0xFF;
    SY_TX_BUF[20] = (_g_spt.Part2.DeviceAddress >> 8) & 0xFF;
    SY_TX_BUF[21] = _g_spt.Part2.DeviceAddress & 0xFF;
    SY_TX_BUF[22] = (_g_spt.Part2.CFG_PktSize >> 8) & 0xFF;
    SY_TX_BUF[23] = _g_spt.Part2.CFG_PktSize & 0xFF;
    SY_TX_BUF[24] = (_g_spt.Part2.CFG_BoundRate >> 8) & 0xFF;
    SY_TX_BUF[25] = _g_spt.Part2.CFG_BoundRate & 0xFF;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[26] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[27] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 设置设备握手口令 */
void PS_SetPwd(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    _g_spt.Part2.PassWord = (libdata[10] << 24) | (libdata[11] << 16) | (libdata[12] << 8) | (libdata[13]);
    if (FPSSetSpt(FMC_WRITE_START_ADDR, (uint32_t*)&_g_spt))
        SY_TX_BUF[9] = 0x01;
    else
        SY_TX_BUF[9] = 0x00;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 验证设备握手口令 */
void PS_VfyPwd(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    if(_g_spt.Part2.PassWord == ((libdata[10] << 24) | (libdata[11] << 16) | (libdata[12] << 8) | (libdata[13])))
        SY_TX_BUF[9] = 0x00;
    else
        SY_TX_BUF[9] = 0x13;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 设置芯片地址 */
void PS_SetChipAddr(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据 */
    _g_spt.Part2.DeviceAddress = (libdata[10] << 24) | (libdata[11] << 16) | (libdata[12] << 8) | (libdata[13]);
    if (FPSSetSpt(FMC_WRITE_START_ADDR, (uint32_t*)&_g_spt))
        SY_TX_BUF[9] = 0x01;
    else
        SY_TX_BUF[9] = 0x00;
    /* 设备地址 */
    SY_TX_BUF[2] = libdata[10];
    SY_TX_BUF[3] = libdata[11];
    SY_TX_BUF[4] = libdata[12];
    SY_TX_BUF[5] = libdata[13];
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 读取 FLASH Information Page 内容 */
void PS_ReadINFpage(uint8_t *libdadta)
{
    int i;
    int j;
    int k;
    int PackLen;
    int PackNum;
    uint32_t data[128] = {0};
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    FPSReadSpt(FMC_WRITE_START_ADDR, data);
    SY_TX_BUF[9] = 0x00;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
    /* 计算数据包长度和数量 */
    PackLen = (_g_spt.Part2.CFG_PktSize + 1) * 32;
    PackNum = 512 / PackLen;
    /* 制作数据包 */
    k = 0;
    for(i = 0; i < PackNum; i++)
    {
        /* 初始化数组 */
        memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
        memcpy(SY_TX_BUF, SY_DATA_BUF, sizeof(SY_DATA_BUF));
        /* 命令字 */
        if (i == (PackNum - 1))
            SY_TX_BUF[6] = 0x08;
        else
            SY_TX_BUF[6] = 0x02;
        /* 数据长度 */
        SY_TX_BUF[7] = ((PackLen + 2) >> 8) & 0xFF;
        SY_TX_BUF[8] = (PackLen + 2) & 0xFF;
        /* 数据 */
        for (j = 0; j < PackLen;)
        {
            SY_TX_BUF[j+9] = data[k] & 0xFF;
            SY_TX_BUF[j+10] = (data[k] >> 8) & 0xFF;
            SY_TX_BUF[j+11] = (data[k] >> 16) & 0xFF;
            SY_TX_BUF[j+12] = (data[k] >> 24) & 0xFF;
            j += 4;
            k++;
        }
        /* 校验和 */
        cksums = calc_check();
        SY_TX_BUF[9 + PackLen] = (cksums >> 8) & 0xFF;
        SY_TX_BUF[10 + PackLen] = cksums & 0xFF;
        /* 发送数据包 */
        sy_rep();
    }
}
/* 读有效模板个数 */
void PS_ValidTempleteNum(uint8_t *libdata)
{
    int ret = 0;
    uint8_t ValidN;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x05;
    /* 数据 */
    ret = TFPSLib_GetRecCount(FPDATA, &ValidN);
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            break;
        default:
            SY_TX_BUF[9] = 0x01;
            break;
    }
    if (ret == TFPRet_OK)
    {
        SY_TX_BUF[10] = (ValidN >> 8) & 0xFF;
        SY_TX_BUF[11] = ValidN & 0xFF;
    }
    else
    {
        SY_TX_BUF[10] = 0x00;
        SY_TX_BUF[11] = 0x00;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[12] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[13] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 读索引表 */
void PS_ReadIndexTable(uint8_t *libdata)
{
    uint16_t PageNum;
    uint8_t regID = 0x00U;
    uint32_t* ptrd;
    uint32_t WordNum;
    uint8_t rest = 0;
    int i;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x23;
    /* 数据 */
    SY_TX_BUF[9] = 0x00;
    PageNum = libdata[10]*256;
    ptrd = (uint32_t*)0x8080204U;
    WordNum = *ptrd;
    if (PageNum >= WordNum)
        SY_TX_BUF[9] = 0x01;
    else
    {
        if ((WordNum % 4) != 0)
            rest = 1;
        else
            rest = 0;
        for(i = 0; i < (WordNum / 4 + rest); i++){
            ptrd++;
            if ((*ptrd & 0xFF) != 0x00)
                regID |= 0x1 << (i % 2 * 4);
            if ((*ptrd & 0xFF00) != 0x00)
                regID |= 0x1 << (i % 2 * 4 + 1);
            if ((*ptrd & 0xFF0000) != 0x00)
                regID |= 0x1 << (i % 2 * 4 + 2);
            if ((*ptrd & 0xFF000000) != 0x00)
                regID |= 0x1 << (i % 2 * 4 + 3);
            if ((i + 1) % 2 == 0)
            {
                SY_TX_BUF[10 + (i / 2)] = regID;
                regID = 0x00U;
            }
        }
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[42] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[43] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 注册用获取图像 */
void PS_GetEnrollImage(uint8_t *libdata)
{
    PS_GetImage(libdata);
}
/* 获取芯片唯一序列号 */
void PS_GetChipSN(uint8_t *libdata)
{
    int i;
    uint32_t* uidCode;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x23;
    /* 数据 */
    SY_TX_BUF[9] = 0x00;
    uidCode = (uint32_t *)0x1FFFF7E8;
    for(i = 0; i < 3; i++)
    {
        SY_TX_BUF[i*4+10] = uidCode[i] & 0xFF;
        SY_TX_BUF[i*4+11] = (uidCode[i] >> 8) & 0xFF;
        SY_TX_BUF[i*4+12] = (uidCode[i] >> 16) & 0xFF;
        SY_TX_BUF[i*4+13] = (uidCode[i] >> 24) & 0xFF;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[42] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[43] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 握手指令 */
void PS_HandShake(uint8_t *libdata)
{
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    SY_TX_BUF[9] = 0x00;
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
/* 校验传感器 */
void PS_CheckSensor(uint8_t *libdata)
{
    int ret = 0;
    uint16_t cksums;

    /* 初始化数组 */
    memset(SY_TX_BUF, 0, SY_REC_LEN * sizeof(uint8_t));
    memcpy(SY_TX_BUF, SY_ACK_BUF, sizeof(SY_ACK_BUF));
    /* 数据长度 */
    SY_TX_BUF[7] = 0x00;
    SY_TX_BUF[8] = 0x03;
    /* 数据 */
    ret = TFPSLib_CapInit(FPDATA, 1);
    switch(ret)
    {
        case TFPRet_OK:
            SY_TX_BUF[9] = 0x00;
            break;
        default:
            SY_TX_BUF[9] = 0x29;
            break;
    }
    /* 校验和 */
    cksums = calc_check();
    SY_TX_BUF[10] = (cksums >> 8) & 0xFF;
    SY_TX_BUF[11] = cksums & 0xFF;
    /* 发送数据包 */
    sy_rep();
}
