#include "rt_os.h"
#include "rt_os_private.h"
#include "timer.h"

/************************ 定时器配置 ****************************/
void tick_timer_init(void)
{
    TIM_InitTypeDef		TIM_InitStructure;					//结构定义
    TIM_InitStructure.TIM_Mode      = TIM_16BitAutoReload;	//指定工作模式,   TIM_16BitAutoReload,TIM_16Bit,TIM_8BitAutoReload,TIM_16BitAutoReloadNoMask
    TIM_InitStructure.TIM_Polity    = PolityLow;			//指定中断优先级, PolityHigh,PolityLow
    TIM_InitStructure.TIM_Interrupt = ENABLE;				//中断是否允许,   ENABLE或DISABLE
    TIM_InitStructure.TIM_ClkSource = TIM_CLOCK_12T;			//指定时钟源,     TIM_CLOCK_1T,TIM_CLOCK_12T,TIM_CLOCK_Ext
    TIM_InitStructure.TIM_ClkOut    = DISABLE;				//是否输出高速脉冲, ENABLE或DISABLE
    TIM_InitStructure.TIM_Value     = 65536UL - (MAIN_Fosc * OS_TICK_RATE_MS / 12000UL);		//初值,
    TIM_InitStructure.TIM_Run       = DISABLE;				//是否初始化后启动定时器, ENABLE或DISABLE
    Timer_Inilize(Timer0,&TIM_InitStructure);				//初始化Timer0	  Timer0,Timer1,Timer2
}

void tick_timer_start(void)
{
    Timer_Start(Timer0);
}

//描述：调用此函数以延迟当前正在运行的任务的执行，直到指定的系统滴答数到期为止。
//参数：ticks是任务将以时钟'ticks'的数量暂停的时间延迟。
//请注意，通过指定0，任务不会延迟。
void os_tick_sleep(u16 ticks)
{
    if(ticks) { //当延时有效
        OS_ENTER_CRITICAL();
        os_rdy_tbl &= ~(0x01<<os_task_running_ID); //清除任务就绪表标志
        os_tcb[os_task_running_ID].OSTCBDly = ticks;
        OS_EXIT_CRITICAL();
        OS_TASK_SW(); //从新调度
    }
}

/********************* Timer0中断函数************************/
void timer0_int (void) interrupt TIMER0_VECTOR {
    u8 i;
    for(i=0; i<TASK_SIZE; i++) { //任务时钟
        if(os_tcb[i].OSTCBDly) {
            os_tcb[i].OSTCBDly--;
            if(os_tcb[i].OSTCBDly == 0) { //当任务时钟到时,必须是由定时器减时的才行
                os_rdy_tbl |= (0x01<<i); //使任务在就绪表中置位
            }
        }
    }
}