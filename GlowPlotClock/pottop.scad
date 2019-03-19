module orig() {
    import("i:/incoming/Glow-In-Dark_Plot_Clock/files/Case_Top.STL");
};



module sliver() {
difference() {
    orig();
    translate([0,1,0]) cube([90,10,45]);
}
}

difference() {
    union() {
        orig();
        for(a=[0:10])
        translate([0,-0.5-0.5*a,0]) sliver();
    }
    translate([44.5,3,26]) rotate([90,0,0]) cylinder(h=10,d=25,$fn=100);
}