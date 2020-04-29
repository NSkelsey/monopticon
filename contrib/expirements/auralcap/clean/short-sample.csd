/* Getting started.. 1.1 Hello World

The 'Basics Chapter' of the Tutorials, will explain the root functionality of CsoundQt.
You will find the descriptions as comments directly in the code. 

This first example is focused on the different comment-types and shows a simple program, which outputs a "Hello World - 440Hz beep" to the computer's audio output and the "Hello World" string to the console.

To start it, press the RUN Button in the CsoundQt-toolbar, or choose "Control->Run Csound" from the menu. 
*/

<CsoundSynthesizer>
<CsOptions>
; Uncomment to always use the DAC -odac
</CsOptions>
<CsInstruments>
;ksmps=32
; This is a comment!
; Comments describe how things are done, and help to explain the code.
; They are shown in green in CsoundQt.
; Anything after a semicolon will be ignored by Csound, so you can use it for human readable information.

/*
If you need more space than one line,
it's
no
problem, with those comment signs used here.
*/

instr 123 					; instr starts an instrument block and refers it to a number. In this case, it is 123.
							; You can put comments everywhere, they will not become compiled.
	prints "Hello World!%n" 	     ; 'prints' will print a string to the Csound console.
	aSin	oscils 0dbfs/4, 440, 0 	; the opcode 'oscils' here generates a 440 Hz sinetone signal at -12dB FS
	out aSin				     ; here the signal is assigned to the computer audio output
endin

instr 1
	aSin	poscil3 0dbfs/4, 261.656 ;C
    out aSin
endin
instr 2
	aSin	poscil3 0dbfs/4, 329.628 ;E
    out aSin
endin
instr 3
	aSin	poscil3 0dbfs/4, 391.995 ;G
    out aSin
endin

instr 4
	aSin	poscil3 0dbfs/4, 440 ;A
    out aSin
endin

</CsInstruments>
<CsScore>
f0 120
i1 0.0 0.4
i2 0.2 0.2
i3 0.8 0.2

i4 1.0 0.4
i1 1.2 0.2
i2 1.4 0.2
e 							; e - ends the score
</CsScore>
</CsoundSynthesizer>