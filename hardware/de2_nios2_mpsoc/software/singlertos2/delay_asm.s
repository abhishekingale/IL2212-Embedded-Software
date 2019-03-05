
        .equ    delaycount,     12000
#        .equ    delaycount,     170
        .text                   # Instructions follow
        .global delay1           # Makes "main" globally known

delay1:  beq     r4,r0,fin       # exit outer loop

        movi    r8,delaycount   # delay estimation for 1ms

inner:  beq     r8,r0,outer     # exit from inner loop

        subi    r8,r8,1         # decrement inner counter
        
        br      inner
        
outer:  subi    r4,r4,1         # decrement outer counter
        br      delay1


fin:    ret

