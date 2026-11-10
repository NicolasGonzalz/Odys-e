// Le bestiaire, version publique (source : docs/ZONES.md) : ce qu'on croise, pas ce que ça cache.
// Sans spoiler : aucun nom de faction, aucun lien avec le récit, aucun boss.
import type { ImageMetadata } from "astro";
import chien from "../assets/chien.webp";
import colosse from "../assets/colosse.webp";
import drone from "../assets/drone.webp";
import insecte from "../assets/insecte.webp";
import loup from "../assets/loup.webp";
import plante from "../assets/plante.webp";
import robot from "../assets/robot.webp";
import sanglier from "../assets/sanglier.webp";

export interface Creature {
  nom: string;
  image: ImageMetadata;
  texte: string;
}

export interface Famille {
  id: string;
  titre: string;
  accroche: string;
  creatures: Creature[];
}

export const familles: Famille[] = [
  {
    id: "faune",
    titre: "La faune revenue",
    accroche: "Chiens, sangliers, cerfs : les hommes sont partis, les bêtes sont restées. Elles ont retrouvé leurs instincts.",
    creatures: [
      { nom: "Chien errant", image: chien, texte: "Ancien compagnon, désormais en meute. Il vous suit de loin, puis de plus près." },
      { nom: "Sanglier", image: sanglier, texte: "Territorial et têtu. On le chasse pour sa viande, on l'évite quand il baisse la tête." },
    ],
  },
  {
    id: "vivant",
    titre: "Ce qui pousse",
    accroche: "Certaines choses ont poussé trop vite, trop grand, et pas comme il faut. Elles brillent doucement dans la nuit.",
    creatures: [
      { nom: "Loup de feuilles", image: loup, texte: "Le pelage est fait de feuillage, les yeux luisent. Il ne fait aucun bruit." },
      { nom: "Insecte-feuille", image: insecte, texte: "Petit, vif, entouré de spores lumineuses. On l'entend avant de le voir." },
      { nom: "Plante à tentacules", image: plante, texte: "Magnifique, de loin. Les fleurs sont un piège." },
    ],
  },
  {
    id: "machines",
    titre: "Ce qui tourne encore",
    accroche: "Rouillées, envahies de lierre, mais toujours en marche. Elles suivent des ordres que plus personne ne donne.",
    creatures: [
      { nom: "Drone de garde", image: drone, texte: "Un œil vert, quatre pattes, et une ronde qui ne s'arrête jamais." },
      { nom: "Sentinelle", image: robot, texte: "Haute, lente, patiente. Elle surveille des lieux que tout le monde a oubliés." },
      { nom: "Colosse", image: colosse, texte: "On l'entend avant de le voir. Mieux vaut ne pas être là quand il passe." },
    ],
  },
];
