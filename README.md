# CAN-alarm-system
Project regarding networked embedded systems. Development of an alarm system interconnecting Mbed nodes with a Controller Area Network (CAN).

In this project we aimed at making a realistic and energy efficient implementation of an alarm
system. In a relatively closed quarters environment we idealized a controller area network
(CAN), composed of static nodes able to sense their surroundings, and able to talk to a more
central unit that will be from now on referred to as main node, or the master. The master
would work as a simple interface to the user, and would allow the redefinition of
several system settings, and the checking of all the values being measured by the several
(enslaved) nodes spread throughout the rooms.


In this work we go into detail on the five major areas of development. Firstly,
a description of the global architecture of our program is made. Secondly, the energy
management of our project is discussed and the paths that were taken and considered to make this a
”greener” solution are introduced. Afterwards all the sensing and acting components used to
bring our alarm system to life are analyzed, followed by two sections describing the system support and
network communications responsible for binding all of the hardware together into a reliable
technology demonstrator.


A user manual for the use of the system is provided.
All the code utilized in each board is also made available.