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

/** Ce qu'on tire des matières premières, étape par étape. */
export interface Etape {
  id: string;
  nom: string;
}
export interface Chaine {
  titre: string;
  etapes: Etape[];
  texte: string;
}

export const transformes: Objet[] = [
  { id: "planches", nom: "Planches", texte: "Une bûche, quelques coups, et le bois devient matériau : établis, abris, manches." },
  { id: "baton", nom: "Bâton", texte: "Taillé dans une planche. Il sert de manche à presque tous les outils." },
  { id: "lingot", nom: "Lingot de métal", texte: "Le minerai fondu à la forge. Du métal enfin utilisable, prêt à devenir outil." },
];

export const chaines: Chaine[] = [
  {
    titre: "Le bois",
    etapes: [
      { id: "buche", nom: "Bûche" },
      { id: "planches", nom: "Planches" },
      { id: "baton", nom: "Bâton" },
    ],
    texte: "On abat, on débite, on taille. Chaque étape rapproche l'arbre de l'outil.",
  },
  {
    titre: "Le métal",
    etapes: [
      { id: "minerai", nom: "Minerai" },
      { id: "lingot", nom: "Lingot" },
    ],
    texte: "Du minerai brut à la barre de métal : la fonte change ce qu'on peut faire.",
  },
];

/** Où l'on fabrique : trois niveaux, chacun ouvre le suivant. */
export interface Poste {
  numero: string;
  nom: string;
  role: string;
  exemples: string[];
}

export const postes: Poste[] = [
  {
    numero: "1",
    nom: "À la main",
    role: "Partout, sans rien. Le strict nécessaire pour passer la première nuit.",
    exemples: ["Bâton", "Hache et pioche de pierre", "Lance", "Torche", "Établi"],
  },
  {
    numero: "2",
    nom: "À l'établi",
    role: "On travaille le bois et on assemble. L'établi se pose dans le monde et sert à tous.",
    exemples: ["Planches", "Arc et flèches", "Arbalète", "Coffres", "Table de cartographie", "Forge"],
  },
  {
    numero: "3",
    nom: "À la forge",
    role: "On fond le métal et on le travaille. Les outils changent de catégorie.",
    exemples: ["Lingots", "Hache et pioche de métal", "Pièces d'armes à feu"],
  },
];

export const paliers: Palier[] = [
  {
    id: "pierre",
    numero: "I",
    titre: "Un bâton et un caillou",
    accroche: "On se réveille les mains vides. Il suffit pourtant de très peu pour commencer.",
    materiaux: ["Bâton", "Pierre"],
    objets: [
      { id: "hache_pierre", nom: "Hache de pierre", texte: "Un bâton, un caillou, du lien. Elle abat les arbres sans élégance, mais elle les abat." },
      { id: "pioche_pierre", nom: "Pioche de pierre", texte: "De quoi casser la roche et ramasser le minerai qui dépasse." },
    ],
  },
  {
    id: "metal",
    numero: "II",
    titre: "Le métal",
    accroche: "Le minerai change tout : des outils qui durent, qui vont plus vite, qui ouvrent de nouveaux endroits.",
    materiaux: ["Bâton", "Lingot de métal"],
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
  { id: "pistolet", nom: "Pistolet", texte: "Une relique d'avant. Ses balles sont rares : chaque tir compte." },
  { id: "fusil_pompe", nom: "Fusil à pompe", texte: "Du bruit, de la puissance, et peu de cartouches." },
];

export const munitions: Objet[] = [
  { id: "fleche", nom: "Flèches", texte: "La même pour l'arc et l'arbalète." },
  { id: "munitions_pistolet", nom: "Balles de pistolet", texte: "De petits tas de cartouches retrouvés dans les ruines. Elles sont rares." },
  { id: "munitions_fusil", nom: "Cartouches de fusil", texte: "Lourdes, bruyantes, précieuses. On les compte avant d'entrer quelque part." },
];

export const reperes: Objet[] = [
  { id: "carte", nom: "Carte", texte: "Vierge au réveil, sauf la zone de départ. Le reste se dessine à mesure qu'on explore." },
  { id: "gps", nom: "GPS", texte: "Une flèche vers le prochain lieu repéré mais pas encore visité. Il marche avec des piles." },
  { id: "pile", nom: "Piles", texte: "À glaner dans les ruines. Elles font vivre le GPS comme la lampe, et elles s'usent." },
  { id: "lampe", nom: "Lampe", texte: "Les ruines sont sombres, les nuits aussi. Elle consomme les mêmes piles que le GPS." },
  { id: "jumelles", nom: "Jumelles", texte: "Repérer avant d'avancer : ce qu'on voit de loin évite des ennuis." },
];

export const equipement: Objet[] = [
  { id: "lampe", nom: "Lampe", texte: "Les ruines sont sombres, les nuits aussi." },
  { id: "pile", nom: "Pile", texte: "Récupérée dans les vestiges, elle fait vivre la lampe et le GPS." },
  { id: "gps", nom: "GPS", texte: "Un vieil appareil qui fonctionne encore, sur piles. Il pointe vers les lieux à découvrir." },
  { id: "jumelles", nom: "Jumelles", texte: "Repérer avant d'avancer : ce qu'on voit de loin évite des ennuis." },
  { id: "pied_de_biche", nom: "Pied-de-biche", texte: "Pour forcer ce que les anciens ont fermé." },
];
