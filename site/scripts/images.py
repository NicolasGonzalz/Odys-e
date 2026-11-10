#!/usr/bin/env python3
"""Images du site, tirées des concepts de art/ : recadrées, réduites et converties en WebP.

Les originaux (art/concepts/, Git LFS, plusieurs Mo chacun) restent la référence. Le site garde
des versions web légères dans site/src/assets/, versionnées hors LFS (voir .gitattributes) pour
que Vercel les trouve sans Git LFS.

Usage (depuis la racine du dépôt) : python3 site/scripts/images.py
"""
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
CONCEPTS = ROOT / "art" / "concepts"
ASSETS = ROOT / "site" / "src" / "assets"
PUBLIC = ROOT / "site" / "public"
OG_SIZE = (1200, 630)  # aperçu affiché quand on partage le lien (réseaux, messageries)
MAX_SIDE = 1600
QUALITY = 80

# nom → (image source dans art/concepts, recadrage [gauche, haut, droite, bas] ou None)
IMAGES = {
    "zone-1": ("ambiances/zone-1-plaine.jpg", None),
    "zone-2": ("ambiances/zone-2-ville-engloutie.jpg", None),
    "zone-3": ("ambiances/zone-3-terres-noyees.jpg", None),
    "zone-4": ("ambiances/zone-4-site-des-portes-lieux.jpeg", (1834, 0, 2752, 768)),  # le lac gelé : sans les gardiens
    "zone-5": ("ambiances/zones-1-a-5-planche-b.jpeg", (1376, 768, 2752, 1536)),
    # Planches de face / trois quarts / dos : on garde la vue de trois quarts, au milieu.
    "mara": ("personnages/mara-velin.jpg", (1134, 0, 1934, 1536)),
    "nadia": ("personnages/nadia-okoro.jpg", (1050, 0, 1850, 1536)),
    # Personnages en pied : même format portrait que les deux autres.
    "ilian": ("personnages/ilian-sorel.jpg", (271, 0, 1521, 2400)),
    "teo": ("personnages/teo-brandt.jpg", (271, 0, 1521, 2400)),
}


# Découpes sans les titres des planches (qui nomment les factions : spoiler).
CREATURES = {
    "chien": ("creatures/bestiaire.jpg", (70, 80, 540, 540)),
    "sanglier": ("creatures/bestiaire.jpg", (640, 110, 1090, 520)),
    "loup": ("creatures/bestiaire.jpg", (75, 605, 695, 1050)),
    "insecte": ("creatures/bestiaire.jpg", (735, 680, 1040, 1030)),
    "plante": ("creatures/bestiaire.jpg", (1100, 545, 1730, 1070)),
    "drone": ("creatures/bestiaire.jpg", (1870, 130, 2210, 500)),
    "robot": ("creatures/bestiaire.jpg", (1730, 630, 2190, 1165)),
    "colosse": ("creatures/bestiaire.jpg", (2265, 510, 2695, 1045)),
}
DECORS = {
    "etabli": ("objets/objets-interactifs.jpg", (1900, 75, 2670, 650)),
    "feu": ("objets/objets-interactifs.jpg", (695, 535, 1260, 940)),
    "coffre": ("objets/objets-interactifs.jpg", (115, 65, 825, 640)),
    "ruines": ("decors/vegetation-ruines-campement.jpg", None),
    "monde-sauvage": ("ambiances/zones-1-3-5-lieux.jpeg", None),
}
ICONS = ["baton", "planches", "lingot", "munitions_pistolet", "munitions_fusil", "hache_pierre", "pioche_pierre", "hache_metal", "pioche_metal", "buche", "pierre", "minerai",
         "arc", "fleche", "lance", "arbalete", "pistolet", "fusil_pompe", "lampe", "pile", "gps",
         "jumelles", "pied_de_biche"]
EXPORTS = ROOT / "art" / "exports" / "icones"


def export_icons():
    """Icônes d'inventaire (PNG transparents 256 px) → WebP dans src/assets/items/."""
    out = ASSETS / "items"
    out.mkdir(parents=True, exist_ok=True)
    for name in ICONS:
        Image.open(EXPORTS / f"{name}.png").convert("RGBA").save(out / f"{name}.webp", "WEBP", quality=88, method=6)
    print(f"{len(ICONS)} icônes → {out.relative_to(ROOT)}")


def export(name, source, box):
    image = Image.open(CONCEPTS / source).convert("RGB")
    if box:
        image = image.crop(box)
    image.thumbnail((MAX_SIDE, MAX_SIDE), Image.LANCZOS)
    path = ASSETS / f"{name}.webp"
    image.save(path, "WEBP", quality=QUALITY, method=6)
    return path, image.size


def export_og():
    """Les cinq zones, recadrées au format 1200 × 630 des aperçus de liens."""
    image = Image.open(CONCEPTS / "ambiances/zones-1-a-5-planche-b.jpeg").convert("RGB")
    ratio = OG_SIZE[0] / OG_SIZE[1]
    height = round(image.width / ratio)
    top = (image.height - height) // 2
    image = image.crop((0, top, image.width, top + height)).resize(OG_SIZE, Image.LANCZOS)
    path = PUBLIC / "og.jpg"
    image.save(path, "JPEG", quality=82, optimize=True, progressive=True)
    return path, image.size


def main():
    ASSETS.mkdir(parents=True, exist_ok=True)
    PUBLIC.mkdir(parents=True, exist_ok=True)
    export_icons()
    results = [export(name, source, box) for name, (source, box) in {**IMAGES, **CREATURES, **DECORS}.items()]
    for path, size in results + [export_og()]:
        print(f"{path.relative_to(ROOT)} : {size[0]}×{size[1]}, {path.stat().st_size // 1024} Ko")


if __name__ == "__main__":
    main()
