/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2000
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for SERVO */
#define SERVO_INST                                                         TIMG7
#define SERVO_INST_IRQHandler                                   TIMG7_IRQHandler
#define SERVO_INST_INT_IRQN                                     (TIMG7_INT_IRQn)
#define SERVO_INST_CLK_FREQ                                               100000
/* GPIO defines for channel 1 */
#define GPIO_SERVO_C1_PORT                                                 GPIOA
#define GPIO_SERVO_C1_PIN                                         DL_GPIO_PIN_27
#define GPIO_SERVO_C1_IOMUX                                      (IOMUX_PINCM60)
#define GPIO_SERVO_C1_IOMUX_FUNC                     IOMUX_PINCM60_PF_TIMG7_CCP1
#define GPIO_SERVO_C1_IDX                                    DL_TIMER_CC_1_INDEX




/* Defines for OLED */
#define OLED_INST                                                           I2C1
#define OLED_INST_IRQHandler                                     I2C1_IRQHandler
#define OLED_INST_INT_IRQN                                         I2C1_INT_IRQn
#define OLED_BUS_SPEED_HZ                                                 100000
#define GPIO_OLED_SDA_PORT                                                 GPIOB
#define GPIO_OLED_SDA_PIN                                          DL_GPIO_PIN_3
#define GPIO_OLED_IOMUX_SDA                                      (IOMUX_PINCM16)
#define GPIO_OLED_IOMUX_SDA_FUNC                       IOMUX_PINCM16_PF_I2C1_SDA
#define GPIO_OLED_SCL_PORT                                                 GPIOB
#define GPIO_OLED_SCL_PIN                                          DL_GPIO_PIN_2
#define GPIO_OLED_IOMUX_SCL                                      (IOMUX_PINCM15)
#define GPIO_OLED_IOMUX_SCL_FUNC                       IOMUX_PINCM15_PF_I2C1_SCL


/* Defines for PRINT */
#define PRINT_INST                                                         UART0
#define PRINT_INST_FREQUENCY                                            40000000
#define PRINT_INST_IRQHandler                                   UART0_IRQHandler
#define PRINT_INST_INT_IRQN                                       UART0_INT_IRQn
#define GPIO_PRINT_RX_PORT                                                 GPIOA
#define GPIO_PRINT_TX_PORT                                                 GPIOA
#define GPIO_PRINT_RX_PIN                                         DL_GPIO_PIN_31
#define GPIO_PRINT_TX_PIN                                         DL_GPIO_PIN_28
#define GPIO_PRINT_IOMUX_RX                                       (IOMUX_PINCM6)
#define GPIO_PRINT_IOMUX_TX                                       (IOMUX_PINCM3)
#define GPIO_PRINT_IOMUX_RX_FUNC                        IOMUX_PINCM6_PF_UART0_RX
#define GPIO_PRINT_IOMUX_TX_FUNC                        IOMUX_PINCM3_PF_UART0_TX
#define PRINT_BAUD_RATE                                                 (115200)
#define PRINT_IBRD_40_MHZ_115200_BAUD                                       (21)
#define PRINT_FBRD_40_MHZ_115200_BAUD                                       (45)





/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOB)

/* Defines for LED_0: GPIOB.22 with pinCMx 50 on package pin 21 */
#define LED_LED_0_PIN                                           (DL_GPIO_PIN_22)
#define LED_LED_0_IOMUX                                          (IOMUX_PINCM50)
/* Port definition for Pin Group KEY */
#define KEY_PORT                                                         (GPIOB)

/* Defines for KEY9: GPIOB.6 with pinCMx 23 on package pin 58 */
// pins affected by this interrupt request:["KEY9","KEY10"]
#define KEY_INT_IRQN                                            (GPIOB_INT_IRQn)
#define KEY_INT_IIDX                            (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define KEY_KEY9_IIDX                                        (DL_GPIO_IIDX_DIO6)
#define KEY_KEY9_PIN                                             (DL_GPIO_PIN_6)
#define KEY_KEY9_IOMUX                                           (IOMUX_PINCM23)
/* Defines for KEY10: GPIOB.7 with pinCMx 24 on package pin 59 */
#define KEY_KEY10_IIDX                                       (DL_GPIO_IIDX_DIO7)
#define KEY_KEY10_PIN                                            (DL_GPIO_PIN_7)
#define KEY_KEY10_IOMUX                                          (IOMUX_PINCM24)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_SERVO_init(void);
void SYSCFG_DL_OLED_init(void);
void SYSCFG_DL_PRINT_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
