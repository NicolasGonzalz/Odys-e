"""Pièces plates à partir d'un contour mesuré sur une image (tête de hache, lame, plaque…).

Le contour relevé pixel par pixel est simplifié en quelques arêtes droites (look low-poly),
puis épaissi : l'épaisseur peut varier selon x (fine au tranchant, épaisse au talon).
"""
import math

import bmesh
import bpy


# --- Simplification du contour ------------------------------------------------------------------

def _distance_to_line(point, start, end):
    (px, pz), (ax, az), (bx, bz) = point, start, end
    length = math.hypot(bx - ax, bz - az)
    if length == 0:
        return math.hypot(px - ax, pz - az)
    return abs((bx - ax) * (az - pz) - (ax - px) * (bz - az)) / length


def simplify(points, tolerance):
    """Douglas-Peucker : ne garde que les points qui s'écartent de plus de `tolerance` (m)."""
    if len(points) < 3:
        return list(points)
    distances = [_distance_to_line(p, points[0], points[-1]) for p in points[1:-1]]
    farthest = max(range(len(distances)), key=distances.__getitem__) + 1
    if distances[farthest - 1] <= tolerance:
        return [points[0], points[-1]]
    return simplify(points[:farthest + 1], tolerance)[:-1] + simplify(points[farthest:], tolerance)


def _turns_back(previous, point, following):
    """Vrai si le contour repart presque dans la direction d'où il vient (une pointe aller-retour)."""
    ax, az = point[0] - previous[0], point[1] - previous[1]
    bx, bz = following[0] - point[0], following[1] - point[1]
    norms = math.hypot(ax, az) * math.hypot(bx, bz)
    return norms > 0 and (ax * bx + az * bz) / norms < -0.9


def clean_outline(points, min_distance=0.004):
    """Retire les points en double et les allers-retours : un contour qui se recoupe (en « nœud
    papillon ») ferait perdre une partie de la face à Blender."""
    cleaned = []
    for point in points:
        if not cleaned or math.dist(point, cleaned[-1]) >= min_distance:
            cleaned.append(point)
    if len(cleaned) > 2 and math.dist(cleaned[0], cleaned[-1]) < min_distance:
        cleaned.pop()

    changed = True
    while changed and len(cleaned) > 3:
        changed = False
        for i in range(len(cleaned)):
            if _turns_back(cleaned[i - 1], cleaned[i], cleaned[(i + 1) % len(cleaned)]):
                del cleaned[i]
                changed = True
                break
    return cleaned


# --- Pièce épaisse ------------------------------------------------------------------------------

def _flat_face(bm, outline):
    vertices = [bm.verts.new((x, 0.0, z)) for x, z in outline]
    return bm.faces.new(vertices)


def _cut(bm, cuts_x):
    """Coupes verticales : l'épaisseur pourra changer d'un côté à l'autre de chaque coupe."""
    for x in cuts_x:
        geometry = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(bm, geom=geometry, plane_co=(x, 0, 0), plane_no=(1, 0, 0))


def _thicken(bm, thickness_at):
    """Double la face (avant / arrière) et écarte chaque sommet selon l'épaisseur à son x."""
    front = bm.verts[:]
    extruded = bmesh.ops.extrude_face_region(bm, geom=bm.faces[:])
    back = [g for g in extruded["geom"] if isinstance(g, bmesh.types.BMVert)]
    for vertex in front:
        vertex.co.y = -thickness_at(vertex.co.x) / 2
    for vertex in back:
        vertex.co.y = thickness_at(vertex.co.x) / 2
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])


def profile_solid(name, outline, thickness_at, materials, material_index_at, cuts_x=(),
                  chamfer=0.0025, segments=1):
    """Pièce épaisse d'après un contour (x, z) dans le plan de face.

    `thickness_at(x)` donne l'épaisseur ; `material_index_at(x)` le matériau d'une face selon
    la position de son centre (ex. bande claire du tranchant). `chamfer` et `segments` : un petit
    chanfrein (arête vive) par défaut, ou un arrondi (plusieurs segments) pour une crosse en bois.
    """
    bm = bmesh.new()
    _flat_face(bm, clean_outline(outline))
    _cut(bm, cuts_x)
    _thicken(bm, thickness_at)
    for face in bm.faces:
        face.material_index = material_index_at(face.calc_center_median().x)

    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for material in materials:
        obj.data.materials.append(material)
    _chamfer(obj, chamfer, segments)
    return obj


def _chamfer(obj, width=0.0025, segments=1):
    """Petit chanfrein sur les arêtes vives : elles accrochent la lumière, comme sur les planches."""
    modifier = obj.modifiers.new("Chanfrein", "BEVEL")
    modifier.width = width
    modifier.segments = segments
    modifier.limit_method = "ANGLE"
    modifier.angle_limit = 0.6  # radians (~35°) : seulement les arêtes nettes
