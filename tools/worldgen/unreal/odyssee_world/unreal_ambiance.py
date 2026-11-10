"""Ambiance : ciel et soleil, plan d'eau, et brumes propres à chaque zone (voir art/concepts/)."""
import unreal

from .unreal_util import actors, safe_set, spawn, to_cm

FOLDER = "Odyssee/Ambiance"

# Brumes locales (LocalFogVolume) : position (m), taille (m : largeur, profondeur, hauteur),
# couleur, lueur et densité. Réglages de départ, à ajuster à l'œil dans l'éditeur.
ZONE_MISTS = [
    # L'Abri : brume matinale sur les prairies et la rivière (zone-1-plaine.jpg)
    ("Brume — prairie ouest", (900, 2350), (900, 700, 25), (0.92, 0.93, 0.95), 0.0, 0.6),
    ("Brume — rivière", (2000, 1975), (1400, 300, 20), (0.92, 0.94, 0.97), 0.0, 0.8),
    ("Brume — prairie est", (2900, 2700), (900, 500, 25), (0.92, 0.93, 0.95), 0.0, 0.5),
    # Le Cœur : voile bleu-vert, lueur des champignons et des cocons (zones-1-3-5-lieux.jpeg)
    ("Voile — forêt du Cœur ouest", (2000, 450), (1300, 800, 60), (0.30, 0.75, 0.70), 0.05, 1.0),
    ("Voile — forêt du Cœur est", (3300, 450), (1400, 800, 60), (0.30, 0.75, 0.70), 0.05, 1.0),
    # Le Site des Portes : brume de neige légère
    ("Brume — plateau enneigé", (700, 900), (1300, 1500, 120), (0.95, 0.96, 1.00), 0.0, 0.4),
]

# Brouillard toxique de la Centrale (3 niveaux) : violet, de plus en plus dense vers le centre.
TOXIC_COLOR = (0.55, 0.30, 0.75)
TOXIC_DENSITY = {"leger": 0.8, "dense": 2.0, "mortel": 5.0}


def build_ambiance(layout_data, assets):
    """`layout_data` : le contenu brut de world_layout.json."""
    _sky()
    _water(layout_data, assets)
    if not hasattr(unreal, "LocalFogVolume"):
        unreal.log_warning("[Odyssée] LocalFogVolume indisponible : brumes non créées")
        return
    for label, center, size, color, glow, density in ZONE_MISTS:
        _mist(label, center, size, color, glow, density)
    _toxic_fog(layout_data["toxic_fog"])


# --- Ciel -------------------------------------------------------------------------------------

def _exists(actor_class):
    """Vrai si le niveau contient déjà un acteur de ce type (on ne crée pas de doublon)."""
    return any(isinstance(a, actor_class) for a in actors().get_all_level_actors())


def _sky():
    if not _exists(unreal.DirectionalLight):
        sun = spawn(unreal.DirectionalLight, to_cm(2016, 1512, 600), "Soleil — matin", FOLDER,
                    rotation=unreal.Rotator(0.0, -35.0, -40.0))
        light = sun.get_component_by_class(unreal.DirectionalLightComponent)
        safe_set(light, "atmosphere_sun_light", True)
    if not _exists(unreal.SkyAtmosphere):
        spawn(unreal.SkyAtmosphere, to_cm(2016, 1512, 0), "Ciel", FOLDER)
    if not _exists(unreal.SkyLight):
        sky = spawn(unreal.SkyLight, to_cm(2016, 1512, 500), "Lumière du ciel", FOLDER)
        safe_set(sky.get_component_by_class(unreal.SkyLightComponent), "real_time_capture", True)
    if not _exists(unreal.ExponentialHeightFog):
        fog = spawn(unreal.ExponentialHeightFog, to_cm(2016, 1512, 0), "Brouillard global", FOLDER)
        component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        safe_set(component, "fog_density", 0.008)
    if not _exists(unreal.VolumetricCloud):
        spawn(unreal.VolumetricCloud, to_cm(2016, 1512, 0), "Nuages", FOLDER)


# --- Eau --------------------------------------------------------------------------------------

def _water(layout_data, assets):
    """Un grand plan d'eau au niveau 0 : rivière, lac et mares (le terrain est creusé dessous)."""
    world = layout_data["world"]
    width, height = world["width_m"], world["height_m"]
    water = spawn(unreal.StaticMeshActor, to_cm(width / 2, height / 2, world["water_level_m"]),
                  "Eau — niveau 0", FOLDER, ["eau"])
    water.set_actor_scale3d(unreal.Vector(width, height, 1))  # le plan d'origine fait 1 m
    water.static_mesh_component.set_static_mesh(assets.shape_mesh("plane"))
    water.static_mesh_component.set_material(0, assets.color_material((0.16, 0.36, 0.55)))
    safe_set(water, "is_spatially_loaded", False)  # toujours chargé (il couvre tout le monde)


# --- Brumes -----------------------------------------------------------------------------------

def _mist(label, center, size, color, glow, density):
    """Volume de brume local ; sa forme suit l'échelle de l'acteur (base : sphère de 1 m)."""
    fog = spawn(unreal.LocalFogVolume, to_cm(center[0], center[1], size[2] / 2), label, FOLDER,
                ["brume"])
    fog.set_actor_scale3d(unreal.Vector(size[0] / 2, size[1] / 2, size[2] / 2))
    component = fog.get_component_by_class(unreal.LocalFogVolumeComponent)
    if component is None:
        unreal.log_warning("[Odyssée] LocalFogVolumeComponent introuvable : brume ignorée")
        return
    safe_set(component, "radial_fog_extinction", density)
    safe_set(component, "height_fog_extinction", density)
    safe_set(component, "fog_albedo", unreal.LinearColor(*color, 1))
    safe_set(component, "fog_emissive", unreal.LinearColor(color[0] * glow, color[1] * glow,
                                                            color[2] * glow, 1))


def _toxic_fog(toxic):
    x, y = toxic["center"]
    for level in toxic["levels"]:
        radius = level["radius_m"]
        _mist(f"Brouillard toxique — {level['id']}", (x, y), (radius * 2, radius * 2, 40),
              TOXIC_COLOR, 0.15, TOXIC_DENSITY[level["id"]])
