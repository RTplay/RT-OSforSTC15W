
#ifndef	__EEPROM_H
#define	__EEPROM_H

#include	"config.h"

//	ѡ��MCU�ͺ�
#define	MCU_Type	STC15F_L2K08S2	//STC15F_L2K08S2, STC15F_L2K16S2, STC15F_L2K24S2, STC15F_L2K32S2, STC15F_L2K40S2, STC15F_L2K48S2, STC15F_L2K56S2, STC15F_L2K60S2, IAP15F_L2K61S2


/************************** ISP/IAP *****************************
 IAPϵ�� ������Ӧ�ó����޸�Ӧ�ó���

STC15F/L2KxxS2	�������䣬512�ֽ�/��������0x0000��ʼ��

     �ͺ�        ��С   ������  ��ʼ��ַ  ������ַ   MOVC��ƫ�Ƶ�ַ
STC15F/L2K08S2   53K   106����  0x0000  ~  0xD3FF        0x2000
STC15F/L2K16S2   45K    90����  0x0000  ~  0xB3FF        0x4000
STC15F/L2K24S2   37K    74����  0x0000  ~  0x93FF        0x6000
STC15F/L2K32S2   29K    58����  0x0000  ~  0x73FF        0x8000
STC15F/L2K40S2   21K    42����  0x0000  ~  0x53FF        0xA000
STC15F/L2K48S2   13K    26����  0x0000  ~  0x33FF        0xC000
STC15F/L2K56S2   5K     10����  0x0000  ~  0x13FF        0xE000
STC15F/L2K60S2   1K      2����  0x0000  ~  0x03FF        0xF000

STC15F/L2K61S2   ��EPROM, ����122������FLASH�����Բ�д ��ַ 0x0000~0xF3ff.

*/

#if   (MCU_Type == STC15F_L2K08S2)
      #define   MOVC_ShiftAddress    0x2000
#elif (MCU_Type == STC15F_L2K16S2)
      #define   MOVC_ShiftAddress    0x4000
#elif (MCU_Type == STC15F_L2K24S2
      #define   MOVC_ShiftAddress    0x6000
#elif (MCU_Type == STC15F_L2K32S2
      #define   MOVC_ShiftAddress    0x8000
#elif (MCU_Type == STC15F_L2K40S2
      #define   MOVC_ShiftAddress    0xA000
#elif (MCU_Type == STC15F_L2K48S2
      #define   MOVC_ShiftAddress    0xC000
#elif (MCU_Type == STC15F_L2K56S2
      #define   MOVC_ShiftAddress    0xE000
#elif (MCU_Type == STC15F_L2K60S2
      #define   MOVC_ShiftAddress    0xF000
#elif (MCU_Type == IAP15F_L2K61S2
      #define   MOVC_ShiftAddress    0x0000
#endif


void	DisableEEPROM(void);
void 	EEPROM_read_n(u16 EE_address,u8 *DataAddress,u16 number);
void 	EEPROM_write_n(u16 EE_address,u8 *DataAddress,u16 number);
void	EEPROM_SectorErase(u16 EE_address);


#endif