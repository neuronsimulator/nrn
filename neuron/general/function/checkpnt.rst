.. _checkpnt:

checkpnt
--------



.. function:: checkpoint


    Syntax:
        :code:`checkpoint("filename")`


    Description:
        saves the current state of the system in a portable file to 
        allow one to take up where you left off --- possibly on another 
        machine. Returning to this state is accomplished by running the 
        program with the checkpoint file as the first argument. 
        If the checkpoint file is inconsistent with the executable the 
        program prints an error message and exits. 
         
        At this time many portions of the computer state are left out of the 
        checkpoint file. ie it is not as complete as a core dump. 
        Some things that ARE included are: 
        All interpreter symbols with definitions and values, 
        all hoc instructions, 
        all neuron state/parameters with mechanisms. 
        Many aspects of the GUI are not included. 
         
        there is not enough implementation at this time to make this 
        facility useful. 

