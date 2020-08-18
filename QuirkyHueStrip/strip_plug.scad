inner_diam=12.5;
outer_diam=14;
inner_wall=1;
$fn=100;

translate([0,0,4]) cylinder(d=outer_diam,h=1.5);
difference() {
    cylinder(d=inner_diam,h=4);
    translate([0,0,-1]) cylinder(d=inner_diam-inner_wall,h=4);
}