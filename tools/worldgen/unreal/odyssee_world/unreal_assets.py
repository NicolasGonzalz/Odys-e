"""Modèles et matériaux : le vrai modèle d'un élément s'il existe, sinon des formes colorées."""
import unreal

GENERATED_DIR = "/Game/Odyssee/Generated"
MATERIALS_DIR = f"{GENERATED_DIR}/Materials"
BASE_MATERIAL_NAME = "M_OdysseeProvisoire"
LANDSCAPE_MATERIAL_NAME = "M_OdysseeSol"

SHAPES = {
    "cube": "/Engine/BasicShapes/Cube",
    "sphere": "/Engine/BasicShapes/Sphere",
    "cylinder": "/Engine/BasicShapes/Cylinder",
    "cone": "/Engine/BasicShapes/Cone",
    "plane": "/Engine/BasicShapes/Plane",
}


class AssetLibrary:
    """Fournit les modèles et crée (une seule fois) les matériaux colorés des formes provisoires."""

    def __init__(self):
        self._tools = unreal.AssetToolsHelpers.get_asset_tools()
        self._materials = {}
        self._base = self._base_material()

    # --- Modèles ------------------------------------------------------------------------------

    def shape_mesh(self, shape):
        return unreal.EditorAssetLibrary.load_asset(SHAPES[shape])

    def real_mesh(self, path):
        """Le vrai modèle d'un élément, ou None s'il n'existe pas (encore) dans le projet."""
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.log_warning(f"[Odyssée] modèle introuvable : {path} (forme provisoire utilisée)")
            return None
        return unreal.EditorAssetLibrary.load_asset(path)

    # --- Matériaux ----------------------------------------------------------------------------

    def color_material(self, color, emissive=0.0):
        """Matériau d'une couleur donnée (créé au premier besoin, puis réutilisé)."""
        key = (tuple(round(c, 3) for c in color), round(emissive, 2))
        if key not in self._materials:
            self._materials[key] = self._create_instance(key[0], key[1])
        return self._materials[key]

    def landscape_material(self, color):
        """Matériau du Landscape : couleur unie, et trous possibles (couche de visibilité, utilisée
        pour les entrées de grotte). Créé une seule fois ; la couleur est celle du premier appel."""
        path = f"{GENERATED_DIR}/{LANDSCAPE_MATERIAL_NAME}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.EditorAssetLibrary.load_asset(path)

        material = self._tools.create_asset(LANDSCAPE_MATERIAL_NAME, GENERATED_DIR, unreal.Material,
                                            unreal.MaterialFactoryNew())
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        library = unreal.MaterialEditingLibrary
        base = library.create_material_expression(
            material, unreal.MaterialExpressionConstant3Vector, -400, 0)
        base.set_editor_property("constant", unreal.LinearColor(*color, 1))
        library.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
        holes = library.create_material_expression(
            material, unreal.MaterialExpressionLandscapeVisibilityMask, -400, 250)
        library.connect_material_property(holes, "", unreal.MaterialProperty.MP_OPACITY_MASK)
        library.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        return material

    def save(self):
        unreal.EditorAssetLibrary.save_directory(GENERATED_DIR, only_if_is_dirty=True)

    def _base_material(self):
        """Matériau parent : une couleur (« Couleur ») et une lueur (« Emission ») réglables."""
        path = f"{GENERATED_DIR}/{BASE_MATERIAL_NAME}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.EditorAssetLibrary.load_asset(path)

        material = self._tools.create_asset(BASE_MATERIAL_NAME, GENERATED_DIR, unreal.Material,
                                            unreal.MaterialFactoryNew())
        self._add_vector_parameter(material, "Couleur", unreal.MaterialProperty.MP_BASE_COLOR, 0)
        self._add_vector_parameter(material, "Emission", unreal.MaterialProperty.MP_EMISSIVE_COLOR,
                                   250)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        return material

    @staticmethod
    def _add_vector_parameter(material, name, material_property, y):
        library = unreal.MaterialEditingLibrary
        node = library.create_material_expression(
            material, unreal.MaterialExpressionVectorParameter, -400, y)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 1))
        library.connect_material_property(node, "", material_property)

    def _create_instance(self, color, emissive):
        r, g, b = color
        name = f"MI_{int(r * 255):02X}{int(g * 255):02X}{int(b * 255):02X}_E{int(emissive * 10)}"
        path = f"{MATERIALS_DIR}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return unreal.EditorAssetLibrary.load_asset(path)

        instance = self._tools.create_asset(name, MATERIALS_DIR, unreal.MaterialInstanceConstant,
                                            unreal.MaterialInstanceConstantFactoryNew())
        library = unreal.MaterialEditingLibrary
        library.set_material_instance_parent(instance, self._base)
        library.set_material_instance_vector_parameter_value(instance, "Couleur",
                                                             unreal.LinearColor(r, g, b, 1))
        library.set_material_instance_vector_parameter_value(
            instance, "Emission", unreal.LinearColor(r * emissive, g * emissive, b * emissive, 1))
        library.update_material_instance(instance)
        return instance
