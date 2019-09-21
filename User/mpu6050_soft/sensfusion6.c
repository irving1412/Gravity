#include <math.h>
#include "sensfusion6.h"
#include "mymath.h"
//#include "config.h"
//#include "ledseq.h"

/********************************************************************************	 
 * ������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
 * ALIENTEK MiniFly
 * 6�������ںϴ���	
 * ����ԭ��@ALIENTEK
 * ������̳:www.openedv.com
 * ��������:2018/6/22
 * �汾��V1.2
 * ��Ȩ���У�����ؾ���
 * Copyright(C) �������������ӿƼ����޹�˾ 2014-2024
 * All rights reserved
********************************************************************************/

#define ACCZ_SAMPLE		350

float Kp = 0.4f;		/*��������*/
float Ki = 0.001f;		/*��������*/
float exInt = 0.0f;
float eyInt = 0.0f;
float ezInt = 0.0f;		/*��������ۼ�*/

//float Kp = 80.0f;		/*��������*/
//float Ki = 1.0f;		/*��������*/
//float exInt = 0.0f;
//float eyInt = 0.0f;
//float ezInt = 0.0f;

static float q0 = 1.0f;	/*��Ԫ��*/
static float q1 = 0.0f;
static float q2 = 0.0f;
static float q3 = 0.0f;	

static float baseZacc = 1.0;		/*��̬Z����ٶ�*/
bool isGravityCalibrated = false;	/*�Ƿ�УУ׼���*/
static float maxError = 0.f;		/*������*/


static float invSqrt(float x);	/*���ٿ�ƽ����*/


static void calBaseAccZ(float accZ)	/*���㾲̬Z����ٶ�*/
{
	static u16 cnt = 0;
	static float accZMin = 1.5;
	static float accZMax = 0.5;
	static float sumAccZ = 0.f;
	
	sumAccZ += accZ;
	if(accZ < accZMin)	accZMin = accZ;
	if(accZ > accZMax)	accZMax = accZ;
	
	if(++cnt >= ACCZ_SAMPLE) /*��������*/
	{
		cnt = 0;
		maxError = accZMax - accZMin;
		accZMin = 1.5;
		accZMax = 0.5;
		
		if(maxError < 0.015f)
		{
			baseZacc = sumAccZ / ACCZ_SAMPLE;
			
			isGravityCalibrated = true;
			
//			ledseqRun(SYS_LED, seq_calibrated);	/*У׼ͨ��ָʾ��*/
		}
		
		sumAccZ = 0.f;
	}	
}
	
#define DEG2RAD		0.017453293f	/* ��ת���� ��/180 */
#define RAD2DEG		57.29578f		/* ����ת�� 180/�� */
state_t	state;	
void imuUpdate(Axis3f acc, Axis3f gyro, state_t *state , float dt)	/*�����ں� �����˲�*/
{
	float normalise;
	float ex, ey, ez;
	float q0s, q1s, q2s, q3s;	/*��Ԫ����ƽ��*/
	static float R11,R21;		/*����(1,1),(2,1)��*/
	static float vecxZ, vecyZ, veczZ;	/*��������ϵ�µ�Z��������*/
	float halfT = 0.5f * dt;
	Axis3f tempacc = acc;

	gyro.x = gyro.x * DEG2RAD;	/* ��ת���� */
	gyro.y = gyro.y * DEG2RAD;
	gyro.z = gyro.z * DEG2RAD;

	/* ���ٶȼ������Чʱ,���ü��ٶȼƲ���������*/
	if((acc.x != 0.0f) || (acc.y != 0.0f) || (acc.z != 0.0f))
	{
		/*��λ�����ټƲ���ֵ*/
		normalise = invSqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
		acc.x *= normalise;
		acc.y *= normalise;
		acc.z *= normalise;

		/*���ټƶ�ȡ�ķ������������ټƷ���Ĳ�ֵ����������˼���*/
		ex = (acc.y * veczZ - acc.z * vecyZ);
		ey = (acc.z * vecxZ - acc.x * veczZ);
		ez = (acc.x * vecyZ - acc.y * vecxZ);
		
		/*����ۼƣ�����ֳ������*/
		exInt += Ki * ex * dt ;  
		eyInt += Ki * ey * dt ;
		ezInt += Ki * ez * dt ;
		
		/*�ò���������PI����������ƫ�����������ݶ����е�ƫ����*/
		gyro.x += Kp * ex + exInt;
		gyro.y += Kp * ey + eyInt;
		gyro.z += Kp * ez + ezInt;
	}
	/* һ�׽����㷨����Ԫ���˶�ѧ���̵���ɢ����ʽ�ͻ��� */
	float q0Last = q0;
	float q1Last = q1;
	float q2Last = q2;
	float q3Last = q3;
	q0 += (-q1Last * gyro.x - q2Last * gyro.y - q3Last * gyro.z) * halfT;
	q1 += ( q0Last * gyro.x + q2Last * gyro.z - q3Last * gyro.y) * halfT;
	q2 += ( q0Last * gyro.y - q1Last * gyro.z + q3Last * gyro.x) * halfT;
	q3 += ( q0Last * gyro.z + q1Last * gyro.y - q2Last * gyro.x) * halfT;
	
	/*��λ����Ԫ��*/
	normalise = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= normalise;
	q1 *= normalise;
	q2 *= normalise;
	q3 *= normalise;
	/*��Ԫ����ƽ��*/
	q0s = q0 * q0;
	q1s = q1 * q1;
	q2s = q2 * q2;
	q3s = q3 * q3;
	
	R11 = q0s + q1s - q2s - q3s;	/*����(1,1)��*/
	R21 = 2 * (q1 * q2 + q0 * q3);	/*����(2,1)��*/

	/*��������ϵ�µ�Z��������*/
	vecxZ = 2 * (q1 * q3 - q0 * q2);/*����(3,1)��*/
	vecyZ = 2 * (q0 * q1 + q2 * q3);/*����(3,2)��*/
	veczZ = q0s - q1s - q2s + q3s;	/*����(3,3)��*/
	
	if (vecxZ>1) vecxZ=1;
	if (vecxZ<-1) vecxZ=-1;
	
	/*����roll pitch yaw ŷ����*/
	state->attitude.pitch = -asinf(vecxZ) * RAD2DEG; 
	state->attitude.roll = atan2f(vecyZ, veczZ) * RAD2DEG;
	state->attitude.yaw = atan2f(R21, R11) * RAD2DEG;
	
 

	if (!isGravityCalibrated)	/*δУ׼*/
	{		
		float temp = tempacc.x* vecxZ + tempacc.y * vecyZ + tempacc.z * veczZ;		
		calBaseAccZ(temp);					
	}
	else	/*У׼���*/
	{
		state->acc.z= tempacc.x* vecxZ + tempacc.y * vecyZ + tempacc.z * veczZ - baseZacc;	/*Z����ٶ�(ȥ���������ٶ�)*/
		state->acc.z *= 980.f;	/*��λ cm/s/s*/
	}
	
		printf("%.2f\r\n", state->attitude.roll);
}

// Fast inverse square-root
// See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
float invSqrt(float x)	/*���ٿ�ƽ����*/
{
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f3759df - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	return y;
}

bool getIsCalibrated(void)
{
	return isGravityCalibrated;
}
