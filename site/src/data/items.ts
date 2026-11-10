// Objets et artisanat, version publique (source : docs/VISION.md, art/README.md).
// Les recettes exactes ne sont pas fixées : on décrit l'esprit (matériaux, paliers), pas des chiffres.
import type { ImageMetadata } from "astro";

const modules = import.meta.glob<{ default: ImageMetadata }>("../assets/items/*.webp", { eager: true });

/** Icône d'inventaire d'un objet (nom de fichier sans extension). */
export function icone(nom: string): ImageMetadata {
  const module = modules[`../assets/items/${nom}.webp`];
  if (!module) throw new Error(`Icône introuvable : ${nom}`);
  return module.default;
}

export interface Objet {
  id: string;
  nom: string;
  texte: string;
}

export interface Palier {
  id: string;
  numero: string;
  titre: string;
  accroche: string;
  materiaux: string[];
  objets: Objet[];
}

export const ressources: Objet[] = [
  { id: "buche", nom: "Bûche", texte: "Le premier geste : frapper un arbre. Manches, feux, établis, tout commence par le bois." },
  { id: "pierre", nom: "Pierre", texte: "Un caillou bien tenu fait une tête de hache. Elle s'use, mais elle sauve une première nuit." },
  { id: "minerai", nom: "Minerai", texte: "Veines rouillées au creux des roches. Il faudra une pioche solide pour les atteindre." },
];

export const paliers: Palier[] = [
  {
    id: "pierre",
    numero: "I",
    titre: "Un bâton et un caillou",
    accroche: "On se réveille les mains vides. Il suffit pourtant de très peu pour commencer.",
    materiaux: ["Bois", "Pierre"],
    objets: [
      { id: "hache_pierre", nom: "Hache de pierre", texte: "Un manche, un caillou, du lien. Elle abat les arbres sans élégance, mais elle les abat." },
      { id: "pioche_pierre", nom: "Pioche de pierre", texte: "De quoi casser la roche et ramasser le minerai qui dépasse." },
    ],
  },
  {
    id: "metal",
    numero: "II",
    titre: "Le métal",
    accroche: "Le minerai change tout : des outils qui durent, qui vont plus vite, qui ouvrent de nouveaux endroits.",
    materiaux: ["Bois", "Minerai"],
    objets: [
      { id: "hache_metal", nom: "Hache de métal", texte: "Plus rapide, plus solide. Elle tranche aussi bien le bois que ce qui rôde la nuit." },
      { id: "pioche_metal", nom: "Pioche de métal", texte: "Elle perce la roche dure et rapporte de quoi aller encore plus loin." },
    ],
  },
];

export const armes: Objet[] = [
  { id: "lance", nom: "Lance", texte: "Simple, silencieuse, fiable. L'arme de ceux qui n'ont rien d'autre." },
  { id: "arc", nom: "Arc", texte: "Pour chasser sans s'approcher." },
  { id: "arbalete", nom: "Arbalète", texte: "Lente à armer, rude à l'arrivée. Elle partage ses flèches avec l'arc." },
  { id: "fleche", nom: "Flèche", texte: "Une seule munition pour l'arc et l'arbalète." },
  { id: "pistolet", nom: "Pistolet", texte: "Une relique d'avant. Les munitions sont rares." },
  { id: "fusil_pompe", nom: "Fusil à pompe", texte: "Du bruit, de la puissance, et peu de coups." },
];

export const equipement: Objet[] = [
  { id: "lampe", nom: "Lampe", texte: "Les ruines sont sombres, les nuits aussi." },
  { id: "pile", nom: "Pile", texte: "Récupérée dans les vestiges, elle fait vivre la lampe et le GPS." },
  { id: "gps", nom: "GPS", texte: "Un vieil appareil qui fonctionne encore, par endroits." },
  { id: "jumelles", nom: "Jumelles", texte: "Repérer avant d'avancer : ce qu'on voit de loin évite des ennuis." },
  { id: "pied_de_biche", nom: "Pied-de-biche", texte: "Pour forcer ce que les anciens ont fermé." },
];
