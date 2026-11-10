"""Contour noir façon dessin animé, comme sur les planches de concepts (« inverted hull »).

Une copie du modèle, légèrement gonflée, aux faces retournées et d'une couleur sombre : vue de face,
elle est invisible (ses faces regardent vers l'intérieur et sont écartées) ; sur les bords, elle
dépasse et dessine un trait. Ça marche tel quel dans Unreal (faces arrière non dessinées par défaut)
et ne demande aucun effet de post-traitement.
"""
import bmesh
import bpy

from .materials import flat_material

INK = (0.07, 0.06, 0.06)
OUTLINED = "contour"  # groupe de sommets qui reçoivent le contour (voir without_outline)


def without_outline(obj):
    """Marque un petit détail peint (trait du fil, nœud, cernes) : pas de contour noir, sinon
    un trait de 1 mm deviendrait un gros trait noir de 8 mm. Lu par finish.join."""
    obj["sans_contour"] = True
    return obj


def mark_outlined(obj):
    """Met les sommets d'une pièce (non marquée sans_contour) dans le groupe OUTLINED."""
    if obj.get("sans_contour"):
        return
    group = obj.vertex_groups.new(name=OUTLINED)
    group.add(list(range(len(obj.data.vertices))), 1.0, "REPLACE")


def ink_outline(obj, thickness=0.0035):
    """Ajoute le contour au modèle (même objet, matériau « encre » en dernier emplacement)."""
    ink = flat_material("encre", INK)
    ink.use_backface_culling = True
    obj.data.materials.append(ink)

    modifier = obj.modifiers.new("Contour", "SOLIDIFY")
    modifier.thickness = thickness
    modifier.offset = 1.0              # vers l'extérieur
    modifier.use_flip_normals = True   # faces retournées : invisibles de face
    modifier.use_rim = False
    modifier.material_offset = len(obj.data.materials) - 1
    if OUTLINED in obj.vertex_groups:  # les détails marqués sans_contour gardent une épaisseur nulle
        modifier.vertex_group = OUTLINED
        modifier.thickness_vertex_group = 0.0

    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    _drop_empty_shells(obj)
    return obj


def _drop_empty_shells(obj):
    """Supprime la coque des détails sans_contour : d'épaisseur nulle, collée à l'envers sur la
    pièce, elle ne sert à rien et peut clignoter en noir dans un rendu qui montre les faces arrière."""
    if OUTLINED not in obj.vertex_groups:
        return
    ink_index = len(obj.data.materials) - 1
    group = obj.vertex_groups[OUTLINED].index
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    weights = bm.verts.layers.deform.active
    empty = [f for f in bm.faces if f.material_index == ink_index
             and all(group not in v[weights] for v in f.verts)]
    bmesh.ops.delete(bm, geom=empty, context="FACES")
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
