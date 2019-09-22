#include "config.h"
#include "Q_Shell.h"
#include <string.h>
#include <stdio.h>

#if USE_Q_SHELL_FUNCTION      
QSH_RECORD *qsh_record_head;
//环境变量不占用内存
extern int qShellFunTab$$Base;	//函数记录段段首
extern int qShellFunTab$$Limit;	//函数记录段段尾
extern int qShellVarTab$$Base;	//变量记录段段首
extern int qShellVarTab$$Limit;	//变量记录段段尾

#define QSH_FUN_PARA_MAX_NUM 4//函数最多支持四个参数

#if USE_THREAD_MODE == 0
//指令保存数组
GUARTHIST gUartHist[10];
#endif

typedef unsigned int (*QSH_FUNC_TYPE)();//统一的shell函数类型支持不同个数的参数

typedef enum{
	QSCT_ERROR,        //无效调用
	QSCT_LIST_FUN,     //列出所有可被shell访问的函数
	QSCT_LIST_VAR,     //列出所有可被shell访问的变量
	QSCT_CLC,		   //清屏
	QSCT_RESET,		   //软复位
	QSCT_CALL_FUN,     //调用函数
	QSCT_READ_VAR,     //查看变量的值
	QSCT_WRITE_VAR,    //给变量赋值
	QSCT_READ_REG,     //读寄存器
	QSCT_WRITE_REG     //写寄存器
}Q_SH_CALL_TYPE;       //shell调用类型

typedef enum{
	QSFPT_NOSTRING,   //非字符串型参数
	QSFPT_STRING      //字符串型参数
}Q_SH_FUN_PARA_TYPE;  //函数参数类型

typedef struct{
	char               *CmdStr;    //命令字符串首地址
	Q_SH_CALL_TYPE     CallType;   //shell调用类型
	unsigned char      CallStart;  //shell调用名在命令字符串中的起始位置                   
	unsigned char      CallEnd;    //shell调用名在命令字符串中的结束位置 
	unsigned char      ParaNum;    //shell调用的参数个数
	unsigned char      ParaStart[QSH_FUN_PARA_MAX_NUM];//每个参数在命令字符串中的起始位置   
	unsigned char      ParaEnd[QSH_FUN_PARA_MAX_NUM];  //每个参数在命令字符串中的结束位置   
	Q_SH_FUN_PARA_TYPE ParaTypes[QSH_FUN_PARA_MAX_NUM];//每个参数的类型               
}Q_SH_LEX_RESU_DEF;
/* 
功能：在函数记录段寻找指定的函数
入参：函数名
返回：对应的函数记录
 */
static QSH_RECORD* Q_Sh_SearchFun(char* Name)
{
	QSH_RECORD* Index;
	for (Index = (QSH_RECORD*) &qShellFunTab$$Base; Index < (QSH_RECORD*) &qShellFunTab$$Limit; Index ++)
	{
		if (strcmp(Index->name, Name) == 0)
			return Index;
	}
	return (void *)0;
}
/* 
功能：在函数记录段寻找指定的变量
入参：变量名
返回：对应的变量记录
 */
static QSH_RECORD* Q_Sh_SearchVar(char* Name)
{
	QSH_RECORD* Index;
	for (Index = (QSH_RECORD*) &qShellVarTab$$Base; Index < (QSH_RECORD*) &qShellVarTab$$Limit; Index ++)
	{
		if (strcmp(Index->name, Name) == 0)
			return Index;
	}
	return (void *)0;
}
/* 
功能：列出函数段中的所有函数记录的描述字符串
入参：无
返回：无
 */
static void Q_Sh_ListFuns(void)
{
	QSH_RECORD* Index;
	for (Index = (QSH_RECORD*) &qShellFunTab$$Base; Index < (QSH_RECORD*) &qShellFunTab$$Limit; Index++)
	{	
		printf("%s\t%s\r\n", Index->name, Index->desc);
	}
}
/*
功能：列出变量段中的所有变量记录的描述字符串
入参：无
返回：无 
 */
static void Q_Sh_ListVars(void)
{
	QSH_RECORD* Index;
	for (Index = (QSH_RECORD*) &qShellVarTab$$Base; Index < (QSH_RECORD*) &qShellVarTab$$Limit; Index++)
	{
		printf("%s\r\n",Index->desc);
	}
}
/* 
功能：清除屏幕上的字符
入参：无
返回：无
 */
static void Q_Sh_Clc(void)
{
	printf("\r\n\x0c\r\n\r\n");
}
/* 
功能：软复位系统
入参：无
返回：无
 */
static void Q_Sh_Reset(void)
{
	NVIC_SystemReset();
}

/*
功能：计算m的n次方
入参：m 底数 n 幂指数
返回：计算结果 
 */
static unsigned int Q_Sh_Pow(unsigned int m,unsigned int n)
{
	unsigned int Result=1;	 
	while(n--)
		Result*=m;    
	return Result;
}
/*
功能：将字符转换成数字，支持16进制（大小写字母均可），不支持负数
入参：Str 数字字符串 Res 结果存放的地址
返回：0 成功 ！0 失败
 */
static unsigned int Q_Sh_Str2Num(char*Str,unsigned int *Res)
{
	unsigned int Temp;
	unsigned int Num=0;			  
	unsigned int HexDec=10;
	char *p;
	p=Str;
	*Res=0;
	while(1)
	{
		if((*p>='A'&&*p<='F')||(*p=='X'))
			*p=*p+0x20;
		else if(*p=='\0')break;
		p++;
	}
	p=Str;
	while(1)
	{
		if((*p<='9'&&*p>='0')||(*p<='f'&&*p>='a')||(*p=='x'&&Num==1))
		{
			if(*p>='a')HexDec=16;	
			Num++;					
		}else if(*p=='\0')break;	
		else return 1;				
		p++; 
	} 
	p=Str;			    
	if(HexDec==16)		
	{
		if(Num<3)return 2;			
		if(*p=='0' && (*(p+1)=='x'))
		{
			p+=2;	
			Num-=2;
		}else return 3;
	}else if(Num==0)return 4;
	while(1)
	{
		if(Num)Num--;
		if(*p<='9'&&*p>='0')Temp=*p-'0';	
		else Temp=*p-'a'+10;				 
		*Res+=Temp*Q_Sh_Pow(HexDec,Num);		   
		p++;
		if(*p=='\0')break;
	}
	return 0;
}	
/*
功能：从母字符串中截得子字符串
入参：Base 母字符串首地址 Start 待截子字符的起始位置 End 待截字符串结束位置
返回：子字符串首地址
 */
static char *Q_Sh_StrCut(char *Base,unsigned int Start,unsigned int End)
{
	Base[End+1]=0;
	return (char *)((unsigned int)Base+Start);
}
/*
功能：对输入的命令字符串进行词法分析
入参： CmdStr 命令字符串首地址 pQShLexResu 分析结果存放处
返回：0 词法分析失败 1 词法分析成功
 */
static unsigned int Q_Sh_LexicalAnalysis(char *CmdStr,Q_SH_LEX_RESU_DEF *pQShLexResu)
{
	unsigned int Index=0,TmpPos=0,ParamNum=0;
		
	while(1)
	{
		if( CmdStr[0] == '\0' )
			return 0;
		
		if( !( ( CmdStr[Index]>='a' && CmdStr[Index]<='z' ) || ( CmdStr[Index]>='A' && CmdStr[Index]<='Z' ) || ( CmdStr[Index]>='0' && CmdStr[Index]<='9' ) || CmdStr[Index]=='_' || CmdStr[Index]=='(') )
		{
			pQShLexResu->CallType=QSCT_ERROR;
			printf("Lexical Analysis Error: Function or Call name is illegal!!!\r\n");
			return 0;
		}
		if(CmdStr[Index]=='(')
		{
			if(Index==0)
			{
				pQShLexResu->CallType=QSCT_ERROR;
				printf("Lexical Analysis Error: Function or Call name is empty!!!\r\n");
				return 0;
			}
			pQShLexResu->CallEnd=(Index-1);
			TmpPos=++Index;
			break;
		}
		Index++;
	}
	
	while(1)
	{
		if( CmdStr[Index]<32||CmdStr[Index]>126 )
		{
			pQShLexResu->CallType=QSCT_ERROR;
			printf("Lexical Analysis Error: parameter include illegal character!!!\r\n");
			return 0;
		}
		if(CmdStr[Index]==',')
		{
			if(Index==TmpPos)
			{
				pQShLexResu->CallType=QSCT_ERROR;
				printf("Lexical Analysis Error: parameter include illegal comma!!!\r\n");
				return 0;
			}
			if(CmdStr[Index-1]=='\"')
			{
				if( ParamNum<QSH_FUN_PARA_MAX_NUM )
				{
					pQShLexResu->ParaTypes[ParamNum]=QSFPT_STRING;
					pQShLexResu->ParaStart[ParamNum]=TmpPos+1;
					pQShLexResu->ParaEnd[ParamNum]=Index-2;
					ParamNum++;
				}
				else
				{
					ParamNum++;
				}
				TmpPos=Index+1;
			}
			else
			{
				if( ParamNum<QSH_FUN_PARA_MAX_NUM )
				{
					pQShLexResu->ParaTypes[ParamNum]=QSFPT_NOSTRING;
					pQShLexResu->ParaStart[ParamNum]=TmpPos;
					pQShLexResu->ParaEnd[ParamNum]=Index-1;
					ParamNum++;
				}
				else
				{
					ParamNum++;
				}
				TmpPos=Index+1;
			}
		}
		else if(CmdStr[Index]==')')
		{
			if(Index==pQShLexResu->CallEnd+2)
			{
				pQShLexResu->ParaNum=0;
				break;
			}
			if(Index==TmpPos)
			{
				pQShLexResu->CallType=QSCT_ERROR;
				printf("Lexical Analysis Error: parameter include illegal comma!!!\r\n");
				return 0;
			}
			if(CmdStr[Index-1]=='\"')
			{
				if( ParamNum<QSH_FUN_PARA_MAX_NUM )
				{
					pQShLexResu->ParaTypes[ParamNum]=QSFPT_STRING;
					pQShLexResu->ParaStart[ParamNum]=TmpPos+1;
					pQShLexResu->ParaEnd[ParamNum]=Index-2;
					ParamNum++;
				}
				else
				{
					ParamNum++;
				}
				pQShLexResu->ParaNum=ParamNum;
				break;
			}
			else
			{
				if( ParamNum<QSH_FUN_PARA_MAX_NUM )
				{
					pQShLexResu->ParaTypes[ParamNum]=QSFPT_NOSTRING;
					pQShLexResu->ParaStart[ParamNum]=TmpPos;
					pQShLexResu->ParaEnd[ParamNum]=Index-1;
					ParamNum++;
				}
				else
				{
					ParamNum++;
				}
				pQShLexResu->ParaNum=ParamNum;
				break;
			}	
		}
		Index++;
	}
	pQShLexResu->CmdStr=CmdStr;
	pQShLexResu->CallStart=0;
	if( strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"lf")==0 )
		pQShLexResu->CallType=QSCT_LIST_FUN;
	else if(strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"lv")==0)
		pQShLexResu->CallType=QSCT_LIST_VAR;
	else if(strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"clear")==0)
		pQShLexResu->CallType=QSCT_CLC;
	else if(strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"reset")==0)
		pQShLexResu->CallType=QSCT_RESET;
	else if(strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"get")==0)
		pQShLexResu->CallType=QSCT_READ_VAR;
	else if(strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"set")==0)
		pQShLexResu->CallType=QSCT_WRITE_VAR;
	else if(strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"read")==0)
		pQShLexResu->CallType=QSCT_READ_REG;
	else if(strcmp(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd),"write")==0)
		pQShLexResu->CallType=QSCT_WRITE_REG;
	else
		pQShLexResu->CallType=QSCT_CALL_FUN;
	return 1;
}
/*
功能：对词法分析的结果进行语法分析，并执行
入参： pQShLexResu 词法分析结果存放处
返回：0 失败 1 成功
 */
static unsigned int Q_Sh_ParseAnalysis(Q_SH_LEX_RESU_DEF *pQShLexResu)
{
	QSH_RECORD *pRecord;
	unsigned int FunPara[QSH_FUN_PARA_MAX_NUM];
	unsigned int FunResu;
	unsigned int VarSet;
	unsigned int VarAddr;
	unsigned char i;
	unsigned char Ret=1;
	switch(pQShLexResu->CallType)
	{
		case QSCT_CALL_FUN:
		{
			pRecord=Q_Sh_SearchFun(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->CallStart,pQShLexResu->CallEnd));
			if( pRecord == 0 )
			{
				printf("Parse Analysis Error: the input function have not been regist!!!\r\n");
				Ret=0;
				break;
			}			
			if(pQShLexResu->ParaNum>QSH_FUN_PARA_MAX_NUM)
			{	
				printf("Parse Analysis Error: function's parameter number can't over four!!!\r\n");
				Ret=0;
				break;
			}
			for(i=0;i<pQShLexResu->ParaNum;i++)
			{
				if(pQShLexResu->ParaTypes[i]==QSFPT_STRING)
				{
					FunPara[i]=(unsigned int)Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[i],pQShLexResu->ParaEnd[i]);					
				}
				else
				{
					if(Q_Sh_Str2Num(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[i],pQShLexResu->ParaEnd[i]),&(FunPara[i]))!=0)
					{
						printf("Parse Analysis Error: the input number string is illegal!!!\r\n");
						Ret=0;
						break;
					}
				}
			}
			FunResu=((QSH_FUNC_TYPE)(pRecord->addr))(FunPara[0],FunPara[1],FunPara[2],FunPara[3]);
			printf("return %d\r\n",FunResu);
		}
		break;
		
		case QSCT_READ_VAR:
		{
			pRecord=Q_Sh_SearchVar(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[0],pQShLexResu->ParaEnd[0]));
			if( pRecord == 0 )
			{
				printf("Parse Analysis Error: the input variable have not been regist!!!\r\n");
				Ret=0;
				break;
			}
			if(pQShLexResu->ParaNum!=1)
			{	
				printf("Parse Analysis Error: this call must have only one parameter!!!\r\n");
				Ret=0;
				break;
			}
			if(strcmp(pRecord->typedesc,"u8")==0)
				printf("%s=%d\r\n",pRecord->name,*(unsigned char *)(pRecord->addr));
			else if(strcmp(pRecord->typedesc,"u16")==0)
				printf("%s=%d\r\n",pRecord->name,*(unsigned short *)(pRecord->addr));
			else if(strcmp(pRecord->typedesc,"u32")==0)
				printf("%s=%d\r\n",pRecord->name,*(unsigned int *)(pRecord->addr));
			else
			{
				printf("the describe of variable's type is illegal!!!\r\n");
				Ret=0;
				break;
			}
		}
		break;

		case QSCT_WRITE_VAR:
		{
			pRecord=Q_Sh_SearchVar(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[0],pQShLexResu->ParaEnd[0]));
			if( pRecord == 0 )
			{
				printf("Parse Analysis Error: the input variable have not been regist!!!\r\n");
				Ret=0;
				break;
			}
			if(pQShLexResu->ParaNum!=2)
			{	
				printf("Parse Analysis Error: this call must have two parameter!!!\r\n");
				Ret=0;
				break;
			}
			if(pQShLexResu->ParaTypes[1]==QSFPT_STRING)
			{
				printf("Parse Analysis Error: this call's second parameter can't be string type!!!\r\n");
				Ret=0;
				break;
			}
			if(Q_Sh_Str2Num(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[1],pQShLexResu->ParaEnd[1]),&VarSet)!=0)
			{
				printf("Parse Analysis Error: the input number string is illegal!!!\r\n");
				Ret=0;
				break;
			}
			if(strcmp(pRecord->typedesc,"u8")==0)
			{
				*(unsigned char *)(pRecord->addr)=VarSet;
			    printf("%s=%d\r\n",pRecord->name,*(unsigned char *)(pRecord->addr));	
			}
			else if(strcmp(pRecord->typedesc,"u16")==0)
			{
				*(unsigned short *)(pRecord->addr)=VarSet;
			    printf("%s=%d\r\n",pRecord->name,*(unsigned short *)(pRecord->addr));	
			}
			else if(strcmp(pRecord->typedesc,"u32")==0)
			{
				*(unsigned int *)(pRecord->addr)=VarSet;
			    printf("%s=%d\r\n",pRecord->name,*(unsigned int *)(pRecord->addr));	
			}
			else
			{
				printf("the describe of variable's type is illegal!!!\r\n");
				Ret=0;
				break;
			}
		}
		break;
		
		case QSCT_READ_REG:
		{
			if(pQShLexResu->ParaNum!=1)
			{	
				printf("Parse Analysis Error: this call must have only one parameter!!!\r\n");
				Ret=0;
				break;
			}
			if(pQShLexResu->ParaTypes[0]==QSFPT_STRING)
			{
				printf("Parse Analysis Error: this call's parameter can not be string type!!!\r\n");
				Ret=0;
				break;
			}
			if(Q_Sh_Str2Num(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[0],pQShLexResu->ParaEnd[0]),&VarAddr)!=0)
			{
				printf("Parse Analysis Error: the input number string is illegal!!!\r\n");
				Ret=0;
				break;
			}
			printf("*(0x%x)=0x%x\r\n",VarAddr,*(unsigned int *)VarAddr);
		}
		break;
		
		case QSCT_WRITE_REG:
		{
			if(pQShLexResu->ParaNum!=2)
			{	
				printf("Parse Analysis Error: this call must have two parameter!!!\r\n");
				Ret=0;
				break;
			}
			if(pQShLexResu->ParaTypes[0]==QSFPT_STRING)
			{
				printf("Parse Analysis Error: this call's first parameter can not be string type!!!\r\n");
				Ret=0;
				break;
			}
			if(pQShLexResu->ParaTypes[1]==QSFPT_STRING)
			{
				printf("Parse Analysis Error: this call's second parameter can not be string type!!!\r\n");
				Ret=0;
				break;
			}
			if(Q_Sh_Str2Num(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[0],pQShLexResu->ParaEnd[0]),&VarAddr)!=0)
			{
				printf("Parse Analysis Error: the input number string is illegal!!!\r\n");
				Ret=0;
				break;
			}
			if(Q_Sh_Str2Num(Q_Sh_StrCut(pQShLexResu->CmdStr,pQShLexResu->ParaStart[1],pQShLexResu->ParaEnd[1]),&VarSet)!=0)
			{
				printf("Parse Analysis Error: the input number string is illegal!!!\r\n");
				Ret=0;
				break;
			}
			*(unsigned int *)VarAddr=VarSet;
			printf("*(0x%x)=0x%x\r\n",VarAddr,*(unsigned *)VarAddr);
		}
		break;
		
		case QSCT_LIST_FUN:
		{
			if(pQShLexResu->ParaNum>0)
			{	
				printf("Parse Analysis Error: this call must have no parameter!!!\r\n");
				Ret=0;
				break;
			}
			Q_Sh_ListFuns();
		}
		break;
		
		case QSCT_LIST_VAR:
		{
			if(pQShLexResu->ParaNum>0)
			{	
				printf("Parse Analysis Error: this call must have no parameter!!!\r\n");
				Ret=0;
				break;
			}
			Q_Sh_ListVars();
		}
		break;
		
		case QSCT_CLC:
		{
			if(pQShLexResu->ParaNum>0)
			{	
				printf("Parse Analysis Error: this call must have no parameter!!!\r\n");
				Ret=0;
				break;
			}
			Q_Sh_Clc();
		}
		break;
		
		case QSCT_RESET:
		{
			if(pQShLexResu->ParaNum>0)
			{	
				printf("Parse Analysis Error: this call must have no parameter!!!\r\n");
				Ret=0;
				break;
			}
			Q_Sh_Reset();
		}
		break;
		
		default:
		{	
			printf("Parse Analysis Error: can't get here!!!\r\n");
			Ret=0;
		}
	}
	return Ret;
}
/* 
shell密码存放处，开机后第一次使用shell需要输入此密码
 */
static const char Q_ShellPassWord[]=PASSWORD_KEY;

/* 
功能：shell对外的接口，执行命令
入参：IfCtrl 指示是否接收到一个控制字符 CmdStr 从串口得到的命令字符串
返回：1 成功 0 失败
 */
#if USE_THREAD_MODE == 0
int HistCon;//历史命令调用计数
int i;
extern u8 Idxx;
extern char gUartBufIT[UART_BUF_LEN];//串口缓冲
#endif
unsigned int Q_Sh_CmdHandler(unsigned int IfCtrl,char *CmdStr)
{
	Q_SH_LEX_RESU_DEF Q_Sh_LexResu;
	static unsigned char IfPass=PASSWORD_DISABLE,IfWaitInput=0;
	if(IfPass)
	{
		if(IfCtrl==1)
		{
			if(((unsigned short *)CmdStr)[0]==0x445b)//left key
			{
//				printf("CtrlCode:%x\n\r",((unsigned short *)CmdStr)[0]);
			}
			else if(((unsigned short *)CmdStr)[0]==0x435b)//right key
			{
//				printf("CtrlCode:%x\n\r",((unsigned short *)CmdStr)[0]);
			}
			else if(((unsigned short *)CmdStr)[0]==0x425b)//down key
			{
#if USE_THREAD_MODE == 0
				if(HistCon>1)
				{
					for(i=0; i<Idxx; i++)
					{
						USART_SendData(USART1,'\b');//回显
						while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
						USART_SendData(USART1,' ');//回显
						while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
						USART_SendData(USART1,'\b');//回显
						while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
					}
					Idxx = strlen(gUartHist[HistCon-2].gUartHist);
					memset(gUartBufIT, '\0', UART_BUF_LEN);
					memcpy(gUartBufIT, gUartHist[HistCon-2].gUartHist, Idxx);
					printf("%s",gUartBufIT);
					if(HistCon>1)
						HistCon--;
				}
#endif
			}
			else if(((unsigned short *)CmdStr)[0]==0x415b)//up key
			{
#if USE_THREAD_MODE == 0
				if(gUartHist[HistCon].Valid == TRUE)
				{
					for(i=0; i<Idxx; i++)
					{
						USART_SendData(USART1,'\b');//回显
						while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
						USART_SendData(USART1,' ');//回显
						while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
						USART_SendData(USART1,'\b');//回显
						while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
					}
					Idxx = strlen(gUartHist[HistCon].gUartHist);
					memset(gUartBufIT, '\0', UART_BUF_LEN);
					memcpy(gUartBufIT, gUartHist[HistCon].gUartHist, Idxx);
					printf("%s",gUartBufIT);
					if(HistCon<9)
						HistCon++;
				}
#endif
			}
			else
				printf("CtrlCode:%x\n\r",((unsigned short *)CmdStr)[0]);
			return 0;
		}
		if( Q_Sh_LexicalAnalysis(CmdStr,&Q_Sh_LexResu) == 0 )//词法分析
			return 0;
		if( Q_Sh_ParseAnalysis(&Q_Sh_LexResu) == 0 )//语法分析
			return 0;	 
		return 1;
	}
	else
	{
		if(IfWaitInput==0)
		{
			printf("Q_Shell Password:\r\n");
			IfWaitInput=1;
		}
		else
		{
			if(strcmp(CmdStr,Q_ShellPassWord)==0)
			{
				IfPass=1;
				printf("Password Right!\r\n\r\n");
			}
			else
			{
				printf("Password Wrong!\r\n\r\n");
				printf("Q_Shell Password:\r\n");
			}
		}
		return 0;
	}
}

/* 
功能：shell接收到TAB按键自动补齐功能
入参：name 从串口得到的命令字符串 bp 接收到的字符串个数的指针
返回：无
 */
void Q_Sh_TabComplete(char *name, unsigned char *bp)
{
	int n, m;
	const char *fm = NULL;
	QSH_RECORD* Index;

	printf("\r\n");

	/* show matching commands */
	for (Index = (QSH_RECORD*) &qShellFunTab$$Base; Index < (QSH_RECORD*) &qShellFunTab$$Limit; Index ++)
	{
		if (!strncmp(name, Index->name, *bp)) {
			m++;
		//	printf("m is %d, fm is %s\r\n", m, fm);
			if (m == 1)
				fm = Index->name;
			else if (m == 2)
				printf("%s %s ", fm, Index->name);
			else
				printf("%s ", Index->name);
		}
		n++;
	}

	/* there's only one match, so complete the line */
	if (m == 1 && fm) {
		n = strlen(fm) - *bp;
		if (*bp + n < UART_BUF_LEN) {
			memcpy(name + *bp, fm + *bp, n);
			*bp += n;
			name[(*bp)++] = '(';
			name[*bp] = '\0';
		}
	}

	/* just redraw input line */
	printf("\r\n%s%s", SHELL_HEAD, name);
}
#if USE_THREAD_MODE

/* 
功能：shell在线程中的处理接收字符
入参：name 从串口得到的命令字符串 bp 接收到的字符串个数的指针
返回：无
 */
void Q_Sh_GetInput(void)
{
	static char gUartBuf[UART_BUF_LEN];//串口缓冲
	static u8 Idx=0;
	static u8 EscFlag=0;//输入完成标志
	static char EscBuf[4];//控制支付存储地
	char Resieve;
	
	if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
	{
		Resieve=USART_ReceiveData(USART1);
		if(EscFlag)//溢出了就省略2个字节
		{
			EscBuf[EscFlag-1]=Resieve;//存入字符
			if(EscFlag==2)//控制字符
			{
				EscBuf[2]=0;
				USART_SendData(USART1,'\n');//回显
				while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
				USART_SendData(USART1,'\r');//回显
				while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
				Q_Sh_CmdHandler(1,EscBuf);
				EscFlag=0;
			}
			else	EscFlag++;//正常字符
		}
		else
		{
			if(Resieve==13)//回车
			{
				gUartBuf[Idx]=0;
				USART_SendData(USART1,'\n');//回显
				while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
				USART_SendData(USART1,'\r');//回显
				while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
				Q_Sh_CmdHandler(0,gUartBuf);
				memset(gUartBuf, '\0', UART_BUF_LEN);
				printf(SHELL_HEAD);
				Idx=0;
			}
			else if(Resieve==0x08)//回删
			{
				if(Idx>0)
				{
					gUartBuf[--Idx]=0;
					USART_SendData(USART1,'\b');//回显
					while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
					USART_SendData(USART1,' ');//回显
					while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
					USART_SendData(USART1,'\b');//回显
					while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
				}
			}
			else if(Resieve=='\t')//Tab
			{
				QSH_RECORD* Index;

				
				if(Idx == 0)
				{
					printf("\r\n FunName \t\t FunDesction \r\n");
					for (Index = (QSH_RECORD*) &qShellFunTab$$Base; Index < (QSH_RECORD*) &qShellFunTab$$Limit; Index++)
					{
						if(strlen(Index->name) < 6)
							printf(" %s \t\t\t %s \r\n", Index->name, Index->desc);
						else if( (strlen(Index->name) >= 6) && (strlen(Index->name) < 14) )
							printf(" %s \t\t %s \r\n", Index->name, Index->desc);
						else
							printf(" %s \t %s \r\n", Index->name, Index->desc);
					}
					printf("\r\n%s", SHELL_HEAD);
				}
				if(Idx>0)
				{
					Q_Sh_TabComplete(gUartBuf, &Idx);
				}
			}
			else if(Resieve>=0x20)
			{
				gUartBuf[Idx++]=Resieve;
				USART_SendData(USART1,Resieve);//回显
				while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);//等待发送结束
			}
			else if(Resieve==0x1b)//溢出了
			{
				EscFlag=1;
			}

			if(Idx>=UART_BUF_LEN) Idx--;//超出范围，就停止加入新的了。
		}

	}

}

#endif
#else
/*
当USE_Q_SHELL_FUNCTION=0即shell功能被关闭后，为了不需要修改原程序而加入
 */
unsigned int Q_Sh_CmdHandler(unsigned int IfCtrl,char *CmdStr)
{
	return 0;
}

#endif
