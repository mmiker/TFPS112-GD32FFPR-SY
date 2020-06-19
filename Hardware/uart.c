#include "uart.h"

static rcu_periph_enum COM_CLK[COMn] = {EVAL_COM0_CLK, EVAL_COM1_CLK};
static uint32_t COM_TX_PIN[COMn] = {EVAL_COM0_TX_PIN, EVAL_COM1_TX_PIN};
static uint32_t COM_RX_PIN[COMn] = {EVAL_COM0_RX_PIN, EVAL_COM1_RX_PIN};
static uint32_t COM_GPIO_PORT[COMn] = {EVAL_COM0_GPIO_PORT, EVAL_COM1_GPIO_PORT};
static rcu_periph_enum COM_GPIO_CLK[COMn] = {EVAL_COM0_GPIO_CLK, EVAL_COM1_GPIO_CLK};
uint8_t  USART_RX_BUF[USART_REC_LEN];
uint16_t USART_RX_STA;
uint32_t USART_REC;
uint16_t USART_LEN;
uint8_t USART_RX_FLG;

/*!
    \brief      configure COM port
    \param[in]  com: COM on the board
      \arg        EVAL_COM0: COM0 on the board
      \arg        EVAL_COM1: COM1 on the board
    \param[out] none
    \retval     none
*/
void com_init(uint32_t usart_periph, uint32_t baudval, uint32_t stblen)
{
    uint32_t com_id = 0U;
    if(EVAL_COM0 == usart_periph){
        com_id = 0U;
    }else if(EVAL_COM1 == usart_periph){
        com_id = 1U;
    }
    
    /* enable GPIO clock */
    rcu_periph_clock_enable(COM_GPIO_CLK[com_id]);

    /* enable USART clock */
    rcu_periph_clock_enable(COM_CLK[com_id]);

    /* connect port to USARTx_Tx */
    gpio_init(COM_GPIO_PORT[com_id], GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, COM_TX_PIN[com_id]);

    /* connect port to USARTx_Rx */
    gpio_init(COM_GPIO_PORT[com_id], GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, COM_RX_PIN[com_id]);

    /* USART configure */
    usart_deinit(usart_periph);
    usart_stop_bit_set(usart_periph, stblen);
    usart_baudrate_set(usart_periph, baudval);
    usart_receive_config(usart_periph, USART_RECEIVE_ENABLE);
    usart_transmit_config(usart_periph, USART_TRANSMIT_ENABLE);
    usart_enable(usart_periph);
}

/*!
    \brief      this function handles USART RBNE interrupt request and TBE interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void USART0_IRQHandler(void)
{
    uint8_t Res;

    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE)){
        /* receive data */
        Res = usart_data_receive(USART0);

        USART_RX_BUF[USART_REC] = Res;
        if (USART_RX_STA & 0x8000)
        {
            USART_REC++;
            if (USART_REC > 8)
            {
                USART_LEN = (USART_RX_BUF[7] << 8) | USART_RX_BUF[8];
                if (USART_REC > (8 + USART_LEN))
                {
                    USART_REC = 0;
                    USART_RX_STA = 0;
                    USART_RX_FLG = 1;
                }
                else
                    USART_RX_FLG = 0;
            }
            else
                USART_LEN = 0;
        }
        else
        {
            if (USART_RX_STA & 0x4000)
            {
                if (Res == 0x01)
                {
                    USART_REC++;
                    USART_RX_STA |= 0x8000;
                }
                else
                {
                    USART_REC = 0;
                    USART_RX_STA = 0;
                }
            }
            else
            {
                if (Res == 0xEF)
                {
                    USART_REC++;
                    USART_RX_STA |= 0x4000;
                }
                else
                {
                    USART_REC = 0;
                    USART_RX_STA = 0;
                }
            }
        }
    }
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));

    return ch;
}
