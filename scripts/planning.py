#!/usr/bin/env python3
"""Planning d'Odyssée : source unique = planning/tasks.tsv + planning/milestones.tsv.

Usage :
    scripts/planning.py render              # régénère docs/PLANNING.md
    scripts/planning.py github [--dry-run]  # crée labels, jalons et issues sur GitHub (idempotent)
    scripts/planning.py github --project N  # ... et ajoute les issues au GitHub Project n° N
    scripts/planning.py project N           # range toutes les issues dans le Project n° N et remplit ses champs

État d'une tâche : colonne `etat` de tasks.tsv (a_faire, en_cours, fait). `render` l'affiche,
`github` ferme ou rouvre l'issue, `project` règle la colonne Status du Project.

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
WHO = {"nicolas": "Nicolas", "ami": "Coéquipier", "both": "Les deux"}
# État d'une tâche : case dans PLANNING.md, noms possibles de l'option Status du Project.
STATES = {
    "a_faire": ("☐", ("Todo", "À faire")),
    "en_cours": ("◐", ("In Progress", "En cours")),
    "fait": ("☑", ("Done", "Fait", "Terminé")),
}

MONTHS = ["janv.", "févr.", "mars", "avr.", "mai", "juin",
          "juil.", "août", "sept.", "oct.", "nov.", "déc."]

LABEL_COLORS = {
    "doc": "0e8a16", "lore": "5319e7", "gameplay": "fbca04", "design": "d876e3", "figma": "d876e3",
    "art": "c5def5", "blender": "c5def5", "tech": "1d76db", "shared": "1d76db", "net": "0052cc",
    "server": "b60205", "client": "0075ca", "unreal": "313131", "sim": "006b75", "tests": "bfd4f2",
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
    return (f"**Semaine {t['semaine']}** (du {fmt(monday)}) · **Qui :** {WHO[t['qui']]} · "
            f"**Estimation :** {t['heures']} h\n\n"
            f"### Fini quand\n{t['fini_quand']}\n\n"
            f"_Généré depuis `planning/tasks.tsv` — modifier la source, pas l'issue._")


def progress_lines(tasks, milestones):
    """Tableau d'avancement par jalon, puis les tâches en cours."""
    lines = ["## Avancement", "", "| Jalon | Tâches faites | Heures faites | En cours |", "|---|---|---|---|"]
    for m in milestones.values():
        mine = [t for t in tasks if t["jalon"] == m["id"]]
        done = [t for t in mine if t["etat"] == "fait"]
        hours = sum(int(t["heures"]) for t in done), sum(int(t["heures"]) for t in mine)
        ongoing = ", ".join(t["id"] for t in mine if t["etat"] == "en_cours") or "—"
        lines.append(f"| {m['titre']} | {len(done)} / {len(mine)} | {hours[0]} / {hours[1]} h | {ongoing} |")
    lines += ["", "☑ fait · ◐ en cours · ☐ à faire. L'état vient de la colonne `etat` de `planning/tasks.tsv`.", ""]
    return lines


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
        "- **Équipe :** Nicolas (code, serveur, réseau) et son coéquipier (Unreal, art, cartes) — voir [TEAM.md](TEAM.md)",
        "- **Rythme :** ~12 h/semaine pour Nicolas (10–15 h), objectifs fixés semaine par semaine",
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

    lines += [""] + progress_lines(tasks, milestones)
    lines += ["## Semaine par semaine", ""]
    for w in sorted(by_week):
        week_tasks = by_week[w]
        monday = week_monday(w)
        def h(who):
            return sum(int(t["heures"]) for t in week_tasks if t["qui"] in (who, "both"))
        jalons = sorted({t["jalon"] for t in week_tasks}, key=lambda j: list(milestones).index(j))
        lines += [
            f"### Semaine {w} — du {fmt(monday)} · {', '.join(jalons)} · Nicolas {h('nicolas')} h · Coéquipier {h('ami')} h",
            "",
            "| | ID | Qui | Tâche | h | Fini quand |",
            "|---|---|---|---|---|---|",
        ]
        for t in week_tasks:
            lines.append(f"| {STATES[t['etat']][0]} | {t['id']} | {WHO[t['qui']]} | {t['titre']} | {t['heures']} | {t['fini_quand']} |")
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


def sync_issue_state(issue, task, dry_run):
    """Issue fermée si la tâche est faite, ouverte sinon."""
    closed = issue.get("state", "").upper() == "CLOSED"
    if task["etat"] == "fait" and not closed:
        gh("issue", "close", str(issue["number"]), "--repo", REPO, "--reason", "completed",
           dry_run=dry_run, capture=True)
        print(f"{task['id']} : issue fermée (fait)")
    elif task["etat"] != "fait" and closed:
        gh("issue", "reopen", str(issue["number"]), "--repo", REPO, dry_run=dry_run, capture=True)
        print(f"{task['id']} : issue rouverte ({task['etat']})")


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
    existing_issues = {} if dry_run else {
        i["title"].split(" · ")[0]: i
        for i in json.loads(gh("issue", "list", "--repo", REPO, "--state", "all", "--limit", "1000",
                               "--json", "number,title,body,labels,milestone,state", capture=True))
    }
    for t in tasks:
        current = existing_issues.get(t["id"])
        if current:
            sync_issue_state(current, t, dry_run)
            wanted = set(t["labels"].split(","))
            have = {l["name"] for l in current["labels"]}
            ms = (current.get("milestone") or {}).get("title")
            if (current["title"], current["body"], have, ms) == (
                    issue_title(t), issue_body(t), wanted, ms_titles[t["jalon"]]):
                continue
            args = ["issue", "edit", str(current["number"]), "--repo", REPO, "--title", issue_title(t),
                    "--body", issue_body(t), "--milestone", ms_titles[t["jalon"]]]
            if wanted - have:
                args += ["--add-label", ",".join(sorted(wanted - have))]
            if have - wanted:
                args += ["--remove-label", ",".join(sorted(have - wanted))]
            gh(*args, dry_run=dry_run, capture=True)
            print(f"{t['id']} mise à jour")
            continue
        url = gh("issue", "create", "--repo", REPO, "--title", issue_title(t), "--body", issue_body(t),
                 "--label", t["labels"], "--milestone", ms_titles[t["jalon"]],
                 dry_run=dry_run, capture=True).strip()
        print(f"{t['id']} créée {url}")
        if project and url:
            gh("project", "item-add", str(project), "--owner", REPO.split("/")[0], "--url", url,
               dry_run=dry_run)


PROJECT_FIELDS = {  # nom -> type
    "Semaine": "NUMBER",
    "Estimation (h)": "NUMBER",
    "Début": "DATE",
    "Fin": "DATE",
    "Jalon": "TEXT",
    "Qui": "SINGLE_SELECT",
}


def project(number, dry_run):
    """Ajoute toutes les issues au GitHub Project et remplit ses champs (idempotent)."""
    owner = REPO.split("/")[0]
    tasks = {t["id"]: t for t in read_tsv(TASKS)}

    def gh_json(*args):
        return json.loads(gh(*args, "--format", "json", capture=True))

    project_id = next(p["id"] for p in gh_json("project", "list", "--owner", owner)["projects"]
                      if p["number"] == number)

    fields = {f["name"]: f for f in gh_json("project", "field-list", str(number), "--owner", owner,
                                            "--limit", "100")["fields"]}
    for name, kind in PROJECT_FIELDS.items():
        if name not in fields:
            extra = ["--single-select-options", ",".join(WHO.values())] if kind == "SINGLE_SELECT" else []
            gh("project", "field-create", str(number), "--owner", owner, "--name", name,
               "--data-type", kind, *extra, dry_run=dry_run)
    fields = {f["name"]: f for f in gh_json("project", "field-list", str(number), "--owner", owner,
                                            "--limit", "100")["fields"]}
    status = fields.get("Status")
    status_options = {o["name"]: o["id"] for o in (status or {}).get("options", [])}
    status_of = {state: next((status_options[n] for n in names if n in status_options), None)
                 for state, (_, names) in STATES.items()}

    items = {i["content"].get("url"): i for i in gh_json("project", "item-list", str(number), "--owner",
                                                         owner, "--limit", "1000")["items"]}
    issues = json.loads(gh("issue", "list", "--repo", REPO, "--state", "all", "--limit", "1000",
                           "--json", "title,url", capture=True))

    for issue in sorted(issues, key=lambda i: i["title"]):
        tid = issue["title"].split(" · ")[0]
        t = tasks.get(tid)
        if t is None:
            continue
        item = items.get(issue["url"])
        if item is None:
            item = json.loads(gh("project", "item-add", str(number), "--owner", owner, "--url",
                                 issue["url"], "--format", "json", capture=True))
        monday = week_monday(int(t["semaine"]))
        values = {
            "Semaine": ("--number", t["semaine"]),
            "Estimation (h)": ("--number", t["heures"]),
            "Début": ("--date", monday.isoformat()),
            "Fin": ("--date", (monday + dt.timedelta(days=6)).isoformat()),
            "Jalon": ("--text", t["jalon"]),
        }
        for name, (flag, value) in values.items():
            gh("project", "item-edit", "--project-id", project_id, "--id", item["id"],
               "--field-id", fields[name]["id"], flag, value, dry_run=dry_run, capture=True)
        who = next(o["id"] for o in fields["Qui"]["options"] if o["name"] == WHO[t["qui"]])
        gh("project", "item-edit", "--project-id", project_id, "--id", item["id"],
           "--field-id", fields["Qui"]["id"], "--single-select-option-id", who,
           dry_run=dry_run, capture=True)
        wanted = status_of[t["etat"]]
        if wanted:  # l'état vient du TSV : la colonne Status du Project le suit
            gh("project", "item-edit", "--project-id", project_id, "--id", item["id"],
               "--field-id", status["id"], "--single-select-option-id", wanted,
               dry_run=dry_run, capture=True)
        print(f"{tid} : dans le projet ({t['etat']})")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("render")
    p = sub.add_parser("github")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--project", type=int, help="numéro du GitHub Project où ajouter les issues")
    pp = sub.add_parser("project")
    pp.add_argument("number", type=int, help="numéro du GitHub Project")
    pp.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    if args.cmd == "render":
        render()
    elif args.cmd == "project":
        project(args.number, args.dry_run)
    else:
        try:
            github(args.dry_run, args.project)
        except FileNotFoundError:
            sys.exit("gh n'est pas installé : sudo apt install gh (Linux) ou "
                     "winget install --id GitHub.cli (Windows), puis gh auth login")


if __name__ == "__main__":
    main()
