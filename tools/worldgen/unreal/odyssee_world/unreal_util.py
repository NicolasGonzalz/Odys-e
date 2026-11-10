"""Petits outils communs aux modules Unreal : création d'acteurs, étiquettes, réglages prudents."""
import unreal

GENERATED_TAG = "odyssee_generated"  # tout ce que le script pose ; supprimé à chaque relance


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def to_cm(x, y, z):
    """Mètres du plan du monde → centimètres Unreal (X = est, Y = sud, Z = haut)."""
    return unreal.Vector(x * 100.0, y * 100.0, z * 100.0)


def spawn(actor_class, location, label, folder, tags=(), rotation=None):
    """Crée un acteur, lui donne un nom lisible, un dossier et l'étiquette « généré »."""
    actor = actors().spawn_actor_from_class(actor_class, location, rotation or unreal.Rotator())
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.set_editor_property("tags", [GENERATED_TAG, *tags])
    return actor


def safe_set(target, name, value):
    """Règle une propriété ; si elle n'existe pas dans cette version, prévient et continue."""
    try:
        target.set_editor_property(name, value)
        return True
    except Exception as error:  # noqa: BLE001 — on veut continuer quoi qu'il arrive
        unreal.log_warning(f"[Odyssée] réglage « {name} » ignoré sur {target.get_name()} : {error}")
        return False


def clear_generated():
    """Supprime ce qu'une génération précédente a posé (le reste du niveau n'est pas touché)."""
    generated = [a for a in actors().get_all_level_actors() if a.actor_has_tag(GENERATED_TAG)]
    if generated:
        actors().destroy_actors(generated)
    return len(generated)
