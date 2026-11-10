# Guide de style des modèles 3D (Odyssée)

Leçons tirées des premiers outils (hache et pioche, en pierre et en métal). À appliquer à chaque
nouveau modèle ; la plupart sont **déjà automatiques** via les modules de `lowpoly/`.

## 1. Partir de l'image, pas de l'œil

- Quand un concept existe : **mesurer l'image** (`references/trace.py`) plutôt que l'imiter.
  Silhouette, courbes, largeurs, positions des détails et couleurs viennent des pixels.
- Régler chaque seuil sur des **valeurs mesurées** (afficher les pixels de chaque zone) : deviner un
  seuil fait perdre plus de temps qu'il n'en fait gagner.
- Toujours vérifier l'analyse sur son **image de contrôle** (`<nom>_trace.png`) avant de modéliser.
- Sans concept (ex. outils en pierre) : reprendre les **proportions** et le style d'un objet voisin
  qui a un concept, et le signaler comme un choix de design à valider.

## 2. Forme

- **Low-poly à grandes facettes** : simplifier tout contour mesuré (Douglas-Peucker, tolérance
  ≈ 2 pixels de l'image). Jamais d'escalier hérité des pixels.
- **Rien de « tout droit »** : manches légèrement courbés, rayons irréguliers, pattes pliées.
- **Contours propres** : nettoyer les points en double et les allers-retours (`clean_outline`), sinon
  Blender perd une partie de la face.
- **Pièces cachées** : quand le dessin masque une partie (manche dans la tête), la faire passer
  plus fine que ce qui la recouvre, sinon elle transperce.
- **Proportions de la direction artistique** : trapues et lisibles (outils tenus en vue 1re personne).

## 3. Couleur et matière

- Couleurs données en **sRGB** (comme dans une image) : `flat_material` les convertit en linéaire.
  Sans ça, tout paraît délavé.
- **Palette commune** (`tool_parts.TOOL_COLORS`) : un même bois, un même tissu, un même métal pour
  toute une famille d'objets.
- **Matières distinctes au premier coup d'œil** : deux gris trop proches (pierre et métal) se
  confondent. Pierre = gris ardoise foncé et froid ; métal = gris clair et chaud, taché de rouille.
- **Deux tons + variations** (`shading.two_tone`) : un côté clair, un côté sombre, et des facettes
  légèrement différentes. C'est l'aspect « peint à la main » des planches. Pas de texture photo.
- **Détails d'usure** : arête fraîchement taillée plus claire (pierre), bout des pointes usé (métal),
  rouille en plaques irrégulières.

## 4. Bois (`lowpoly/wood.py`)

- **Tons en veines**, pas au hasard (`streaked`) : une même nuance court le long du fil.
- **Traits du fil** fins et ondulés, plaqués sur les facettes (`grain_lines`, `grain_strokes`),
  un **nœud** sur la partie nue, **bout scié** à cernes (`end_grain`).
- **Manche ovale** (`HAFT_FLATTEN`), plus large dans le sens du coup ; les bandages suivent l'ovale.
- Ces petits détails peints sont marqués `without_outline` : sinon le contour noir de 3,5 mm
  transforme un trait de 1 mm en grosse barre noire.

## 5. Finition (automatique)

- **Chanfrein** sur les arêtes vives des pièces rigides (`profile_solid`, `knapped`).
- **Contour noir** (`outline.ink_outline`, 3,5 mm) : la coque inversée dessine le trait des planches.
  Désactivable par modèle avec `OUTLINE = False`.
- **Pivot au pied**, mètres → centimètres à l'export FBX.
- **Faces orientées vers l'extérieur** (`finish.outward_normals`, à l'assemblage). Une face tournée
  vers l'intérieur disparaît dès que les faces arrière sont masquées (Unreal, contour noir) :
  la pièce devient invisible ou noire.
- Pour voir le contour correctement dans Blender en mode Solid : **Viewport Shading → Backface
  Culling**. Sans ça, l'intérieur de la coque noire recouvre le modèle (dans Unreal c'est automatique).

## 6. Vérifier avant de montrer

1. `compare.py <nom>` : concept et rendu côte à côte, sous le **même angle** (`VIEW`,
   `PREVIEW_ROTATION_DEG`).
2. Regarder **sous plusieurs angles** (face, trois quarts, profil, dos) dans Blender.
3. Vérifier par des **mesures** quand l'œil hésite (épaisseurs, hauteurs, points d'un contour).
4. Les objets d'inventaire déclarent `ICON = True` : icône transparente dans `exports/icones/`.
5. Rien n'est commité avant la validation de Nicolas.
