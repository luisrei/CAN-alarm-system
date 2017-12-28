/* mbed Microcontroller Library - SPISlave
 * Copyright (c) 2010-2011 ARM Limited. All rights reserved. 
 */

#ifndef MBED_SPISLAVE_H
#define MBED_SPISLAVE_H

#include "device.h"

#if DEVICE_SPISLAVE

#include "platform.h"
#include "PinNames.h"
#include "PeripheralNames.h"
#include "Base.h"

namespace mbed {

/** A SPI slave, used for communicating with a SPI Master device
 *
 *  The default format is set to 8-bits, mode 0, and a clock frequency of 1MHz
 *
 * Example:
 * @code
 * // Reply to a SPI master as slave
 *
 * #include "mbed.h"
 *
 * SPISlave device(p5, p6, p7, p8); // mosi, miso, sclk, ssel
 *
 * int main() {
 *     device.reply(0x00);              // Prime SPI with first reply
 *     while(1) {
 *         if(device.receive()) {
 *             int v = device.read();   // Read byte from master
 *             v = (v + 1) % 0x100;     // Add one to it, modulo 256
 *             device.reply(v);         // Make this the next reply
 *         }
 *     }
 * }
 * @endcode
 */ 
class SPISlave : public Base {

public:

    /** Create a SPI slave connected to the specified pins
     *
     *  Pin Options:
     *  (5, 6, 7i, 8) or (11, 12, 13, 14)
     *
     *  mosi or miso can be specfied as NC if not used
     *
     *  @param mosi SPI Master Out, Slave In pin
     *  @param miso SPI Master In, Slave Out pin
     *  @param sclk SPI Clock pin
     *  @param ssel SPI chip select pin
     *  @param name (optional) A string to identify the object     
     */
    SPISlave(PinName mosi, PinName miso, PinName sclk, PinName ssel,
        const char *name = NULL);

    /** Configure the data transmission format
     *
     *  @param bits Number of bits per SPI frame (4 - 16)
     *  @param mode Clock polarity and phase mode (0 - 3)
     *
     * @code
     * mode | POL PHA 
     * -----+--------     
     *   0  |  0   0 
     *   1  |  0   1
     *   2  |  1   0 
     *   3  |  1   1
     * @endcode
     */
    void format(int bits, int mode = 0);

    /** Set the spi bus clock frequency
     *
     *  @param hz SCLK frequency in hz (default = 1MHz)
     */
    void frequency(int hz = 1000000);

    /** Polls the SPI to see if data has been received
     *
     *  @returns
     *    0 if no data,
     *    1 otherwise
     */
    int receive(void);

    /** Retrieve  data from receive buffer as slave
     *
     *  @returns
     *    the data in the receive buffer
     */
    int read(void);

    /** Fill the transmission buffer with the value to be written out
     *  as slave on the next received message from the master.
     *
     *  @param value the data to be transmitted next
     */
    void reply(int value);

protected:

    SPIName _spi;
    
    int _bits;
    int _mode;
    int _hz;

};

} // namespace mbed

#endif

#endif
