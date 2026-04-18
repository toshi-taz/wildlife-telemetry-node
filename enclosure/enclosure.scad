// Wildlife Telemetry Node — Enclosure v0.1
// Parametrizable — actualizar medidas cuando llegue el vernier
// Autor: Toshi — ISERI IPN Zacatenco

// =====================
// PARÁMETROS (editar aquí)
// =====================

// ESP32 (con pines)
esp32_l = 51;
esp32_w = 28;
esp32_h = 13;

// GPS GY-GPS6MV2
gps_l = 26;
gps_w = 23;
gps_h = 6;

// Holgura interna
margin = 4;

// Pared
wall = 3;

// Dimensiones internas calculadas
inner_l = esp32_l + margin * 2;
inner_w = esp32_w + gps_l + margin * 3;
inner_h = esp32_h + margin * 2;

// Dimensiones externas
ext_l = inner_l + wall * 2;
ext_w = inner_w + wall * 2;
ext_h = inner_h + wall * 2;

// Radio de esquinas
corner_r = 6;

// =====================
// MÓDULOS
// =====================

module rounded_box(l, w, h, r) {
    hull() {
        translate([r, r, 0])       cylinder(h=h, r=r, $fn=32);
        translate([l-r, r, 0])     cylinder(h=h, r=r, $fn=32);
        translate([r, w-r, 0])     cylinder(h=h, r=r, $fn=32);
        translate([l-r, w-r, 0])   cylinder(h=h, r=r, $fn=32);
    }
}

module enclosure_body() {
    difference() {
        // Cuerpo exterior
        rounded_box(ext_l, ext_w, ext_h, corner_r);
        // Vaciado interior
        translate([wall, wall, wall])
            rounded_box(inner_l, inner_w, inner_h, corner_r - wall);
        // Ventana GPS (tapa superior)
        translate([wall + margin + esp32_l/2 - 15, wall + margin + esp32_w + margin/2, ext_h - wall])
            cube([30, 24, wall + 1]);
        // Puerto USB-C (frente)
        translate([wall + margin + esp32_l/2 - 6, -1, wall + 4])
            cube([12, wall + 2, 6]);
        // Agujeros tornillos tapa (4 esquinas M3)
        screw_holes();
    }
}

module screw_holes() {
    inset = 8;
    for (x = [inset, ext_l - inset])
        for (y = [inset, ext_w - inset])
            translate([x, y, ext_h - wall - 1])
                cylinder(h = wall + 2, r = 1.6, $fn = 16);
}

module esp32_placeholder() {
    color("blue", 0.5)
    translate([wall + margin, wall + margin, wall + margin])
        cube([esp32_l, esp32_w, esp32_h]);
}

module gps_placeholder() {
    color("green", 0.5)
    translate([wall + margin, wall + margin + esp32_w + margin, wall + margin])
        cube([gps_l, gps_w, gps_h]);
}

module standoffs() {
    h = margin;
    r = 2;
    positions = [
        [wall + margin, wall + margin],
        [wall + margin + esp32_l, wall + margin],
        [wall + margin, wall + margin + esp32_w],
        [wall + margin + esp32_l, wall + margin + esp32_w]
    ];
    for (p = positions)
        translate([p[0], p[1], wall])
            difference() {
                cylinder(h=h, r=r, $fn=16);
                cylinder(h=h+1, r=0.9, $fn=16);
            }
}

// =====================
// RENDER
// =====================

enclosure_body();
standoffs();
esp32_placeholder();
gps_placeholder();

