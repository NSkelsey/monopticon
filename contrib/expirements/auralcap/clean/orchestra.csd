<CsoundSynthesizer>
<CsOptions>
-odac    ;;;realtime audio out
;-m0 ;; uncomment to disable warnings
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


instr 9
          seed 42 ; seed from systemtime

          ilinb     =         44
          krnd     random 1, 100
          printk 0.5, krnd/100
          iamp      =         1;ampdb(p4/10000000)
          imeth     =         p4
          ilforate  =         p5                            ; rate for lfo and random index
          itab      =         2
          itabsize  =         8

if (imeth == 2)     kgoto     lfo
if (imeth == 3)     kgoto     random

lfo:      kindex    phasor    ilforate
          kpitch    table     kindex * itabsize, itab
                    kgoto     contin

seed 0
random:   kindex    randh     int(7), ilforate, 42
          kpitch    table     abs(kindex), itab

contin:   kamp      linseg    0, ilinb * .01, iamp, ilinb * 0.99, 0  ; amp envelope
          asig      oscil     kamp-0.2, cpspch(kpitch), 1       ; audio oscillator
                    out       asig, asig
endin


</CsInstruments>
<CsScore>
f0 3600 ; play for 1hr

f1 0 16384 10 1
f2   0    8    -2 8.00 8.02 8.04 8.05 8.07 8.09 8.11 9.00   ; cpspch C major scale

;instr start stop
;i 1 1 4
;i 2 2 4
;i 3 5 4
;i 4 7 4
;i 3 9 4
;i 5 4 4

;instr start stop p4;method p5:lforate
;i 9 0 0.25 3 4
;i 9 0.25 0.5 3 4
;i 9 0.5 0.75 3 4
</CsScore>
</CsoundSynthesizer>