#ifndef APPLICATIONS_GY30_H_
#define APPLICATIONS_GY30_H_

/* I2C address */
#define GY30_I2C_ADDRESS    0x23 /* ADDR=L */
//#define GY30_I2C_ADDRESS    0x5C /* ADDR=H */
//#define GY30_I2C_BUS_NAME  "i2c2"

/* operation command */
#define GY30_Continuously_H_mode        0x10 /* Continuously H-resolution mode : 1 lx*/
#define GY30_Continuously_H_mode2       0x11 /* Continuously H-resolution mode2 : 0.5 lx*/
#define GY30_Continuously_L_mode        0x13 /* Continuously L-resolution mode : 4 lx*/
#define GY30_One_Time_H_mode            0x20 /* One time H-resolution mode */
#define GY30_One_Time_H_mode2           0x21 /* One time H-resolution mode2 */
#define GY30_One_Time_L_mode            0x23 /* One time L-resolution mode */
#define GY30_Power_Down                 0x00 /* No active state */
#define GY30_Power_On                   0x01 /* Waiting for measurement command */
#define GY30_Reset                      0x07 /* Reset Data register value */

extern uint8_t GY30_Init(const char *name);
extern uint16_t GY30_Measurement(void);

#endif /* APPLICATIONS_GY30_H_ */
