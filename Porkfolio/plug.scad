hole_diam=34.5;

bottom_diam=40;
top_diam=21;
offset=25;

width=1.4;
depth=8;

m4=3.3;
m3=2.5;

plug_thick=2;

$fn=200;

module cap() {
    difference() {  
        union() {  
            translate([0,0,-depth/2]) difference() {
                cylinder(d=hole_diam,h=depth,center=true);
                translate([0,0,-1]) cylinder(d=hole_diam-plug_thick,h=depth,center=true);
}
            translate([0,0,width/2]) hull() {
                cylinder(d=bottom_diam,h=width,center=true);
                translate([0,offset,0]) cylinder(d=top_diam,h=width,center=true);
                }
                }
    translate([0,offset+3,1]) cylinder(d=m3,h=width*2,center=true);
    }
}

plugl=13;
plugw=8.75;
socket_depth=21;
socket_wall=2;

module socket() {
    translate([0,0,-socket_depth/2]) difference() {
        cube([plugl+2*socket_wall,plugw+2*socket_wall,socket_depth],center=true);
        translate([0,0,-1]) cube([plugl,plugw,socket_depth],center=true);
        translate([0,0,-5]) rotate([90,0,0]) cylinder(d=m3,h=plugw*2,center=true);
    }
}

difference() {
union() {
cap();
socket();
}
  translate([0,0,-socket_depth]) cube([plugl,plugw,socket_depth*2],center=true);
 translate([0,0,-socket_depth]) cube([10,5,socket_depth*2+4],center=true);
}