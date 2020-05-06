<CsoundSynthesizer>
; From http://www.csounds.com/toots/#toot12
<CsOptions>
-odac    ;;;realtime audio out
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

                    instr 12
          iseed     =         p8/10
          iamp      =         ampdb(p4/4000)
          kdirect   =         p5
          imeth     =         p6
          ilforate  =         p7                            ; rate for lfo and random index
          itab      =         2
          itabsize  =         8

if (imeth == 1)     igoto     direct
if (imeth == 2)     kgoto     lfo
if (imeth == 3)     kgoto     random

direct:   kpitch    table     kdirect, itab                 ; index f2 via p5
                    kgoto     contin

lfo:      kindex    phasor    ilforate
          kpitch    table     kindex * itabsize, itab
                    kgoto     contin

random:   kindex    randh     int(7), ilforate, iseed
          kpitch    table     abs(kindex), itab

;contin:   kamp      linseg    0, p3 * .001, iamp, p3 * 0.999, 0  ; amp envelope
;contin:   kamp      expseg    0, p3 * .5, iamp, p3 * 0.5, 0  ; amp envelope
;contin:   kamp     expsegr	0.1, .05, 1, .05, .0
contin:   kamp      linseg    0, p3 * .01, iamp, p3 * 0.99, 0  ; amp envelope
          asig      oscil     kamp, cpspch(kpitch), 1       ; audio oscillator
                    out       asig

endin

</CsInstruments>
<CsScore>

f1   0    4096 10 1                                         ; Sine
f2   0    8    -2 8.00 8.02 8.04 8.05 8.07 8.09 8.11 9.00   ; cpspch C major scale
f3   0    8    -2 9.00 8.11 8.09 8.07 7.05 8.04 7.02 8.0    ; C major scale reversed

/*
; method 1 - direct index of table values
; ins     strt dur  amp  index     method    lforate   rndseed
  i12     0    .5   86   7         1         0         0
  i12     .5   .5   86   6         1         0
  i12     +    .5   86   5         1         0
  i12     +    .5   86   4         1         0
  i12     +    .5   86   3         1         0
  i12     +    .5   86   2         1         0
  i12     +    .5   86   1         1         0
  i12     +    .5   86   0         1         0
  i12     +    .5   86   0         1         0
  i12     +    .5   86   2         1         0
  i12     +    .5   86   3         1         0
  i12  +       .5   86   4         1         0
  i12  +       .5   86   5         1         0
  i12  +       .5   86   6         1         0
  i12  +       2.5  86   7         1         0
  f0      10
s
*/

; NOTE use lforate to tweak number of rounds in progression


; method 2 - lfo index of table values
 ins     strt dur  amp  index     method    lforate   rndseed
 i12     0    1    82   0         2         1         0
 i12     3    1    82   0         2         2         0
 i12     6    1    82   2         2         4
 i12     9    2    78   0         2         8
 ;i12     12   2    94   0         2         16
  f0      16
;

s
; method 3 - random index of table values
; ins     strt dur  amp  index     method    lforate   rndseed
;  i12     0    2    86   0         3         2         .1

  i12     0    2    44   1         2         2          0
  i12     2    2    44   0         3         4          1

  i12     4   2    44   2         2         4          0
  i12     6   2    44   0         3         8          2

 ;i12    20    2    44   1         2         4          0
 ;i12    22    4    44   0         2         0.5        0
 ;i12    26    8    44   0         3         2          3

/*
  i12     6    2    86   0         3         4         .3
  i12     9    2    86   0         3         7         .4
  i12     12   2    86   0         3         11        .5
  i12     15   2    86   0         3         18        .6
  i12     18   2    86   0         3         29        .7
  i12     21   2    86   0         3         47        .8
  i12     24   2    86   0         3         76        .9
  i12     27   2    86   0         3         123       .9
  i12     30   5    86   0         3         199       .
*/
</CsScore>
</CsoundSynthesizer>