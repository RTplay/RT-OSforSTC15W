
/*************	����˵��	**************

���ļ�ΪSTC15xxxϵ�е����жϳ�ʼ�����жϳ���,�û�����������ļ����޸��Լ���Ҫ���жϳ���.

******************************************/



#include	"Exti.h"


/********************* INT0�жϺ��� *************************/
void Ext_INT0 (void) interrupt INT0_VECTOR		//���ж�ʱ�Ѿ������־
{
	P00 = ~P00;
}

/********************* INT1�жϺ��� *************************/
void Ext_INT1 (void) interrupt INT1_VECTOR		//���ж�ʱ�Ѿ������־
{
	P01 = ~P01;
}

/********************* INT2�жϺ��� *************************/
void Ext_INT2 (void) interrupt INT2_VECTOR	//
{
	P02 = ~P02;
}

/********************* INT3�жϺ��� *************************/
void Ext_INT3 (void) interrupt INT3_VECTOR
{
	P03 = ~P03;
}

/********************* INT4�жϺ��� *************************/
void Ext_INT4 (void) interrupt INT4_VECTOR
{
	P04 = ~P04;
}

//========================================================================
// ����: u8	Ext_Inilize(u8 EXT, EXTI_InitTypeDef *INTx)
// ����: �ⲿ�жϳ�ʼ������.
// ����: INTx: �ṹ����,��ο�Exti.h��Ķ���.
// ����: �ɹ�����0, �ղ�������1,���󷵻�2.
// �汾: V1.0, 2012-10-22
//========================================================================
u8	Ext_Inilize(u8 EXT, EXTI_InitTypeDef *INTx)
{
	if(EXT >  EXT_INT4)	return 1;	//�ղ���
	
	if(EXT == EXT_INT0)	//���ж�0
	{
		if(INTx->EXTI_Interrupt == ENABLE)		EX0 = 1;	//�����ж�
		else									EX0 = 0;	//��ֹ�ж�
		if(INTx->EXTI_Polity == PolityHigh)		PX0 = 1;	//�����ȼ��ж�
		else									PX0 = 0;	//�����ȼ��ж�
		if(INTx->EXTI_Mode == EXT_MODE_Fall)	IT0 = 1;	//�½����ж�
		else									IT0 = 0;	//����,�½����ж�
		return	0;		//�ɹ�
	}

	if(EXT == EXT_INT1)	//���ж�1
	{
		if(INTx->EXTI_Interrupt == ENABLE)		EX1 = 1;	//�����ж�
		else									EX1 = 0;	//��ֹ�ж�
		if(INTx->EXTI_Polity == PolityHigh)		PX1 = 1;	//�����ȼ��ж�
		else									PX1 = 0;	//�����ȼ��ж�
		if(INTx->EXTI_Mode == EXT_MODE_Fall)	IT1 = 1;	//�½����ж�
		else									IT1 = 0;	//����,�½����ж�
		return	0;		//�ɹ�
	}

	if(EXT == EXT_INT2)		//���ж�2, �̶�Ϊ�½��ص����ȼ��ж�
	{
		if(INTx->EXTI_Interrupt == ENABLE)	INT_CLKO |=  (1 << 4);	//�����ж�	
		else								INT_CLKO &= ~(1 << 4);	//��ֹ�ж�
		return	0;		//�ɹ�
	}

	if(EXT == EXT_INT3)		//���ж�3, �̶�Ϊ�½��ص����ȼ��ж�
	{
		if(INTx->EXTI_Interrupt == ENABLE)	INT_CLKO |=  (1 << 5);	//�����ж�	
		else								INT_CLKO &= ~(1 << 5);	//��ֹ�ж�
		return	0;		//�ɹ�
	}

	if(EXT == EXT_INT4)		//���ж�4, �̶�Ϊ�½��ص����ȼ��ж�
	{
		if(INTx->EXTI_Interrupt == ENABLE)	INT_CLKO |=  (1 << 6);	//�����ж�	
		else								INT_CLKO &= ~(1 << 6);	//��ֹ�ж�
		return	0;		//�ɹ�
	}
	return 2;	//ʧ��
}
