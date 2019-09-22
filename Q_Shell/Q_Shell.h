#ifndef QSYS_Q_SHELL_H  
#define QSYS_Q_SHELL_H
#include "config.h"
#include <stdio.h>
#define USE_Q_SHELL_FUNCTION	 1
#define USE_THREAD_MODE			 1
#define PASSWORD_DISABLE		 1
#define PASSWORD_KEY				"123456"
#define UART_BUF_LEN 				(128)
#define SHELL_VER						"1.2.0"
#define SHELL_HEAD					"\033[1;36;40m[Wing]>>\033[0;37;40m"
#if USE_Q_SHELL_FUNCTION

typedef struct{
	BOOLEAN            Valid;	   //数组存在指令标志
	char			   gUartHist[UART_BUF_LEN];              
}GUARTHIST;

typedef const struct{
	const char*		name;	  //记录对象的名字	
	const char*		desc;	  //记录对象的描述
	void           *addr;	  //记录对象的地址
	const char*     typedesc; //当记录对象为变量时有效，变量对象的类型
    QSH_RECORD      next;     //指向下一个结构体的指针
}QSH_RECORD;

extern QSH_RECORD *qsh_record_head;
/*
功能：注册函数到函数记录段中
入参：name 函数名 desc 函数描述字符串
返回：无
举例：
...
unsigned char Var;
QSH_VAR_REG（Var,"unsigned char Var","u8");
...
 */
#define QSH_FUN_REG(name, desc)					                                \
static const   char  qsh_fun_##name##_name[]  = #name;				            \
static const   char  qsh_fun_##name##_desc[]  = desc;						    \
QSH_RECORD qsh_fun_##name##_record  __attribute__((section("qShellFunTab"))) =  \
{							                                                    \
	qsh_fun_##name##_name,	                                                    \
	qsh_fun_##name##_desc,	                                                    \
	(void *)&name,		                                                        \
	0                                                                           \
}

/*
功能：注册变量到变量记录段中
入参：name 变量名 desc 变量描述字符串 typedesc 变量类型描述字符串("u8","u16","u32"之一)
返回：无
举例：
...
unsigned char Fun(char *str, unsigned int i, char j)
{
	...
}
QSH_FUN_REG(Fun,"unsigned char Fun(char *str,unsigned int i, char j)");
...
 */
#define QSH_VAR_REG(name, desc,typedesc)					                   \
static const   char  qsh_var_##name##_name[] = #name;				           \
static const   char  qsh_var_##name##_desc[] = desc;				           \
static const   char  qsh_var_##name##_typedesc[] = typedesc;				   \
QSH_RECORD qsh_var_##name##_record  __attribute__((section("qShellVarTab"))) = \
{							                                                   \
	qsh_var_##name##_name,	                                                   \
	qsh_var_##name##_desc,	                                                   \
	(void *)&name,		                                                       \
	qsh_var_##name##_typedesc											       \
}
/* 
功能：shell对外的接口，执行命令
入参：IfCtrl 指示是否接收到一个控制字符 CmdStr 从串口得到的命令字符串
返回：1 成功 0 失败
命令字符串格式举例：
lv()                         查看一共注册了哪些变量
get(Var)                     查看变量Var的值。
set(Var,16)或set(Var,0x10)   给变量Var赋值
lf()                         查看一共注册了哪些函数
fun(“123”,0x20001000,1)    调用已注册的函数fun
read(0xE000E004)             读出寄存器的值
write(0xE000E004,0xffffffff) 给寄存器赋值
 */
unsigned int Q_Sh_CmdHandler(unsigned int IfCtrl,char *CmdStr);

/* 
功能：shell接收到TAB按键自动补齐功能
入参：name 从串口得到的命令字符串 bp 接收到的字符串个数的指针
返回：无
 */
void Q_Sh_TabComplete(char *name, unsigned char *bp);

#if USE_THREAD_MODE
/* 
功能：shell在线程中的处理接收字符
入参：name 从串口得到的命令字符串 bp 接收到的字符串个数的指针
返回：无
 */
void Q_Sh_GetInput(void);

#endif

#else
/* 
当USE_Q_SHELL_FUNCTION=0即shell功能被关闭后，为了不需要修改原程序而加入
 */
#define QSH_FUN_REG(name, desc)
#define QSH_VAR_REG(name, desc)
unsigned int Q_Sh_CmdHandler(unsigned int IfCtrl,char *CmdStr);

#endif

#endif
