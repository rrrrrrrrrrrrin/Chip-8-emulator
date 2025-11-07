# CHIP-8 emulator written in C++ via SDL3 library.  


To **compile** a project in MSYS2 MINGW64 (in the project folder Chip-8-emu):  


g++ chip8.cpp main.cpp -o "Rin's Chip-8 Emulator.exe" \  
-I"dlls/SDL3-3.2.22/include" \  
-I"dlls/SDL3_image-3.2.4/include" \  
-L"dlls/SDL3-3.2.22/lib/x64" \  
-L"dlls/SDL3_image-3.2.4/lib/x64" \  
-lSDL3 -lSDL3_image \  
-static \  
-mwindows  


**Download** and unpack Chip-8-Release.zip, run "Rin's Chip-8 Emulator.exe"  


This emulator's instructions are of modern behaviour.  


**Quirks**:    

		   1. VF Reset: on 

           2. Memory: on  
		   
           3. Display wait: off  
		   
           4. Clipping and wrapping: on  
		   
           5. Shifting: off  
		   
           6. Jumping: off  
		   

VF Reset — 8XY1-8XY3 opcodes: flag register VF is set to 0.  

Memory — FX55, FXY65 opcodes: index register I is incremented.  

Display wait — Drawing sprites to the display waits for the vertical blank interrupt,  
               limiting their speed to max 60 sprites per second.  
			   
Clipping — sprites partially drawn on the screen get clipped.  

Wrapping — sprites drawn out of bounds of the screen wrap around it.  

Shifting — 8XY6, 8XYE opcodes: only operate on VX, instead of storing the shifted version of VY in VX.  

Jumping — BNNN opcode: "jump to address NNN + V0" doesn't use V0, but VX.  


**Key bindings** (using scancodes):  


    +---------------------------------------------------+
    |	COSMAC VIP's Chip-8   Customary modern PC's     |
    |	keyboard layout:      Chip-8 keyboard layout:   |
    |	1 2 3 C		          1 2 3 4                   |
    |	4 5 6 D		          Q W E R                   |
    |	7 8 9 E               A S D F                   |
    |	A 0 B F               Z X C V                   |
    +---------------------------------------------------+


How to use: 

			open Chip-8-Release\Chip-8-emu.exe.  
			
            To load files: press L, to play/run a file: press P.  
			
	        Press Esc to come back to the menu.  
			
