<CsoundSynthesizer>
<CsOptions>
-odac    ;;;realtime audio out
-m0 ;; uncomment to disable warnings
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 5

instr 1; C

kcps = 261.63
icps = 261.63

kenv linen 1, .01, p3, 1
asig pluck kenv*0.7, kcps, icps, 0, 6, 0, 0
     outs asig, asig
endin

instr 2 ; E

kcps = 329.63
icps = 329.63

kenv linen 1, .01, p3, 1
asig pluck kenv*0.7, kcps, icps, 0, 6, 0, 0
     outs asig, asig
endin

instr 3 ; G

kcps = 391.995
icps = 391.995

kenv linen 1, .01, p3, 1
asig pluck kenv*0.7, kcps, icps, 0, 6, 0, 0
     outs asig, asig
endin



instr 4 ; D

kcps = 293.66
icps = 293.66
ifn  = 0

kenv linen 1, .01, p3, 1
asig pluck kenv*0.7, kcps, icps, 0, 6, 0, 0
     outs asig, asig
endin

instr 5 ; lowC

kcps = 130.81
icps = 130.81
ifn  = 0

kenv linen 1, .01, p3, 1
asig pluck kenv*0.7, kcps, icps, 0, 6, 0, 0
     outs asig, asig
endin

instr 9
          ilinb     =         44
          krnd     random 1, 100
          printk 0.5, krnd/100
          iamp      =         1;ampdb(p4/10000000)
          imeth     =         p4
          ilforate  =         p5                            ; rate for lfo and random index
          itab      =         2
          itabsize  =         8

if (imeth == 1)     kgoto     downlfo
if (imeth == 2)     kgoto     lfo
if (imeth == 3)     kgoto     random

lfo:      kindex    phasor    ilforate
          kpitch    table     kindex * itabsize, itab
                    kgoto     contin

downlfo:  kindex    phasor    ilforate
          kpitch    table     kindex * itabsize,  3
                    kgoto     contin

seed 42
random:   kindex    randh     int(7), ilforate, 42
          kpitch    table     abs(kindex), itab

contin:   ;kamp      linseg    0, ilinb * 0.1, iamp, ilinb * 0.90, 0  ; amp envelope
          ;kamp      linen    0, p3 * 0.1, p3, p3 * 0.90  ; amp envelope

          kenv linen 1.0, p3*.1, p3, p3*0.1
          asig      oscil     kenv, cpspch(kpitch), 1       ; audio oscillator
                    out       asig, asig
endin


</CsInstruments>
<CsScore>
f0 3600 ; play for 1hr

f1 0 16384 10 1
f2   0    8    -2 8.00 8.02 8.04 8.05 8.07 8.09 8.11 9.00   ; cpspch C major scale
f3   0    8    -2 9.00 8.11 8.09 8.07 8.05 8.04 8.02 8.0    ; C major scale reversed

;instr start stop
;i 1 0 6
;i 2 1 6
;i 3 2 6
;i 4 3 6
;i 5 4 6
;i 1 5 6

;instr start stop p4;method p5:lforate
i 9 0 0.5 2 2
;i 9 1 . 1 2
;i 9 + . 3 4
;i 9 + . 3 4
;i 9 + . 3 4
;i 9 + . 3 4
;i 9 + . 3 4
</CsScore>
</CsoundSynthesizer>