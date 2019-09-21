/*****************************************************************************************************
                                                          #
              ##################### #######################           ##
        ######                     ##                    ##          ####
       ##                             ##### #  ###########          ## ##
      ## ########  ##  ## #######    ##     ###                    ##  #
      ####         #  #         ##  ##     # ##                    #  ##
                  #  #          ## ##     # ##                    #  ##
                 #  #        ### ###     ####                    #   #
                #  #  ###########       ####                    ##  #
               #  #  ##   ###          ## #                    ##  ##
              #  # ##  ###            ## #          ##         #  ##
             #  #  #  ##              # #          # ##       #  ##            ###    #     ####
            #  #  ##   ##            # ##         #   ###    ##  #        ###### ## ####   ##  #
           #  ##  ##    ##          # ##         ##      #  ##  ##      ##       #  #  #  ##  #
          #  ##    #     ##        #  #         #   ####  # #  ##      ##  ###  #  ## ## ##  #
         ## ##     ##     ###     #  #         #   #  ##  ##   #     ##  ## #  #   #  # ## ##
        ## ##       ##      #### #  ##        ##  #   #   #   #     ##  ## ## #   ##  ### ##
       ##  #          ##        ## ##         #  #  ##  ##   ## #  ##   # ## ## ###  ##  ##
      ##  #            #####  ### ##         #  ## ##  ###       ####   ##    ## ##     ##
     ##  #                                  ##       ## ##        ##    ##   ##  #     ##
    ##  ##                                  #       ##  ##########  #### ####    ##   ##
    #   #                                  #      ##                             #   ##
    ####                                  #  #####                              #   #
                                         ## ##                                 #   #
                                        ## ##                                 #  ##
                                       ##  #                                 ## ##
                                       #  #                                  ####
                                        ###

RTWING.C  file

作者：RZ
建立日期: 2014.6.17
修改日期: 2014.6.17
版本：V1.0

All rights reserved
***********************************************************************************************************/
#include "debug_uart.h"
#include "rt_os.h"
#include "RTWing.h"
#include "RTWfunc.h"

#include "RTWfunc.c"

u8 ReceString[6][50] = {0};
u8 StringIndex = 0;
u8 CharPosition = 0;
u8 CurrentData[50] = {0};
u8 CurrentPosition = 0;
u8 *argv[5];

/********************************************************************
函数功能：RTWing初始化。
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void InitRTWing(void)
{
    u8 j;
    for (j = 0; j < 5; j++) {
        argv[j] = ReceString[j+1];
    }
}
////////////////////////End of function//////////////////////////////

/********************************************************************
函数功能：RTWING处理过程。
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void RTWingProcess(u8 *recv_data, u8 data_len)
{
    u8 i, j;
    u8 spt[15] = {0};
    u8 data_ele = 0;
    u8 funced = 0;
    
    if(data_len > 0) {  //收到数据
    u8 data_cnt;
    for (data_cnt = 0; data_cnt<data_len; data_len++)
        data_ele = recv_data[data_cnt];
        if((data_ele == '\n')||(data_ele == '\r')) { //回车
            if(CurrentPosition != 0) {
                i = 0;
                StringIndex = 0;
                CharPosition = 0;
                while(i <= CurrentPosition) {
                    if(CurrentData[i] == 0x20) {
                        StringIndex ++;
                        CharPosition = 0;
                    } else {
                        ReceString[StringIndex][CharPosition++] = CurrentData[i];
                    }
                    i++;
                }
            }

            if(CurrentPosition != 0) {
                for (j = 0; j < ARRAY_LENGTH(rtwing_syscall); j++) {
                    if(strcmp(ReceString[0], rtwing_syscall[j].func_name) == 0) {
                        sprintf(spt, "0x%04X", rtwing_syscall[j].function((int)StringIndex, argv));
                        debug_print("\r\nreturn ");
                        debug_print(spt);
                        funced = 1;
                    }
                }
                if(funced == 0) {
                    debug_print("\r\nThe function can not be executed!");
                }
            }
            memset(ReceString,'\0',300);
            memset(CurrentData,'\0',50);
            CurrentPosition = 0;
            StringIndex = 0;
            debug_print("\r\n[RTWing]: ");
        } else if(data_ele == '\b') { //退格
            if(CurrentPosition > 0) {
                CurrentPosition --;
                CurrentData[CurrentPosition] = '\0';
                debug_print("\b \b");
            }
        } else if(data_ele == '\t') { //tab
            if(CurrentPosition == 0) {
                debug_print("\r\n");
                for (j = 0; j < ARRAY_LENGTH(rtwing_syscall); j++) {
                    debug_print(rtwing_syscall[j].func_name);
                    debug_print("\t\t");
                    debug_print(rtwing_syscall[j].func_desc);
                    debug_print("\r\n");
                }
                debug_print("\r\n[RTWing]: ");
            } else {
                for (j = 0; j < ARRAY_LENGTH(rtwing_syscall); j++) {
                    if(strncmp(CurrentData, rtwing_syscall[j].func_name, strlen(CurrentData)) == 0) {
                        spt[funced] = j;
                        funced ++;
                    }
                }
                if(funced == 1) {
                    for (j = 0; j < CurrentPosition; j++) {
                        debug_print("\b \b");
                    }
                    CurrentPosition = strlen(rtwing_syscall[spt[funced-1]].func_name);
                    memcpy(CurrentData, rtwing_syscall[spt[funced-1]].func_name, CurrentPosition);
                    debug_print(rtwing_syscall[spt[funced-1]].func_name);
                } else if(funced > 1) {
                    j = 0;
                    debug_print("\r\n");
                    while(j < funced) {
                        debug_print(rtwing_syscall[spt[j]].func_name);
                        debug_print("\t\t");
                        debug_print(rtwing_syscall[spt[j]].func_desc);
                        debug_print("\r\n");
                        j ++;
                    }
                    debug_print("[RTWing]: ");
                    debug_print(CurrentData);
                }
            }
        } else if(data_ele == ' ') { //空格
            if((StringIndex < 5)&&(CurrentPosition != 0)&&(CurrentData[CurrentPosition-1] != 0x20)) {
                StringIndex ++;
                CurrentData[CurrentPosition] = data_ele;
                CurrentPosition ++;
                debug_print_char(data_ele);
            } else if(StringIndex >= 5) {
                debug_print("\r\nReached the maximum number of parameters!\r\n");
                debug_print("[RTWing]: ");
                debug_print(CurrentData);
            }
        } else {
            if(CurrentPosition < 50) {
                CurrentData[CurrentPosition] = data_ele;
                CurrentPosition ++;
                debug_print_char(data_ele);
            } else {
                debug_print("\r\nReached the maximum number of string length!\r\n");
                debug_print("[RTWing]: ");
                debug_print(CurrentData);
            }
        }
    }
}
////////////////////////End of function//////////////////////////////

/********************************************************************
函数功能：打印RTWING版本信息等函数。
入口参数：无。
返    回：无。
备    注：无。
********************************************************************/
void PrintRTWing(void)
{
    debug_print("\r\nRTWing 0.0.2 Bulid ");
    debug_print(__DATE__);
    debug_print(" ");
    debug_print(__TIME__);
    debug_print("\r\n");
    debug_print("2014 Develop RZ\r\n");
    debug_print("[RTWing]: ");
}
////////////////////////End of function//////////////////////////////
