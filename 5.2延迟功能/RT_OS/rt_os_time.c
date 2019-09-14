#include "rt_os.h"
#include "rt_os_private.h"
#include "timer.h"

/************************ ��ʱ������ ****************************/
void tick_timer_init(void)
{
    TIM_InitTypeDef		TIM_InitStructure;					//�ṹ����
    TIM_InitStructure.TIM_Mode      = TIM_16BitAutoReload;	//ָ������ģʽ,   TIM_16BitAutoReload,TIM_16Bit,TIM_8BitAutoReload,TIM_16BitAutoReloadNoMask
    TIM_InitStructure.TIM_Polity    = PolityLow;			//ָ���ж����ȼ�, PolityHigh,PolityLow
    TIM_InitStructure.TIM_Interrupt = ENABLE;				//�ж��Ƿ�����,   ENABLE��DISABLE
    TIM_InitStructure.TIM_ClkSource = TIM_CLOCK_12T;			//ָ��ʱ��Դ,     TIM_CLOCK_1T,TIM_CLOCK_12T,TIM_CLOCK_Ext
    TIM_InitStructure.TIM_ClkOut    = DISABLE;				//�Ƿ������������, ENABLE��DISABLE
    TIM_InitStructure.TIM_Value     = 65536UL - (MAIN_Fosc * OS_TICK_RATE_MS / 12000UL);		//��ֵ,
    TIM_InitStructure.TIM_Run       = DISABLE;				//�Ƿ��ʼ����������ʱ��, ENABLE��DISABLE
    Timer_Inilize(Timer0,&TIM_InitStructure);				//��ʼ��Timer0	  Timer0,Timer1,Timer2
}

void tick_timer_start(void)
{
    Timer_Start(Timer0);
}

//���������ô˺������ӳٵ�ǰ�������е������ִ�У�ֱ��ָ����ϵͳ�δ�������Ϊֹ��
//������ticks��������ʱ��'ticks'��������ͣ��ʱ���ӳ١�
//��ע�⣬ͨ��ָ��0�����񲻻��ӳ١�
void os_tick_sleep(u16 ticks)
{
    if(ticks) { //����ʱ��Ч
        OS_ENTER_CRITICAL();
        os_rdy_tbl &= ~(0x01<<os_task_running_ID); //�������������־
        os_tcb[os_task_running_ID].OSTCBDly = ticks;
        OS_EXIT_CRITICAL();
        OS_TASK_SW(); //���µ���
    }
}

/********************* Timer0�жϺ���************************/
void timer0_int (void) interrupt TIMER0_VECTOR {
    u8 i;
    for(i=0; i<TASK_SIZE; i++) { //����ʱ��
        if(os_tcb[i].OSTCBDly) {
            os_tcb[i].OSTCBDly--;
            if(os_tcb[i].OSTCBDly == 0) { //������ʱ�ӵ�ʱ,�������ɶ�ʱ����ʱ�Ĳ���
                os_rdy_tbl |= (0x01<<i); //ʹ�����ھ���������λ
            }
        }
    }
}