//mirror([0, 0, 1]) // B
stencil();

module edge(){
    import("c:/Users/Tereza/Documents/KiCad/dmx-box/Edge_Cuts.gbr_svg.svg");
}

module paste(){
    difference(){
        import("c:/Users/Tereza/Documents/KiCad/dmx-box/F_Paste.gbr_iso_joined_svg.svg");
        edge();
    }
}

module pins(){
    difference(){
        import("c:/Users/Tereza/Documents/KiCad/dmx-box/drl_edit_conv_joined_svg.svg");
        edge();
    }
}

module stencil(){
    union(){
        difference(){
            hull(){
                linear_extrude(height = 0.15) {
                    edge();
                }
            }
            linear_extrude(height = 1, center = true) {
                    paste();
            }
        }
        translate([0, 0, -1]){
            linear_extrude(height = 1) {
                pins();
            }
        }
//        translate([0, 0, 0.125 + 0.10]){
//            hull(){
//                linear_extrude(height = 0.2) {
//                    edge();
//                }
//            }
//        }
    }
}
