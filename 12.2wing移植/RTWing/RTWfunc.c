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

RTWFUNC.C  file

作者：RZ
建立日期: 2014.6.19
修改日期: 2014.6.19
版本：V1.0

All rights reserved
***********************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int hello(int argc, char *argv[])
{
    argc = argc;
    argv = argv;
    debug_print("\r\nhello rtwing!\r\n");
    return 0;
}

int version(int argc, char *argv[])
{
    argc = argc;
    argv = argv;
    debug_print("\r\nRTWing version 0.0.2!\r\n");
    return 0;
}

int status(int argc, char *argv[])
{
    u8 i;
    OS_SS *ss = get_sys_statistics();
    argc = argc;
    argv = argv;
    printf("\r\nID\tCPU\tSTATUS\tUSED STACK\r\n");
    for (i=0; i<TASK_SIZE; i++) {
        if (ss[i].OSSSStatus != OS_STAT_DEFAULT) {//任务已
            printf("%bu\t%bu%%\t", i, ss[i].OSSSCyclesTot);
            switch (ss[i].OSSSStatus) {
            case OS_STAT_RUNNING:
                printf("RUN");
                break;
            case OS_STAT_RDY:
                printf("RDY");
                break;
            case OS_STAT_SLEEP:
                printf("SLEEP");
                break;
            case OS_STAT_SUSPEND:
                printf("SUSPEND");
                break;
            case OS_STAT_DEAD:
                printf("DEAD");
                break;
            case OS_STAT_MUTEX:
                printf("MUTEX");
                break;
            case OS_STAT_SEM:
                printf("SEM");
                break;
            case OS_STAT_MSGQ:
                printf("MSGQ");
                break;
            case OS_STAT_FLAG:
                printf("FLAG");
                break;
            default:
                printf("unknown");
            }
            printf("\t%bu\r\n", ss[i].OSSSMaxUsedStk);
        }
    }
    return 0;
}

//Sign up here call the function module
const struct CLI_RTWING rtwing_syscall[] = {
    RTWING_FUNCTION_EXPORT(hello, hello rtwing!),
    RTWING_FUNCTION_EXPORT(version, rtwing version!),
    RTWING_FUNCTION_EXPORT(status, get tasks status!),
};
