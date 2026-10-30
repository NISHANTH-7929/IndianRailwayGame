"""WAP-7 cab V2: an interior built to the openings of the exterior locomotive it sits in.

Run: blender -b --factory-startup --python build_wap7_cab_v2.py [-- --preview]
Writes 04_Assets/S10/WAP7CabV2 only. Earlier S8/S9 cab assets are left untouched.

Blender frame (same as the S8 cab): metres, +X forward, +Y = driver's LEFT, +Z up above rail.
FBX export flips Y, so Unreal locations are [x*100, -y*100, z*100] cm.

Every opening below was ray-cast from SM_WAP7_Selected_Exterior_LOD0 (see EXTERIOR_* constants):
the S8 cab used two 1.2 x 0.85 m windscreens 0.25 m too high, a full-length side window the
exterior does not have, and extended 0.4 m into the machine room. Driver ergonomics follow a
seated loco pilot on the left-hand desk: eye ~1.3 m behind the screen and ~0.24 m above its sill.
"""
import bpy, bmesh, math, json, sys, hashlib
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '04_Assets/S10/WAP7CabV2'
(OUT / 'Parts').mkdir(parents=True, exist_ok=True)
PREVIEW = '--preview' in sys.argv

# ---- Exterior measurements (ray-cast, metres) -------------------------------------------
EXTERIOR = '04_Assets/S8/Acquired/WAP7_Selected/Prepared/Parts/SM_WAP7_Selected_Exterior_LOD0.fbx'
BODY_HALF = 1.53                      # outer side skin
WIN_Y, WIN_Z0, WIN_Z1 = 1.15, 2.825, 3.375   # single windscreen, wiper pivots at the sill
SIDE_X0, SIDE_X1, SIDE_DIV = 8.70, 9.28, 9.07  # sliding side window and its divider
DOOR_X0, DOOR_X1, DOOR_TOP = 7.80, 8.38, 3.40  # cab door; its droplight follows the window band
DOORWIN_X0, DOORWIN_X1 = 7.86, 8.32
BAND_Z0, BAND_Z1 = 2.79, 3.33          # side glazing band (door and sliding window)
ROOF_CROWN, ROOF_BREAK_X, ROOF_SLOPE = 3.975, 8.57, .513   # cab roof falls toward the nose
CAB_REAR_X = 7.60                      # exterior roof steps down to the machine room here
def front_x(z):                        # sloped nose/windscreen plane, 16 deg from vertical
    return 9.773 - .2873 * (z - 2.80)
FRONT_PHI = math.atan(.2873)

# ---- Interior constants ----------------------------------------------------------------
FLOOR = 1.76
WALL = 1.45                            # inner lining face; glass sits at 1.50
GLASS_Y = 1.50
CEIL = 3.86                            # 2.1 m headroom under the crown
CANT_Z = 3.40                          # wall becomes a cant panel above the window band
CANT_Y = 1.05
def ceiling_z(x):
    return max(3.42, CEIL - ROOF_SLOPE * max(0., x - ROOF_BREAK_X))
def front_in(z):
    return front_x(z) - .06
EYE = Vector((8.36, .62, 3.07))        # seated driver, left desk (above the cushion centre)
def ceiling_profile_at(x):
    c = ceiling_z(x); drop = max(0., c - CANT_Z); k = 1 - min(1, drop/.46)
    return [(WALL, CANT_Z), (CANT_Y + (WALL-CANT_Y)*k, c), (-CANT_Y - (WALL-CANT_Y)*k, c), (-WALL, CANT_Z)]

bpy.ops.wm.read_factory_settings(use_empty=True)
groups, mats, moving = {}, {}, {}
# Names match the existing Unreal cab materials (/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Materials).
PALETTE = dict(CabIvory=(.54,.53,.47), DeskEnamel=(.30,.33,.33), PanelCharcoal=(.02,.024,.024),
               Rubber=(.012,.014,.014), Steel=(.16,.17,.16), Lettering=(.72,.76,.69),
               DialIvory=(.51,.53,.46), SeatBlue=(.07,.17,.23), Red=(.45,.015,.009),
               Amber=(.7,.22,.012), Green=(.035,.28,.085), Brass=(.40,.27,.07),
               DisplayGlass=(.009,.027,.025), Floor=(.07,.08,.075), WindowGlass=(.18,.25,.22))
for name, colour in PALETTE.items():
    m = bpy.data.materials.new(name); m.diffuse_color = (*colour, .25 if name == 'WindowGlass' else 1); mats[name] = m

def add(o, g, m):
    o.data.materials.append(mats[m]); groups.setdefault(g, []).append(o); return o
def box(name, loc, size, m='CabIvory', g='Static', bevel=.004, rot=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc); o = bpy.context.object; o.name = name
    o.scale = size; bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = o.modifiers.new('edge', 'BEVEL'); mod.width = bevel; mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    if rot: o.rotation_euler = rot
    return add(o, g, m)
def cyl(name, loc, r, depth, m='Steel', g='Static', axis=(0,0,1), verts=24):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=r, depth=depth, location=loc); o = bpy.context.object; o.name = name
    o.rotation_euler = Vector(axis).to_track_quat('Z', 'Y').to_euler()
    for p in o.data.polygons: p.use_smooth = len(p.vertices) == 4
    return add(o, g, m)
def rod(name, a, b, r, m='Steel', g='Static', verts=10):
    a, b = Vector(a), Vector(b); return cyl(name, (a+b)/2, r, (b-a).length, m, g, b-a, verts)
def sphere(name, loc, r, m='Rubber', g='Static'):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=10, radius=r, location=loc); o = bpy.context.object; o.name = name
    for p in o.data.polygons: p.use_smooth = True
    return add(o, g, m)
def poly(name, verts, m='CabIvory', g='Static', thickness=0.):
    me = bpy.data.meshes.new(name); me.from_pydata([tuple(v) for v in verts], [], [tuple(range(len(verts)))]); me.update()
    o = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(o)
    if thickness:
        mod = o.modifiers.new('solid', 'SOLIDIFY'); mod.thickness = thickness
        bpy.context.view_layer.objects.active = o; bpy.ops.object.modifier_apply(modifier=mod.name)
    return add(o, g, m)
def text(body, loc, size=.012, m='Lettering', g='Static', rot=(math.pi/2, 0, -math.pi/2)):
    bpy.ops.object.text_add(location=loc, rotation=rot); o = bpy.context.object
    o.data.body = body; o.data.size = size; o.data.align_x = 'CENTER'; o.data.align_y = 'CENTER'; o.data.resolution_u = 2
    bpy.ops.object.convert(target='MESH'); return add(bpy.context.object, g, m)

class Frame:
    """Canonical panel space: face is the plane x=0 facing -X, u=+Y, v=+Z; mapped onto a tilted panel."""
    def __init__(self, origin, phi):
        self.M = Matrix.Translation(Vector(origin)) @ Matrix.Rotation(phi, 4, 'Y')
    def __enter__(self):
        self.before = set(bpy.data.objects); return self
    def __exit__(self, *a):
        for o in set(bpy.data.objects) - self.before: o.matrix_world = self.M @ o.matrix_world
    def point(self, p): return tuple(self.M @ Vector(p))
    def axis(self, a): return tuple((self.M.to_3x3() @ Vector(a)).normalized())

def register(role, group, pivot, axis, rng):
    """axis: Blender axis about which a positive angle gives the intended motion (right-hand rule)."""
    moving[group] = dict(role=role, pivot=tuple(pivot), rotation_axis=list(axis), rotation_range_deg=list(rng))

# ======================= Shell ==========================================================
box('Floor plate', ((CAB_REAR_X+9.95)/2, 0, FLOOR-.03), (9.95-CAB_REAR_X, 2*WALL, .06), 'Floor', bevel=0)
for i in range(22): box('Anti-slip rib', (7.75+i*.075, .2, FLOOR+.002), (.01, 2.2, .004), 'Rubber', bevel=0)
# Rear bulkhead with the machine-room door on the assistant side.
# Bulkhead outline follows the ceiling profile so its corners stay inside the rounded roof.
rear_profile = [(CAB_REAR_X, -WALL, FLOOR), (CAB_REAR_X, WALL, FLOOR)] + [(CAB_REAR_X, yy, zz) for (yy, zz) in ceiling_profile_at(CAB_REAR_X)]
poly('Rear bulkhead', rear_profile, thickness=.05)
box('Machine room door', (CAB_REAR_X+.012, -.62, FLOOR+.97), (.02, .66, 1.90), 'DeskEnamel', bevel=.008)
rod('Machine door handle', (CAB_REAR_X+.05, -.36, 2.70), (CAB_REAR_X+.05, -.36, 2.86), .011)
box('Door window', (CAB_REAR_X+.025, -.62, 3.25), (.006, .30, .26), 'DisplayGlass', bevel=.002)

for s in (1, -1):                       # +1 driver (left), -1 assistant (right)
    y = s * (WALL + .02)
    t = (.04, )                         # lining thickness along Y
    front_low = front_in(FLOOR + .5)
    # Lower band below the glazing, rear of cab to the nose.
    box('Side lining low', ((CAB_REAR_X+front_low)/2, y, (FLOOR+BAND_Z0)/2), (front_low-CAB_REAR_X, .04, BAND_Z0-FLOOR), bevel=.006)
    # Glazing band: piers between the openings (door and sliding window are the only glass).
    for x0, x1 in ((CAB_REAR_X, DOOR_X0), (DOOR_X1, SIDE_X0), (SIDE_X1, front_in(3.1))):
        box('Side pier', ((x0+x1)/2, y, (BAND_Z0+BAND_Z1)/2), (x1-x0, .04, BAND_Z1-BAND_Z0), bevel=.004)
    box('Side lining high', ((CAB_REAR_X+front_in(3.37))/2, y, (BAND_Z1+CANT_Z)/2), (front_in(3.37)-CAB_REAR_X, .04, CANT_Z-BAND_Z1), bevel=.004)
    # Cab door (closed): inset panel, droplight frame, handle and hinges.
    box('Cab door panel', ((DOOR_X0+DOOR_X1)/2, y-s*.012, (FLOOR+BAND_Z0)/2), (DOOR_X1-DOOR_X0-.01, .03, BAND_Z0-FLOOR-.01), 'DeskEnamel', bevel=.006)
    for x in (DOOR_X0+.03, DOOR_X1-.03): box('Door stile', (x, y-s*.012, (BAND_Z0+BAND_Z1)/2), (.06, .03, BAND_Z1-BAND_Z0), 'DeskEnamel', bevel=.004)
    for z in (2.05, 3.10): box('Door hinge', (DOOR_X0+.01, y-s*.03, z), (.03, .02, .09), 'Steel', bevel=.003)
    rod('Door lever', (DOOR_X1-.07, y-s*.06, 2.40), (DOOR_X1-.07, y-s*.06, 2.56), .012)
    rod('Grab rail', (DOOR_X0-.06, y-s*.06, 2.05), (DOOR_X0-.06, y-s*.06, 3.05), .014)
    # Rubber seals around both side openings, sliding sash divider and catch.
    for x0, x1 in ((DOORWIN_X0, DOORWIN_X1), (SIDE_X0, SIDE_X1)):
        for z in (BAND_Z0, BAND_Z1): rod('Glazing seal', (x0, y-s*.025, z), (x1, y-s*.025, z), .012, 'Rubber')
        for x in (x0, x1): rod('Glazing seal', (x, y-s*.025, BAND_Z0), (x, y-s*.025, BAND_Z1), .012, 'Rubber')
    rod('Sliding sash divider', (SIDE_DIV, y-s*.03, BAND_Z0), (SIDE_DIV, y-s*.03, BAND_Z1), .016, 'Steel')
    box('Sash catch', (SIDE_DIV-.04, y-s*.05, 3.06), (.06, .02, .03), 'Rubber', bevel=.004)

# Sloped front: sill panel, pillars and header around the single windscreen.
def front_slab(name, y0, y1, z0, z1, m='CabIvory'):
    zc = (z0+z1)/2; L = (z1-z0)/math.cos(FRONT_PHI)
    box(name, (front_in(zc)+.02, (y0+y1)/2, zc), (.04, y1-y0, L), m, bevel=.006, rot=(0, -FRONT_PHI, 0))
front_slab('Front lower', -WALL, WALL, FLOOR, WIN_Z0)
for s in (1, -1): front_slab('Windscreen pillar', s*WIN_Y if s < 0 else WIN_Y, WALL if s > 0 else -WIN_Y, WIN_Z0, WIN_Z1) if s > 0 else front_slab('Windscreen pillar', -WALL, -WIN_Y, WIN_Z0, WIN_Z1)
front_slab('Windscreen header', -WALL, WALL, WIN_Z1, 3.44)
# Windscreen rubber gasket follows the opening in the sloped plane.
corners = [(-WIN_Y, WIN_Z0), (WIN_Y, WIN_Z0), (WIN_Y, WIN_Z1), (-WIN_Y, WIN_Z1)]
for i in range(4):
    (ya, za), (yb, zb) = corners[i], corners[(i+1) % 4]
    rod('Windscreen gasket', (front_in(za)-.01, ya, za), (front_in(zb)-.01, yb, zb), .013, 'Rubber')

# Ceiling: swept cross-section, flat crown with cant panels, falling toward the windscreen.
ceiling_profile = ceiling_profile_at
xs = [CAB_REAR_X + i*.08 for i in range(int((front_in(3.43)-CAB_REAR_X)/.08)+1)] + [front_in(3.43)]
verts, faces = [], []
for i, x in enumerate(xs):
    for (yy, zz) in ceiling_profile(x): verts.append((x, yy, zz))
    if i:
        a, b = (i-1)*4, i*4
        for k in range(3): faces.append((a+k, a+k+1, b+k+1, b+k))
me = bpy.data.meshes.new('Ceiling'); me.from_pydata(verts, [], faces); me.update()
ceiling = bpy.data.objects.new('Ceiling lining', me); bpy.context.collection.objects.link(ceiling); add(ceiling, 'Static', 'CabIvory')
for p in ceiling.data.polygons: p.use_smooth = True
box('Ceiling lamp', (8.20, 0, CEIL-.015), (.46, .16, .025), 'DialIvory', bevel=.012)
for x in (7.9, 8.5): box('Ceiling vent', (x, -.55, CEIL-.01), (.30, .22, .015), 'Steel', bevel=.004)
# Parked sun blinds just below the sloping roof, clear of the sight line.
for s in (1, -1): box('Sun blind (rolled)', (9.30, s*.57, ceiling_z(9.30)-.04), (.07, 1.0, .05), 'PanelCharcoal', bevel=.02)

# ======================= Desk and consoles ==============================================
DESK_EDGE, DESK_TOP, DESK_BACK = 8.86, 2.60, 9.18
CONSOLE_FRONT = front_in(2.80)
box('Desktop', ((DESK_EDGE+DESK_BACK)/2, 0, DESK_TOP-.02), (DESK_BACK-DESK_EDGE, 2*WALL-.04, .04), 'DeskEnamel', bevel=.008)
rod('Desk nosing', (DESK_EDGE, -WALL+.02, DESK_TOP-.02), (DESK_EDGE, WALL-.02, DESK_TOP-.02), .022, 'Rubber')
# Under-windscreen console rising to the sill (21 deg).
rise = math.atan((2.805-DESK_TOP)/(CONSOLE_FRONT-DESK_BACK))
L = math.hypot(CONSOLE_FRONT-DESK_BACK, 2.805-DESK_TOP)
box('Windscreen console', ((DESK_BACK+CONSOLE_FRONT)/2, 0, (DESK_TOP+2.805)/2-.015), (L, 2*WALL-.04, .03), 'DeskEnamel', bevel=.006, rot=(0, -rise, 0))
# Desk front with a knee recess under the driver's position.
KNEE0, KNEE1 = .20, 1.04
for y0, y1 in ((-WALL+.02, KNEE0), (KNEE1, WALL-.02)):
    box('Desk front', (DESK_EDGE+.02, (y0+y1)/2, (FLOOR+DESK_TOP-.04)/2), (.04, y1-y0, DESK_TOP-.04-FLOOR), 'DeskEnamel', bevel=.006)
box('Knee recess back', (9.32, (KNEE0+KNEE1)/2, (FLOOR+DESK_TOP)/2), (.03, KNEE1-KNEE0, DESK_TOP-FLOOR), 'DeskEnamel', bevel=.004)
for y in (KNEE0, KNEE1): box('Knee recess cheek', ((DESK_EDGE+9.32)/2, y, (FLOOR+DESK_TOP-.04)/2), (9.32-DESK_EDGE, .03, DESK_TOP-.04-FLOOR), 'DeskEnamel', bevel=.004)
rod('Footrest', (9.05, KNEE0+.06, FLOOR+.12), (9.05, KNEE1-.06, FLOOR+.12), .02)
for y in (.48, .78): box('Pedal', (9.10, y, FLOOR+.05), (.22, .13, .05), 'Rubber', bevel=.012, rot=(0, -.35, 0))
for y in (-.75, 1.22):
    box('Cabinet access plate', (DESK_EDGE-.002, y, 2.10), (.01, .28, .42), 'DeskEnamel', bevel=.004)
    for i in range(9): box('Cabinet vent', (DESK_EDGE-.008, y, 1.95+i*.025), (.008, .22, .006), 'PanelCharcoal', bevel=0)

# Instrument binnacle ahead of the driver: tilted 29 deg back toward the eye, hood above,
# top kept under the eye-to-sill sight line so the whole windscreen stays visible.
B_BOTTOM = Vector((DESK_BACK, 0, DESK_TOP)); B_TOP_DRIVER = Vector((9.345, 0, 2.895)); B_TOP_ASSIST = Vector((9.32, 0, 2.85))
def panel_phi(top): d = top - B_BOTTOM; return math.atan2(d.x, d.z)
PHI_D, PHI_A = panel_phi(B_TOP_DRIVER), panel_phi(B_TOP_ASSIST)
H_D, H_A = (B_TOP_DRIVER-B_BOTTOM).length, (B_TOP_ASSIST-B_BOTTOM).length
DRIVER_Y0, DRIVER_Y1 = .08, 1.32
ASSIST_Y0, ASSIST_Y1 = -1.34, -.04
for (y0, y1, phi, h, top, name) in ((DRIVER_Y0, DRIVER_Y1, PHI_D, H_D, B_TOP_DRIVER, 'Driver binnacle'), (ASSIST_Y0, ASSIST_Y1, PHI_A, H_A, B_TOP_ASSIST, 'Assistant panel')):
    with Frame(B_BOTTOM, phi):
        box(name+' face', (.012, (y0+y1)/2, h/2), (.024, y1-y0, h), 'PanelCharcoal', bevel=.004)
        box(name+' hood', (-.05, (y0+y1)/2, h+.012), (.12, y1-y0+.02, .018), 'DeskEnamel', bevel=.006)
        for y in (y0-.012, y1+.012): box(name+' cheek', (-.03, y, h/2), (.08, .024, h+.02), 'DeskEnamel', bevel=.006)
    # Closing panel between the hood and the windscreen console.
    poly(name+' back', [(top.x+.01, y0, top.z+.02), (top.x+.01, y1, top.z+.02), (CONSOLE_FRONT-.02, y1, 2.79), (CONSOLE_FRONT-.02, y0, 2.79)], 'DeskEnamel', thickness=.01)

def gauge(frame, u, v, r, label, maxvalue=10, role=None, rest=None, sweep=(-130, 130)):
    """Round dial in canonical panel space. Needle at rest unless a runtime role drives it."""
    dark = label in ('MR', 'FP'); ink = 'Lettering' if dark else 'PanelCharcoal'
    cyl('Gauge body', (-.010, u, v), r*1.08, .02, 'Rubber', axis=(-1, 0, 0), verts=40)
    cyl('Gauge bezel', (-.022, u, v), r*1.04, .006, 'Steel', axis=(-1, 0, 0), verts=40)
    cyl('Gauge dial', (-.026, u, v), r, .004, 'PanelCharcoal' if dark else 'DialIvory', axis=(-1, 0, 0), verts=40)
    for i in range(41):
        a = math.radians(sweep[0] + i*(sweep[1]-sweep[0])/40); major = i % 5 == 0
        rod('Graduation', (-.029, u-math.sin(a)*r*.86, v+math.cos(a)*r*.86), (-.029, u-math.sin(a)*r*(.70 if major else .79), v+math.cos(a)*r*(.70 if major else .79)), .0008, ink, verts=6)
        if i % 10 == 0: text(str(round(i/40*maxvalue)), (-.030, u-math.sin(a)*r*.55, v+math.cos(a)*r*.55), .009 if r < .07 else .013, ink)
    text(label, (-.030, u, v-r*.36), .008 if r < .07 else .012, ink)
    if role:
        g = role; rod('Needle', (-.034, u, v), (-.034, u, v+r*.68), .0018, 'Red', g, 8); cyl('Needle cap', (-.036, u, v), .006, .004, 'Steel', g, (-1, 0, 0), 12)
        register(role, g, frame.point((-.034, u, v)), frame.axis((1, 0, 0)), sweep if rest is None else rest)
    else:
        a = math.radians(sweep[0] + (sweep[1]-sweep[0]) * (rest or 0)/maxvalue)
        rod('Parked needle', (-.033, u, v), (-.033, u-math.sin(a)*r*.68, v+math.cos(a)*r*.68), .0016, 'Red', verts=8)

# Driver binnacle contents (u = world Y, v = up the panel).
with Frame(B_BOTTOM, PHI_D) as f:
    # Brake/air gauges on the left of the binnacle (kg/cm2 dials). BC follows the applied
    # brake; the others show a charged system at rest until the pneumatic model exists.
    gauge(f, 1.18, .215, .052, 'MR', 10, rest=8.5)
    gauge(f, 1.04, .215, .052, 'BP', 10, rest=5.0)
    gauge(f, 1.18, .085, .052, 'FP', 10, rest=6.0)
    # BC: full service ~3.8 kg/cm2 of a 0-10 dial.
    gauge(f, 1.04, .085, .052, 'BC', 10, role='gauge_brake', rest=(-130, -130+260*.38))
    # Speed indicator directly ahead of the driver, 0-180 km/h (class limit 140).
    gauge(f, .62, .175, .088, 'km/h', 180, role='gauge_speed')
    # Tractive effort meter.
    gauge(f, .86, .21, .046, 'TE', 10, role='gauge_power', rest=(-130, 130))
    # Driver display unit (DDU): recessed screen, bezel and soft keys.
    box('DDU bezel', (-.006, .30, .16), (.014, .27, .19), 'Rubber', bevel=.006)
    box('DDU screen', (-.014, .30, .175), (.004, .22, .13), 'DisplayGlass', bevel=.002)
    for i in range(6): box('DDU key', (-.016, .205+i*.038, .085), (.008, .026, .016), 'DeskEnamel', bevel=.003)
    text('DDU', (-.016, .30, .255), .009, 'Lettering')
    # Indicator lamps along the hood edge.
    for i, (label, colour) in enumerate((('LSDJ', 'Red'), ('LSAF', 'Amber'), ('LSCE', 'Amber'), ('BPFA', 'Green'))):
        cyl('Indicator rim', (-.012, .48+i*.07, .305), .011, .008, 'PanelCharcoal', axis=(-1, 0, 0), verts=16)
        cyl('Indicator lens', (-.018, .48+i*.07, .305), .008, .006, colour, axis=(-1, 0, 0), verts=16)
DDU_CENTRE = Frame(B_BOTTOM, PHI_D).point((-.018, .30, .20))
DDU_LOWER = Frame(B_BOTTOM, PHI_D).point((-.018, .30, .135))

# Assistant panel: electrical meters, indicators and the three lighting switches the game drives.
with Frame(B_BOTTOM, PHI_A) as f:
    for i, label in enumerate(('kV', 'A', 'V')):
        u = -.18 - i*.11
        box('Edgewise meter bezel', (-.008, u, .19), (.016, .085, .15), 'Steel', bevel=.004)
        box('Edgewise meter face', (-.017, u, .19), (.004, .068, .125), 'DialIvory', bevel=.002)
        text(label, (-.020, u, .14), .013, 'PanelCharcoal')
        rod('Meter needle', (-.021, u-.02, .19), (-.021, u+.02, .21), .0012, 'Red', verts=6)
    for i, label in enumerate(('LSDJ', 'LSHO', 'LSP', 'LSAF', 'LSVW')):
        u = -.58 - i*.075
        cyl('Annunciator rim', (-.012, u, .25), .014, .008, 'PanelCharcoal', axis=(-1, 0, 0), verts=16)
        cyl('Annunciator lens', (-.018, u, .25), .011, .006, 'Red' if i == 0 else 'Amber', axis=(-1, 0, 0), verts=16)
        text(label, (-.020, u, .222), .008)
    for i, (label, role) in enumerate((('HEAD LIGHT', 'switch_lamps'), ('WIPER', 'switch_wipers'), ('CAB LIGHT', 'switch_cablight'))):
        u = -.62 - i*.15
        cyl('Switch mounting', (-.012, u, .11), .02, .008, 'Steel', axis=(-1, 0, 0), verts=20)
        box('Rotary switch', (-.03, u, .11), (.022, .012, .036), 'Rubber', role, .004)
        text(label, (-.022, u, .07), .008)
        register(role, role, f.point((-.03, u, .11)), f.axis((1, 0, 0)), (-25, 25))
    for i, label in enumerate(('BLCP', 'BLDJ', 'ZPT', 'ZTEL', 'ZBAN')):
        u = -.15 - i*.085
        cyl('Selector bezel', (-.012, u, .065), .013, .006, 'Steel', axis=(-1, 0, 0), verts=16)
        box('Selector toggle', (-.028, u, .07), (.018, .009, .028), 'Rubber', bevel=.003)
        text(label, (-.02, u, .035), .008)

# ======================= Driver controls on the desktop =================================
# Flat desk legends read left-to-right from the driver's seat with their tops pointing forward.
DESK_TEXT = (0, 0, -math.pi/2)
Z = DESK_TOP
# TE/BE master controller to the driver's right: gate with notch scale, lever pitches forward for TE.
MC = Vector((9.00, .28, Z))
box('Master controller gate', (MC.x, MC.y, Z+.006), (.30, .16, .012), 'PanelCharcoal', bevel=.003)
box('Controller slot', (MC.x, MC.y, Z+.013), (.24, .02, .004), 'Rubber', bevel=.001)
for i in range(11):
    x = MC.x - .11 + i*.022
    box('Notch mark', (x, MC.y+.035, Z+.0135), (.003, .02 if i % 5 else .03, .001), 'Lettering', bevel=0)
text('TE', (MC.x+.13, MC.y+.06, Z+.014), .014, rot=DESK_TEXT); text('0', (MC.x-.12, MC.y+.06, Z+.014), .012, rot=DESK_TEXT)
rod('Master controller lever', (MC.x, MC.y, Z+.01), (MC.x, MC.y, Z+.14), .008, 'Steel', 'throttle')
box('Master controller T-grip', (MC.x, MC.y, Z+.15), (.035, .11, .028), 'Rubber', 'throttle', .012)
register('throttle', 'throttle', (MC.x, MC.y, Z+.01), (0, 1, 0), (-28, 32))
# Reverser (direction) key beside the controller.
RV = Vector((9.02, .46, Z))
cyl('Reverser drum', (RV.x, RV.y, Z+.02), .035, .04, 'PanelCharcoal', verts=24)
for i, label in enumerate(('R', '0', 'F')): text(label, (RV.x-.05+i*.05, RV.y+.05, Z+.002), .012, rot=DESK_TEXT)
rod('Reverser key', (RV.x, RV.y, Z+.04), (RV.x, RV.y, Z+.10), .007, 'Steel', 'reverser')
box('Reverser key grip', (RV.x, RV.y, Z+.105), (.02, .06, .022), 'Rubber', 'reverser', .008)
register('reverser', 'reverser', (RV.x, RV.y, Z+.04), (0, 1, 0), (-20, 20))
# Vigilance acknowledgement (VCD) and horn buttons near the right hand.
for (y, label, colour) in ((.12, 'VCD', 'Amber'), (.05, 'HORN', 'Rubber')):
    cyl('Push button rim', (9.08, y, Z+.006), .022, .012, 'Steel', verts=24)
    cyl('Push button', (9.08, y, Z+.016), .016, .014, colour, verts=24)
    text(label, (9.035, y, Z+.002), .009, rot=DESK_TEXT)
# Emergency brake mushroom within reach, centre of the desk.
cyl('Emergency collar', (9.06, -.06, Z+.008), .035, .016, 'Brass', verts=32)
sphere('Emergency mushroom', (9.06, -.06, Z+.03), .03, 'Red', 'switch_emergency')
register('switch_emergency', 'switch_emergency', (9.06, -.06, Z+.03), (1, 0, 0), (0, 0))
text('EMERGENCY', (9.00, -.06, Z+.002), .009, rot=DESK_TEXT)
# A9 automatic (train) brake valve on the driver's left pedestal, SA9 independent beside it.
PED = Vector((8.96, 1.18, Z+.03))
box('Brake pedestal', (PED.x-.02, PED.y, (FLOOR+PED.z)/2), (.34, .30, PED.z-FLOOR), 'DeskEnamel', bevel=.02)
box('A9 plate', (PED.x, PED.y, PED.z+.004), (.24, .22, .008), 'PanelCharcoal', bevel=.012)
cyl('A9 valve body', (PED.x, PED.y, PED.z+.03), .055, .05, 'Steel', verts=32)
# Handle modelled at mid-travel (pointing back to the driver); it sweeps counter-clockwise from
# above, REL at -40 deg through the self-lapping application zone to full service at +40.
HANDLE = math.radians(180)
grip = Vector((PED.x+math.cos(HANDLE)*.12, PED.y+math.sin(HANDLE)*.12, PED.z+.10))
rod('A9 handle', (PED.x, PED.y, PED.z+.06), grip, .009, 'Steel', 'train_brake')
sphere('A9 grip', grip, .022, 'Rubber', 'train_brake')
register('train_brake', 'train_brake', (PED.x, PED.y, PED.z+.06), (0, 0, 1), (-40, 40))
for label, deg in (('REL', -40), ('RUN', -28), ('MIN', -14), ('FS', 40), ('EMER', 58)):
    a = HANDLE + math.radians(deg); text(label, (PED.x+math.cos(a)*.095, PED.y+math.sin(a)*.095, PED.z+.009), .009, 'Red' if label == 'EMER' else 'Lettering', rot=DESK_TEXT)
cyl('SA9 valve body', (PED.x-.20, PED.y+.02, PED.z-.10), .04, .04, 'Steel', verts=24)
rod('SA9 handle', (PED.x-.20, PED.y+.02, PED.z-.08), (PED.x-.29, PED.y-.02, PED.z-.05), .008, 'Steel')
sphere('SA9 grip', (PED.x-.29, PED.y-.02, PED.z-.05), .018, 'Rubber')
text('SA9', (PED.x-.20, PED.y+.09, PED.z-.078), .01, rot=DESK_TEXT)

# ======================= Seats, equipment ===============================================
for y in (.62, -.70):
    cyl('Seat pedestal', (8.22, y, FLOOR+.24), .06, .48, 'Steel'); cyl('Seat base', (8.22, y, FLOOR+.02), .2, .04, 'PanelCharcoal')
    box('Seat cushion', (8.24, y, FLOOR+.52), (.46, .46, .10), 'SeatBlue', bevel=.045)
    back = box('Seat back', (7.98, y, FLOOR+.92), (.10, .44, .62), 'SeatBlue', bevel=.045); back.rotation_euler.y = -.12
    for e in (1, -1): box('Armrest', (8.22, y+e*.25, FLOOR+.74), (.30, .05, .04), 'Rubber', bevel=.015)
# Wall fan on the rear bulkhead, fire extinguisher, first-aid box and flag/lamp stowage.
fan = Vector((CAB_REAR_X+.10, .95, 3.50))
for r in (.06, .10, .14):
    bpy.ops.mesh.primitive_torus_add(major_radius=r, minor_radius=.0025, major_segments=40, minor_segments=6, location=fan, rotation=(0, math.pi/2, 0)); add(bpy.context.object, 'Static', 'Steel')
for i in range(3):
    a = i*math.tau/3; o = box('Fan blade', fan+Vector((.012, math.sin(a)*.065, math.cos(a)*.065)), (.006, .045, .11), 'PanelCharcoal', bevel=.015); o.rotation_euler.x = -a
cyl('Fan hub', fan, .022, .03, 'Steel', axis=(1, 0, 0))
cyl('Fire extinguisher', (CAB_REAR_X+.12, 1.25, FLOOR+.30), .075, .52, 'Red', verts=24)
box('First aid box', (CAB_REAR_X+.04, -1.20, 2.90), (.08, .26, .20), 'DialIvory', bevel=.01)
box('Flag and lamp locker', (CAB_REAR_X+.10, -1.25, FLOOR+.35), (.18, .34, .70), 'DeskEnamel', bevel=.01)

# ======================= Glazing and wipers =============================================
def pane(name, vs): poly(name, vs, 'WindowGlass', 'Glazing')
pane('Windscreen', [(front_x(WIN_Z0)-.015, -WIN_Y, WIN_Z0), (front_x(WIN_Z0)-.015, WIN_Y, WIN_Z0), (front_x(WIN_Z1)-.015, WIN_Y, WIN_Z1), (front_x(WIN_Z1)-.015, -WIN_Y, WIN_Z1)])
for s in (1, -1):
    for x0, x1 in ((SIDE_X0, SIDE_X1), (DOORWIN_X0, DOORWIN_X1)):
        pane('Side glass', [(x0, s*GLASS_Y, BAND_Z0), (x1, s*GLASS_Y, BAND_Z0), (x1, s*GLASS_Y, BAND_Z1), (x0, s*GLASS_Y, BAND_Z1)])
n = Vector((1, 0, .2873)).normalized(); up = Vector((-.2873, 0, 1)).normalized()
# Wipers park flat along the sill from pivots near the outer corners and sweep upward (0-80 deg).
for s_ in (1, -1):
    g = 'wiper_cab_' + ('1' if s_ > 0 else '-1')
    pivot = Vector((front_x(WIN_Z0+.035)+.025, s_*.98, WIN_Z0+.035))
    tip = pivot + Vector((0, -s_*.56, 0))
    cyl('Wiper spindle', pivot, .018, .03, 'PanelCharcoal', g, tuple(n), 12)
    rod('Wiper arm', pivot, tip, .006, 'PanelCharcoal', g, 8)
    rod('Wiper blade', pivot + Vector((0, -s_*.10, .012)), tip + Vector((0, -s_*.05, .012)), .008, 'Rubber', g, 8)
    register('wiper', g, pivot, tuple(-n if s_ > 0 else n), (0, 80))

# ======================= UVs, join, export ==============================================
def box_uv(o):
    me = o.data
    for name in ('UVMap', 'DetailUV'):
        if name not in me.uv_layers: me.uv_layers.new(name=name)
    for layer in me.uv_layers:
        for p in me.polygons:
            nrm = p.normal; ax = max(range(3), key=lambda k: abs(nrm[k]))
            for li in p.loop_indices:
                co = me.vertices[me.loops[li].vertex_index].co
                u, v = [co[k] for k in range(3) if k != ax]
                layer.data[li].uv = (u*2, v*2)

parts = []
def to_ue(v): return [v[0]*100, -v[1]*100, v[2]*100]
for group, objects in groups.items():
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects: o.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]; bpy.ops.object.join(); o = bpy.context.object
    o.name = 'SM_WAP7_CabV2_' + group.replace('-', 'm')
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    meta = moving.get(group, {}); pivot = Vector(meta.get('pivot', (0, 0, 0)))
    bpy.context.scene.cursor.location = pivot; bpy.ops.object.origin_set(type='ORIGIN_CURSOR'); o.location = (0, 0, 0)
    box_uv(o)
    fname = o.name + '.fbx'
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Parts'/fname), use_selection=True, object_types={'MESH'}, axis_forward='X', axis_up='Z',
                             apply_unit_scale=True, add_leaf_bones=False, use_triangles=True, mesh_smooth_type='FACE', path_mode='STRIP')
    o.location = pivot; o.data.calc_loop_triangles()
    axis = meta.get('rotation_axis', [1, 0, 0])
    parts.append(dict(name=group, mesh=o.name, role=meta.get('role', 'cab_static'), fbx='Parts/'+fname, location_cm=to_ue(pivot),
                      rotation_deg=[0, 0, 0], scale=[1, 1, 1], cab_only=True, rotation_axis=[-axis[0], axis[1], -axis[2]],  # Y mirror: axis -> (-x, y, -z), same angle
                      rotation_range_deg=meta.get('rotation_range_deg', [0, 0]), triangles=len(o.data.loop_triangles),
                      materials=[m.name for m in o.data.materials]))

ddu_pitch = math.degrees(PHI_D)
manifest = dict(
    revision='wap7-cab-v2', units='centimetres', axis='+X forward +Y right +Z up',
    exterior_reference=EXTERIOR,
    driver_camera=dict(location_cm=to_ue(EYE), rotation_deg=[-4, 6, 0], fov=78),  # yaw toward the track centre
    instrument_view_pitch_deg=-17,
    instrument_display=dict(location_cm=to_ue(DDU_LOWER), rotation_deg=[ddu_pitch, 180, 0], size_cm=1.5),
    speed_readout=dict(location_cm=to_ue(DDU_CENTRE), rotation_deg=[ddu_pitch, 180, 0], size_cm=3.2),
    cab_light_location_cm=to_ue((8.20, 0, CEIL-.06)),
    openings_m=dict(windscreen=dict(y=[-WIN_Y, WIN_Y], z=[WIN_Z0, WIN_Z1]), side_window=dict(x=[SIDE_X0, SIDE_X1], z=[BAND_Z0, BAND_Z1]),
                    door=dict(x=[DOOR_X0, DOOR_X1], z=[FLOOR, DOOR_TOP]), cab_rear_x=CAB_REAR_X, floor_z=FLOOR),
    parts=parts, triangles=sum(p['triangles'] for p in parts),
    limitations=['Openings and roof line measured from the CC BY 4.0 exterior model, not a CLW drawing.',
                 'Desk layout follows the conventional IR left-hand driving desk (A9/SA9 left, TE/BE controller right); exact WAP-7 panel legends are representative.',
                 'MR/BP/FP needles show a charged system at rest; BC follows applied brake until the pneumatic model exists.'])
(OUT/'assembly.json').write_text(json.dumps(manifest, indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WAP7_CabV2.blend'))
print('CABV2_BUILT', manifest['triangles'], len(parts))

if PREVIEW:
    sc = bpy.context.scene; sc.render.engine = 'BLENDER_WORKBENCH'
    sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'MATERIAL'
    sc.render.resolution_x = 1600; sc.render.resolution_y = 900
    cam = bpy.data.cameras.new('eye'); co = bpy.data.objects.new('eye', cam); sc.collection.objects.link(co); sc.camera = co
    cam.lens_unit = 'FOV'; cam.sensor_fit = 'HORIZONTAL'; cam.angle = math.radians(78); cam.clip_start = .02
    co.location = EYE; co.rotation_euler = (math.radians(90-4), 0, math.radians(-90-6))
    sc.render.filepath = str(OUT/'preview_driver.png'); bpy.ops.render.render(write_still=True)
    co.location = (7.75, -1.15, 3.55); co.rotation_euler = (Vector((9.3, .4, 2.75)) - co.location).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = str(OUT/'preview_overview.png'); bpy.ops.render.render(write_still=True)
    # Fit check: exterior shell (transparent) over the interior, side and front orthographic.
    bpy.ops.import_scene.fbx(filepath=str(ROOT/EXTERIOR))
    for o in bpy.context.selected_objects: o.color = (.6, .6, .65, .35)
    sc.display.shading.color_type = 'OBJECT'; sc.display.shading.show_xray = True; sc.display.shading.xray_alpha = .45
    for o in bpy.data.objects:
        if o.name.startswith('SM_WAP7_CabV2'): o.color = (.9, .3, .2, 1)
    cam.type = 'ORTHO'
    for name, loc, rot, scale in (('fit_side', (8.8, -12, 2.85), (math.radians(90), 0, 0), 3.4), ('fit_front', (20, 0, 2.85), (math.radians(90), 0, math.radians(90)), 3.6)):
        co.location = loc; co.rotation_euler = rot; cam.ortho_scale = scale
        sc.render.filepath = str(OUT/(name+'.png')); bpy.ops.render.render(write_still=True)
