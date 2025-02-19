#include <gb/gb.h>
#include <stdint.h>
#include "LaroldsJubilantJunkyard.h"
#include "splash_bg_asset.h"


void main(void)
{
  DISPLAY_ON;
  SHOW_BKG;

     // Load & set our background data
    set_bkg_data(0,splash_bg_asset_TILE_COUNT,splash_bg_asset_tiles);
    
    // The gameboy screen is 160px wide by 144px tall
    // We deal with tiles that are 8px wide and 8px tall
    // 160/8 = 20 and 120/8=15
    set_bkg_tiles(0,1,20,15,splash_bg_asset_map);

    // Loop forever
    while(1) {


		// Game main loop processing goes here


		// Done processing, yield CPU and wait for start of next frame
        vsync();
    }
}
