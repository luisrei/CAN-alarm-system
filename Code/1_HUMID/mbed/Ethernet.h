/* mbed Microcontroller Library - Ethernet
 * Copyright (c) 2009-2011 ARM Limited. All rights reserved.
 */ 
 
#ifndef MBED_ETHERNET_H
#define MBED_ETHERNET_H

#include "device.h"

#if DEVICE_ETHERNET

#include "Base.h"

namespace mbed {

/** An ethernet interface, to use with the ethernet pins.
 *
 * Example:
 * @code
 * // Read destination and source from every ethernet packet
 * 
 * #include "mbed.h"
 * 
 * Ethernet eth;
 *  
 * int main() {
 *     char buf[0x600];
 *     
 *     while(1) {
 *         int size = eth.receive();
 *         if(size > 0) {
 *             eth.read(buf, size);
 *             printf("Destination:  %02X:%02X:%02X:%02X:%02X:%02X\n",
 *                     buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
 *             printf("Source: %02X:%02X:%02X:%02X:%02X:%02X\n",
 *                     buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
 *         }
 *         
 *         wait(1);
 *     }
 * }
 * @endcode
 */
class Ethernet : public Base {

public:
    
    /** Initialise the ethernet interface.
     */
    Ethernet();

    /** Powers the hardware down.
     */
    virtual ~Ethernet();

    enum Mode {
        AutoNegotiate
        , HalfDuplex10
        , FullDuplex10
        , HalfDuplex100
        , FullDuplex100
    };

    /** Writes into an outgoing ethernet packet.
     *
     *  It will append size bytes of data to the previously written bytes.
     *  
     *  @param data An array to write.
     *  @param size The size of data.
     *
     *  @returns
     *    The number of written bytes.
     */
    int write(const char *data, int size);

    /** Send an outgoing ethernet packet.
     *
     *  After filling in the data in an ethernet packet it must be send.
     *  Send will provide a new packet to write to.
     *
     *  @returns
     *    0 if the sending was failed,
     *    1 if the package is successfully sent.
     */
    int send();

    /** Recevies an arrived ethernet packet.
     *
     *  Receiving an ethernet packet will drop the last received ethernet packet 
     *  and make a new ethernet packet ready to read.
     *  If no ethernet packet is arrived it will return 0.
     *
     *  @returns
     *    0 if no ethernet packet is arrived,
     *    or the size of the arrived packet.
     */
    int receive();

    /** Read from an recevied ethernet packet.
     *
     *  After receive returnd a number bigger than 0it is
     *  possible to read bytes from this packet.
     *  Read will write up to size bytes into data.
     *
     *  It is possible to use read multible times. 
     *  Each time read will start reading after the last read byte before.
     *
     *  @returns
     *    The number of byte read.
     */
    int read(char *data, int size);
    
    /** Gives the ethernet address of the mbed.
     *
     *  @param mac Must be a pointer to a 6 byte char array to copy the ethernet address in.
     */
    void address(char *mac);

    /** Returns if an ethernet link is pressent or not. It takes a wile after Ethernet initializion to show up.
     * 
     *  @returns
     *   0 if no ethernet link is pressent,
     *   1 if an ethernet link is pressent.
     *
     * Example:
     * @code
     * // Using the Ethernet link function
     * #include "mbed.h"
     * 
     * Ethernet eth;
     * 
     * int main() {
     *     wait(1); // Needed after startup.
     *     if (eth.link()) {
     *          printf("online\n");
     *     } else {
     *          printf("offline\n");
     *     }
     * }
     * @endcode
     */
    int link();

    /** Sets the speed and duplex parameters of an ethernet link
     *
     * - AutoNegotiate      Auto negotiate speed and duplex
     * - HalfDuplex10       10 Mbit, half duplex
     * - FullDuplex10       10 Mbit, full duplex
     * - HalfDuplex100      100 Mbit, half duplex
     * - FullDuplex100      100 Mbit, full duplex
     *
     *  @param mode the speed and duplex mode to set the link to:
     */
    void set_link(Mode mode);

};

} // namespace mbed

#endif

#endif
