//
// gear motor for
// https://www.thingiverse.com/thing:1385312
//

m_dia = 49.8;
m_hgt = 23.0;
m1_dist = 56.7;
m2_dist = 54.8;
m2_offs = 7.0;
mt_hgt = 0.8;
mt_dia = 4.4;
a_ofs = 13.0;
r1_dia = 12.0;
r2_dia = 10.4;
r1_hgt = 2.0;
r2_hgt = 1.0;
a_dia = 7.0; // axle diameter
a_len = 15.0;
a2_len = 12.0; // axle flat section
a2_wid = 5.0;

adapter_dia = 8.0;
adapter_hgt = 10.5;
adapter_tol = 0.15;

base_dia = 140.0;
base_hgt = 2.4;
base_tol = 0.2;
base_wall = 1.5;

conn_dia = 10.0; // 10.9

d = 0.01;

// adapter for existing spiral
module adapter() {
    translate([0,0,m_hgt+r1_hgt+r2_hgt]) {
        difference () {
            color("red") translate([0,0,a_len-adapter_hgt]) cylinder(d=adapter_dia-2*adapter_tol, h=adapter_hgt, $fn=60);
            difference () {
                translate([0,0,-d]) cylinder(d=a_dia+2*adapter_tol, h=d+a_len+d, $fn=60);
                translate([-a_dia/2,-a_dia/2+a2_wid+adapter_tol,a_len-a2_len]) cube([a_dia, a_dia, a2_len+d]);
            }
        }
    }
}

module base() {
    difference () {
        union () { // add
            translate([0,0,-base_wall-base_tol-base_hgt]) 
              color("brown") cylinder(d=base_dia+2*base_tol+2*base_wall, h=3, $fn=120);
            // hole for connector
            /*
            translate([0,base_dia/2,m_hgt/2]) rotate([-90,0,0]) cylinder(d=conn_dia+2*base_wall, h=1.5*base_wall, $fn=30);
            */
        }
        union () { // sub
            
            translate([0,0,-base_tol-base_hgt]) {
                cylinder(d=base_dia-2*base_wall, h=base_hgt+40/*base_tol+base_hgt+(m_hgt+r1_hgt+r2_hgt+a_len-adapter_hgt)+d*/, $fn=120);
            }
            
            translate([0,0,-31-base_hgt+(m_hgt+r1_hgt+r2_hgt+a_len-adapter_hgt)]) {
                cylinder(d=base_dia+2*base_tol, h=base_hgt+100*d, $fn=120);
            }
            // hole for connector
            /*
            translate([0,base_dia/2-2*base_wall,m_hgt/2]) rotate([-90,0,0]) cylinder(d=conn_dia, 
            h=2*base_wall+1.5*base_wall+d, $fn=30);
            */
        }
    }

/*
    translate([0,-a_ofs,-base_tol-base_hgt]) {
        for (dx = [-1,1]) translate([dx*m1_dist/2,0,0]) {
            difference () {
                scale([1,1.9,1]) cylinder(d=6.0, h=base_tol+base_hgt+m_hgt-mt_hgt, $fn=60);
                cylinder(d=2.6, h=base_tol+base_hgt+m_hgt-mt_hgt+d, $fn=60);
            }
        }
        for (dx = [-1,1]) translate([dx*m2_dist/2,-m2_offs,0]) {
            difference () {
                scale([1,1.9,1]) cylinder(d=6.0, h=base_tol+base_hgt+m_hgt-mt_hgt, $fn=60);
                cylinder(d=2.6, h=base_tol+base_hgt+m_hgt-mt_hgt+d, $fn=60);
            }
        }
    }
 */   
}

module motor() {
    translate([0,0,m_hgt]) {
        cylinder(d=r1_dia, h=r1_hgt, $fn=60);
        cylinder(d=r2_dia, h=r1_hgt+r2_hgt, $fn=60);
        difference () {
            cylinder(d=a_dia, h=r1_hgt+r2_hgt+a_len, $fn=60);
            translate([-a_dia/2,-a_dia/2+a2_wid,r1_hgt+r2_hgt+a_len-a2_len]) cube([a_dia, a_dia, a2_len+d]);
        }
    }
    translate([0,-a_ofs,0]) {
            cylinder(d=m_dia, h=m_hgt, $fn=120);
        for (dx = [-1,1]) translate([dx*m1_dist/2,0,m_hgt-mt_hgt]) {
            difference () {
                cylinder(d=10.0, h=mt_hgt, $fn=60);
                translate([0,0,-d])cylinder(d=mt_dia, h=mt_hgt+2*d, $fn=60);
            }
        }
        for (dx = [-1,1]) translate([dx*m2_dist/2,-m2_offs,m_hgt-mt_hgt]) {
            difference () {
                cylinder(d=10.0, h=mt_hgt, $fn=60);
                translate([0,0,-d])cylinder(d=mt_dia, h=mt_hgt+2*d, $fn=60);
            }
        }
    }
}

//adapter();
difference() {
base();
//color("red") translate([-9.25,4.8,-2.5]) import("MotorMount.stl");
color("purple") translate([0,0,-4.5]) cylinder(h=3,r1=5,r2=5,$fn=120);
}
//motor();