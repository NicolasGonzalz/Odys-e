#!/usr/bin/env python3
"""Sources de shared/ compilées dans le plugin Unreal OdysseeNet (ADR 0003, décision 4).

Unreal ne compile que les .cpp situés dans le dossier d'un module. Pour chaque fichier de
shared/src/, le plugin a donc une petite « enveloppe » (Private/Shared/Shared<Nom>.cpp) qui inclut
le fichier d'origine : le code n'est jamais copié, il n'existe qu'à un seul endroit.

Usage (depuis la racine du dépôt) :
    scripts/odysseenet_sources.py           # vérifie (utilisé par la CI) : code 1 si décalage
    scripts/odysseenet_sources.py --write   # régénère les enveloppes après un ajout dans shared/
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SHARED_SRC = ROOT / "shared" / "src"
WRAPPERS = ROOT / "UnrealEngine/Odysse/Plugins/OdysseeNet/Source/OdysseeNet/Private/Shared"

# Code de plateforme : seulement compilé sur sa plateforme (macros définies par Unreal).
PLATFORM_GUARDS = {"posix": "PLATFORM_LINUX || PLATFORM_MAC", "win32": "PLATFORM_WINDOWS"}

HEADER = """// Généré par scripts/odysseenet_sources.py : ne pas modifier à la main.
// Compile {source} dans le module OdysseeNet (voir OdysseeNet.Build.cs).
"""


def wrapper_name(source):
    return f"Shared{source.stem}.cpp"


def wrapper_text(source):
    relative = source.relative_to(ROOT / "shared").as_posix()
    platform = next((p for p in PLATFORM_GUARDS if p in source.parts), None)
    lines = [HEADER.format(source=f"shared/{relative}"), '#include "HAL/Platform.h"', ""]
    if platform:
        lines.append(f"#if {PLATFORM_GUARDS[platform]}")
    if platform == "win32":  # en-têtes Windows : à encadrer pour cohabiter avec ceux d'Unreal
        lines.append('#include "Windows/AllowWindowsPlatformTypes.h"')
    lines += ["THIRD_PARTY_INCLUDES_START", f'#include "{relative}"', "THIRD_PARTY_INCLUDES_END"]
    if platform == "win32":
        lines.append('#include "Windows/HideWindowsPlatformTypes.h"')
    if platform:
        lines.append("#endif")
    return "\n".join(lines) + "\n"


def expected():
    """{nom de l'enveloppe: contenu attendu} pour tous les .cpp de shared/src/."""
    sources = sorted(SHARED_SRC.rglob("*.cpp"))
    names = [wrapper_name(s) for s in sources]
    duplicates = {n for n in names if names.count(n) > 1}
    if duplicates:
        raise SystemExit(f"Noms de fichiers en double dans shared/src : {sorted(duplicates)}")
    return {wrapper_name(s): wrapper_text(s) for s in sources}


def check(wanted):
    present = {p.name: p.read_text(encoding="utf-8") for p in WRAPPERS.glob("*.cpp")}
    missing = sorted(set(wanted) - set(present))
    extra = sorted(set(present) - set(wanted))
    stale = sorted(n for n in wanted if n in present and present[n] != wanted[n])
    for label, names in (("manquante", missing), ("en trop", extra), ("pas à jour", stale)):
        for name in names:
            print(f"Enveloppe {label} : {name}")
    if missing or extra or stale:
        print("→ lancer : scripts/odysseenet_sources.py --write")
        return 1
    print(f"OdysseeNet : {len(wanted)} sources de shared/ à jour.")
    return 0


def write(wanted):
    WRAPPERS.mkdir(parents=True, exist_ok=True)
    for old in WRAPPERS.glob("*.cpp"):
        if old.name not in wanted:
            old.unlink()
    for name, text in wanted.items():
        # newline : fins de ligne LF même sous Windows (comme le reste du dépôt).
        with (WRAPPERS / name).open("w", encoding="utf-8", newline="\n") as file:
            file.write(text)
    print(f"OdysseeNet : {len(wanted)} enveloppes écrites dans {WRAPPERS.relative_to(ROOT)}.")
    return 0


if __name__ == "__main__":
    sys.exit(write(expected()) if "--write" in sys.argv else check(expected()))
