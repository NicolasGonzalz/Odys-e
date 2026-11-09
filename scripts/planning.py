#!/usr/bin/env python3
"""Planning d'Odyssée : source unique = planning/tasks.tsv + planning/milestones.tsv.

Usage :
    scripts/planning.py render              # régénère docs/PLANNING.md
    scripts/planning.py github [--dry-run]  # crée labels, jalons et issues sur GitHub (idempotent)
    scripts/planning.py github --project N  # ... et ajoute les issues au GitHub Project n° N

Pré-requis pour `github` : gh installé et authentifié (`gh auth login`).
Pour `--project` : `gh auth refresh -s project`.
"""
import argparse
import csv
import datetime as dt
import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TASKS = ROOT / "planning" / "tasks.tsv"
MILESTONES = ROOT / "planning" / "milestones.tsv"
OUTPUT = ROOT / "docs" / "PLANNING.md"
START = dt.date(2026, 11, 16)  # lundi de la semaine 1
REPO = "NicolasGonzalz/Odys-e"

MONTHS = ["janv.", "févr.", "mars", "avr.", "mai", "juin",
          "juil.", "août", "sept.", "oct.", "nov.", "déc."]

LABEL_COLORS = {
    "doc": "0e8a16", "lore": "5319e7", "gameplay": "fbca04", "design": "d876e3", "figma": "d876e3",
    "art": "c5def5", "blender": "c5def5", "tech": "1d76db", "shared": "1d76db", "net": "0052cc",
    "server": "b60205", "client": "0075ca", "godot": "478cbf", "sim": "006b75", "tests": "bfd4f2",
    "infra": "6f42c1", "homelab": "6f42c1", "tools": "e99695", "map": "7057ff", "world": "7057ff",
    "data": "c2e0c6", "ui": "f9d0c4", "devlog": "ededed",
}


def read_tsv(path):
    with path.open(encoding="utf-8") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def week_monday(n):
    return START + dt.timedelta(weeks=n - 1)


def fmt(d):
    return f"{d.day} {MONTHS[d.month - 1]} {d.year}"


def issue_title(t):
    return f"{t['id']} · {t['titre']}"


def issue_body(t):
    monday = week_monday(int(t["semaine"]))
    return (f"**Semaine {t['semaine']}** (du {fmt(monday)}) · **Estimation :** {t['heures']} h\n\n"
            f"### Fini quand\n{t['fini_quand']}\n\n"
            f"_Généré depuis `planning/tasks.tsv` — modifier la source, pas l'issue._")


def render():
    tasks = read_tsv(TASKS)
    milestones = {m["id"]: m for m in read_tsv(MILESTONES)}
    by_week = defaultdict(list)
    for t in tasks:
        by_week[int(t["semaine"])].append(t)

    total = sum(int(t["heures"]) for t in tasks)
    last_week = max(by_week)
    lines = [
        "# Planning",
        "",
        "> Fichier généré par `scripts/planning.py render` depuis `planning/tasks.tsv` et `planning/milestones.tsv`.",
        "> Pour modifier le planning : éditer les TSV, puis relancer le script.",
        "",
        f"- **Début :** lundi {fmt(START)} · **Fin prévue du MVP :** semaine {last_week} "
        f"({fmt(week_monday(last_week) + dt.timedelta(days=6))})",
        f"- **Rythme :** ~12 h/semaine (10–15 h), objectifs fixés semaine par semaine",
        f"- **Charge totale :** {total} h, {len(tasks)} tâches",
        "- **Suivi au quotidien :** GitHub Issues + Project (une issue par tâche)",
        "",
        "## Règles du jeu",
        "",
        "1. **Lundi** : on relit l'objectif de la semaine et on passe ses issues en « En cours ».",
        "2. **Une tâche = une branche = une PR**, nommée d'après l'ID (`t021-paquet-header`).",
        "3. **Dimanche** : bilan rapide. Ce qui n'est pas fini glisse à la semaine suivante, et on décale le planning "
        "plutôt que d'empiler.",
        "4. **Fin de jalon** : démo + post d'avancement public.",
        "5. Une tâche qui dépasse 2× son estimation est redécoupée.",
        "",
        "## Jalons",
        "",
        "| Jalon | Échéance | Objectif |",
        "|---|---|---|",
    ]
    for m in milestones.values():
        d = dt.date.fromisoformat(m["echeance"])
        lines.append(f"| **{m['titre']}** | {fmt(d)} | {m['description']} |")

    lines += ["", "## Semaine par semaine", ""]
    for w in sorted(by_week):
        week_tasks = by_week[w]
        monday = week_monday(w)
        hours = sum(int(t["heures"]) for t in week_tasks)
        jalons = sorted({t["jalon"] for t in week_tasks}, key=lambda j: list(milestones).index(j))
        lines += [
            f"### Semaine {w} — du {fmt(monday)} · {hours} h · {', '.join(jalons)}",
            "",
            "| | ID | Tâche | h | Fini quand |",
            "|---|---|---|---|---|",
        ]
        for t in week_tasks:
            lines.append(f"| ☐ | {t['id']} | {t['titre']} | {t['heures']} | {t['fini_quand']} |")
        lines.append("")

    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"{OUTPUT.relative_to(ROOT)} généré ({len(tasks)} tâches, {total} h).")


def gh(*args, dry_run=False, capture=False):
    cmd = ["gh", *args]
    if dry_run:
        print("DRY-RUN:", " ".join(cmd))
        return ""
    res = subprocess.run(cmd, check=True, text=True, capture_output=capture)
    return res.stdout if capture else ""


def github(dry_run, project):
    tasks = read_tsv(TASKS)
    milestones = read_tsv(MILESTONES)

    labels = sorted({l for t in tasks for l in t["labels"].split(",")} | {"jalon"})
    for label in labels:
        gh("label", "create", label, "--repo", REPO, "--force",
           "--color", LABEL_COLORS.get(label, "ededed"), dry_run=dry_run)

    existing_ms = {} if dry_run else {
        m["title"]: m["number"]
        for m in json.loads(gh("api", f"repos/{REPO}/milestones?state=all&per_page=100", capture=True))
    }
    for m in milestones:
        if m["titre"] in existing_ms:
            continue
        gh("api", f"repos/{REPO}/milestones", "-f", f"title={m['titre']}",
           "-f", f"description={m['description']}", "-f", f"due_on={m['echeance']}T23:59:59Z",
           dry_run=dry_run)

    ms_titles = {m["id"]: m["titre"] for m in milestones}
    existing_issues = set() if dry_run else {
        i["title"].split(" · ")[0]
        for i in json.loads(gh("issue", "list", "--repo", REPO, "--state", "all", "--limit", "1000",
                               "--json", "title", capture=True))
    }
    for t in tasks:
        if t["id"] in existing_issues:
            print(f"{t['id']} existe déjà, ignorée.")
            continue
        url = gh("issue", "create", "--repo", REPO, "--title", issue_title(t), "--body", issue_body(t),
                 "--label", t["labels"], "--milestone", ms_titles[t["jalon"]],
                 dry_run=dry_run, capture=True).strip()
        print(f"{t['id']} créée {url}")
        if project and url:
            gh("project", "item-add", str(project), "--owner", REPO.split("/")[0], "--url", url,
               dry_run=dry_run)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("render")
    p = sub.add_parser("github")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--project", type=int, help="numéro du GitHub Project où ajouter les issues")
    args = parser.parse_args()

    if args.cmd == "render":
        render()
    else:
        try:
            github(args.dry_run, args.project)
        except FileNotFoundError:
            sys.exit("gh n'est pas installé : sudo apt install gh, puis gh auth login")


if __name__ == "__main__":
    main()
