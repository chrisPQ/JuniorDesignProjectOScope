# JuniorDesignProjectOScope
Teensy program to run oscilloscope and output to screen

Video Overview: https://www.youtube.com/watch?v=Kpc6_z2Lm2U&ab_channel=MarshallG

Showcase Page: https://eecs.engineering.oregonstate.edu/project-showcase/projects/?id=eiNZnlOKVvULkvkf

Design Problem:
The design problem was to create a 2-channel oscilloscope. The scope needed to have voltage
scaling, time scaling, and trigger capabilities. The system also needs to have AC/DC coupling
modes, and a responsive interface.

Design process: The first step in our design process was to do some preliminary research to
see what it was we would need to accomplish the requirements. This included taking into
account the different choices of hardware we could use. We initially considered this project
based on a field programmable gate array due to the high sampling requirement that were
initially on the project instructions. After that requirement was removed, we went with our
second option, which was a Teensy 4.0 microcontroller board. We went with this option because
it is relatively inexpensive (especially compared to an FPGA), but it is still very fast with a clock
speed of 600 megahertz.

Our next big hurdle was figuring out how to implement the input circuitry. Since we decided our
input signal range was 0-20 volts peak to peak, and our microcontroller could only handle a
range of 0-3.3 volts, we had to adjust the signal to be able to measure it. We worked together on
designing the circuit and simulating the results in LTspice. We then soldered it to a protoboard
and experimented with the circuit in the lab. Our team had to work together to troubleshoot the
circuit and adjust the component values to perform as expected.

Although our whole system was split into individual blocks, we had to work together very closely
to ensure our systems would work as expected. For example, the input conditioning block has to
take voltage from the power supply block. Then the signal conversion and processing block had
to take input from the signal conditioning block.Communication about these individual tasks
made our system integration go smoother as the different pieces meshed together well.

Once all the circuit prototypes were designed and tested, we put them onto a PCB. We worked
together to peer review the PCB to ensure it would work correctly. Once the board arrived, we
used tools in the lab such as the oven and soldering irons to assemble the components onto the
board. This phase was a challenge as we included some very small surface mount parts in our
design. One challenge we had at this stage was that we had some slight mistakes in the PCB.
The pin header where the microcontroller was supposed to be ended up being one pin too
short. We fixed this by making an adapter board that mapped the pins of the MCU (expected
one two that we didn’t need) to a set of pin headers that was the correct number for the footprint
on the PCB.

We finished the process by working on our embedded software to control the device. This was a
challenge because it included meshing together code covering many different tasks, all of which
had challenges of their own.

Timeline:
April 19th: Engineering requirments devised.
April 26th: First block documentation.
May 1st: First Block verification.
May 10th: Second Block documentation
May 15th: Second block verification.
May 31st: System Verification.
June 5th: Demo of final project.

Takeaways:
Over the course of this project we learned about working on a complex project with a tight
deadline. This taught us many lessons of project management and how to focus on prioritizing
the important parts when it comes to developing electronics projects. Specifically we learned the
importance of researching and simulation methods before diving into them. While this may
seem restraining at first, it is a method that saves time and money in the long run because
things are more likely to work the first time around. We also learned many things about specific
technologies. Using direct memory access and interrupts to enhance the performance speed of
our microcontroller were concepts that required us to research and access data that wasn’t as
accessible as things we had worked on in the past.
