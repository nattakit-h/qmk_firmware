// Copyright 2024 yangzheng20003 (@yangzheng20003)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/* System */
#define MG_USB_POWER_PIN                    B1
#define MG_USB_INSERT_PIN                   A7
#define MG_LED_POWER_PIN                    A5
#define MG_LED_BOOST_PIN                    D2
#define MG_BAT_FULL_PIN                     A15
#define MG_USE_BAT_PIN                      C15
#define MG_USE_MAC_PIN                      C14

/* Wireless Info */
#define MD_BT1_NAME                         "M1 V5"
#define MD_BT2_NAME                         "M1 V5"
#define MD_BT3_NAME                         "M1 V5"
#define MD_DONGLE_PRODUCT                   "M1 V5"

/* UART */
#define UART_DRIVER                         SD3
#define UART_TX_PIN                         C10
#define UART_RX_PIN                         C11
#define UART_RX_PAL_MODE                    7

/* Encoder */
#define ENCODER_MAP_KEY_DELAY               1
#define ENCODER_B_PIN                       B6

/* SPI */
#define SPI_DRIVER                          SPIDQ
#define SPI_SCK_PIN                         B3
#define SPI_MOSI_PIN                        B5
#define SPI_MISO_PIN                        B4

/* Flash */
#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN C12
#define WEAR_LEVELING_LOGICAL_SIZE          (WEAR_LEVELING_BACKING_SIZE / 2)

/* WS2812 */
#define WS2812_SPI_DRIVER                   SPIDM2
#define WS2812_SPI_DIVISOR                  32
