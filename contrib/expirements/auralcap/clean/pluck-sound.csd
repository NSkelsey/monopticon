<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if real audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pluck.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1



instr 1; C

kcps = 261.63
icps = 261.63
ifn  = 0

asig pluck 0.7, kcps, icps, ifn, 6, 0, 0
     outs asig, asig

endin

instr 2 ; E

kcps = 329.63
icps = 329.63
ifn  = 0

asig pluck 0.7, kcps, icps, ifn, 6, 0, 0
     outs asig, asig

endin

instr 3 ; G

kcps = 391.995
icps = 391.995
ifn  = 0

asig pluck 0.7, kcps, icps, ifn, 6, 0, 0
     outs asig, asig

endin

instr 4 ; D

kcps = 293.66
icps = 293.66
ifn  = 0

asig pluck 0.7, kcps, icps, ifn, 6, 0, 0
     outs asig, asig

endin

instr 5 ; lowC

kcps = 130.81
icps = 130.81
ifn  = 0

asig pluck 0.7, kcps, icps, ifn, 6, 0, 0
     outs asig, asig

endin
</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

f0 600
;i 1 0 2

; Play Instrument #1 for two seconds.
;i 1 1 2


;i 3 0 4 6
;i 3 0 10 1 ;yes 
i 1 1 4
i 2 2 4
i 3 5 4
i 4 7 4
i 3 9 4
i 5 4 4

;i 3 3 4 6	;needs 2 extra parameters (iparm1, iparm2)
;i 3 5 4 6
e
</CsScore>
</CsoundSynthesizer>