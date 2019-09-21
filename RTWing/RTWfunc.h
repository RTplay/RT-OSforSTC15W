/********************************************************************************************************
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
                                                                                                    
RTWFUNC.H  file

���ߣ�RZ
��������: 2014.6.19
�޸�����: 2014.6.19
�汾��V1.0
 
All rights reserved            
*******************************************************************/

#ifndef __RTWFUNC_H__
#define __RTWFUNC_H__

typedef int (*syscall_func)(int argc, char *argv[]);

struct CLI_RTWING 
{
	/** The name of the CLI command */
	char *func_name;
	/** The help text associated with the command */
	char *func_desc;
	/** The function that should be invoked for this command. */
	int (*function) (int argc, char *argv[]);
};

#define RTWING_FUNCTION_EXPORT(name, desc) 	\
{																						\
		#name,    															\
		#desc,    															\
		(syscall_func)&name     																\
}

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

int hello(int argc, char *argv[]);
int version(int argc, char *argv[]);

#endif