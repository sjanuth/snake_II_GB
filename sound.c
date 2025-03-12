#include <gb/gb.h>

void play_eating_sound(void) {
    NR52_REG = 0x80;  // Enable sound
    NR50_REG = 0x77;  // Set volume (max)
    NR51_REG  = 0x11;  // Enable sound on both channels

    NR10_REG= 0x16;  // Sweep settings (increase pitch)
    NR11_REG= 0x3F;  // Duty cycle 50% (square wave)
    NR12_REG= 0xD0;  // Envelope: Max volume, short decay
    NR13_REG= 0x90;  // Frequency (higher values = lower pitch)
    NR14_REG= 0x87;  // Start sound, enable length counter
}

void play_game_over_sound(void){

    NR52_REG = 0x80;  // Enable sound
    NR50_REG = 0x77;  // Set volume (max)
    NR51_REG  = 0x11;  // Enable sound on both channels

    NR10_REG= 0x32;  // Sweep settings (increase pitch)
    NR11_REG= 0xBF;  // Duty cycle 50% (square wave)
    NR12_REG= 0xFF;  // Envelope: Max volume, short decay
    NR13_REG= 0x61;  // Frequency (higher values = lower pitch)
    NR14_REG= 0x85;  // Start sound, enable length counter
    delay(12);
    NR52_REG = 0x80;  // Enable sound
    NR10_REG= 0x32;  // Sweep settings (increase pitch)
    NR11_REG= 0xBF;  // Duty cycle 50% (square wave)
    NR12_REG= 0xFF;  // Envelope: Max volume, short decay
    NR13_REG= 0x61;  // Frequency (higher values = lower pitch)
    NR14_REG= 0x85;  // Start sound, enable length counter
    delay(20);
    NR52_REG = 0x80;  // Enable sound
    NR10_REG= 0x32;  // Sweep settings (increase pitch)
    NR11_REG= 0xBF;  // Duty cycle 50% (square wave)
    NR12_REG= 0xFF;  // Envelope: Max volume, short decay
    NR13_REG= 0x61;  // Frequency (higher values = lower pitch)
    NR14_REG= 0x85;  // Start sound, enable length counter
}
