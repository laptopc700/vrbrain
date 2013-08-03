/*
 * UARTDriver.cpp --- AP_HAL_VRBRAIN UART driver.
 *
 * Copyright (C) 2013, Virtualrobotix.com Roberto Navoni , Emile 
 * All Rights Reserved.
 *
 * This software is released under the "BSD3" license.  Read the file
 * "LICENSE" for more information.
 */

#include <AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_VRBRAIN
#include "UARTDriver.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <usb.h>
#include <usart.h>
#include <gpio_hal.h>

//static usb_attr_t usb_attr;

using namespace VRBRAIN;

extern const AP_HAL::HAL& hal;

//definisco qui i parametri per le varie seriali preconfigurate

VRBRAINUARTDriver::VRBRAINUARTDriver(struct usart_dev *usart, uint8_t use_usb):
    _usart_device(usart),
    _usb(use_usb),
    _usb_present(0),
    _initialized(false)
{
}

void VRBRAINUARTDriver::begin(uint32_t baud) {

    if(_usb == 1)
	_usb_present = gpio_read_bit(_GPIOD,4);
    else
	_usb_present = 0;

    if(_usb_present == 1)
    {
	/*
	usb_open();
	usb_default_attr(&usb_attr);
	usb_attr.preempt_prio = 0;
	usb_attr.sub_prio = 0;
	usb_attr.use_present_pin = 1;
	usb_attr.present_port = _GPIOD;
	usb_attr.present_pin = 4;
	usb_ioctl(I_USB_SETATTR, &usb_attr);
	*/
    }
    else
    {
	const stm32_pin_info *txi = &PIN_MAP[_usart_device->tx_pin];
	const stm32_pin_info *rxi = &PIN_MAP[_usart_device->rx_pin];

	gpio_set_af_mode(txi->gpio_device, txi->gpio_bit, _usart_device->gpio_af);
	gpio_set_mode(txi->gpio_device, txi->gpio_bit, GPIO_AF_OUTPUT_PP);
	gpio_set_af_mode(rxi->gpio_device, rxi->gpio_bit, _usart_device->gpio_af);
	gpio_set_mode(rxi->gpio_device, rxi->gpio_bit, GPIO_AF_OUTPUT_PP);

	usart_init(_usart_device);
	usart_setup(_usart_device, (uint32)baud, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, USART_Mode_Rx | USART_Mode_Tx, USART_HardwareFlowControl_None, DEFAULT_TX_TIMEOUT);
	usart_enable(_usart_device);
    }
    _initialized = true;
}

void VRBRAINUARTDriver::begin(uint32_t baud, uint16_t rxS, uint16_t txS) {
    begin(baud);
}

void VRBRAINUARTDriver::end() {
    if(_usb_present == 1)
	usb_close();
    else
	usart_disable(_usart_device);
}

void VRBRAINUARTDriver::flush() {
    if(_usb_present ==1){
	usb_reset_rx();
	usb_reset_tx();
    }else {
	usart_reset_rx(_usart_device);
	usart_reset_tx(_usart_device);
    }

}

void VRBRAINUARTDriver::set_blocking_writes(bool blocking) {
    if(_usb_present == 0){
	usart_reset_tx(_usart_device);
	_usart_device->usetxrb = !blocking;
    }
}

bool VRBRAINUARTDriver::tx_pending() {
	return false;
}

/* VRBRAIN implementations of BetterStream virtual methods */
void VRBRAINUARTDriver::print_P(const prog_char_t *pstr) {

    print(pstr);

}
void VRBRAINUARTDriver::println_P(const prog_char_t *pstr) {

    println(pstr);

}
void VRBRAINUARTDriver::printf(const char *fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    _vprintf(fmt, ap);
    va_end(ap);

}
void VRBRAINUARTDriver::_printf_P(const prog_char *fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    _vprintf(fmt, ap);
    va_end(ap);

}

void VRBRAINUARTDriver::vprintf(const char *pstr, va_list ap) {

    _vprintf(pstr, ap);

}

void VRBRAINUARTDriver::vprintf_P(const prog_char *fmt, va_list ap) {

    _vprintf(fmt, ap);

}


void VRBRAINUARTDriver::_internal_vprintf(const char *fmt, va_list ap)
{
    if (hal.scheduler->in_timerprocess()) {
        // not allowed from timers
        return;
    }

    char buf[1024];  //destination buffer

    int n = vsprintf(buf, fmt, ap);

    if(n > 0){
        for (int i = 0; i < n; i++)
        {
            write(buf[i]);
        }
    }
}

void VRBRAINUARTDriver::_vprintf(const char *fmt, va_list ap)
{
    if (hal.scheduler->in_timerprocess()) {
        // not allowed from timers
        return;
    }
    // we don't use vdprintf() as it goes directly to the file descriptor
    if (strstr(fmt, "%S")) {
	char *fmt2 = strdup(fmt);
	if (fmt2 != NULL) {
	    for (uint16_t i=0; fmt2[i]; i++) {
		if (fmt2[i] == '%' && fmt2[i+1] == 'S') {
		    fmt2[i+1] = 's';
			}
	    }
            _internal_vprintf(fmt2, ap);
	    free(fmt2);
	}
    } else {
	_internal_vprintf(fmt, ap);
	}
}




/* VRBRAIN implementations of Stream virtual methods */
int16_t VRBRAINUARTDriver::available() {
    if(_usb_present == 1)
	return usb_data_available();
    else
    return usart_data_available(_usart_device);
}

int16_t VRBRAINUARTDriver::txspace() {
    if(_usb_present == 1)
	return 255;
    else
	return usart_txfifo_freebytes(_usart_device);
}

int16_t VRBRAINUARTDriver::read() {
    if(_usb_present == 1){
	if (usb_data_available() <= 0)
	    return (-1);
	return usb_getc();
    } else {
	if (available() <= 0)
	    return (-1);
	return usart_getc(_usart_device);
    }
}

/* VRBRAIN implementations of Print virtual methods */
size_t VRBRAINUARTDriver::write(uint8_t c) {

    if (hal.scheduler->in_timerprocess()) {
        // not allowed from timers
        return 0;
    }

    if(_usb_present == 1){
	usb_putc(c);
	return 1;
    }
    else{
	usart_putc(_usart_device, c);
	return 1;
    }
}

#endif // CONFIG_HAL_BOARD
