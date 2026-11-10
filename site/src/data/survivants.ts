// Les quatre survivants, version publique (source : docs/CHARACTERS.md).
// Sans spoiler : jamais « ce qu'il ignore », ni les indices personnels, ni les liens entre eux.
import type { ImageMetadata } from "astro";

import ilian from "../assets/ilian.webp";
import mara from "../assets/mara.webp";
import nadia from "../assets/nadia.webp";
import teo from "../assets/teo.webp";

export interface Survivant {
  id: string;
  nom: string;
  role: string;
  age: number;
  image: ImageMetadata;
  avant: string;
  specialite: string;
  atouts: string[];
  silhouette: string;
}

export const survivants: Survivant[] = [
  {
    id: "mara",
    nom: "Mara Velin",
    role: "L'ingénieure",
    age: 34,
    image: mara,
    avant:
      "Technicienne de maintenance sur un immense chantier d'avant. Elle s'est endormie avec une mission précise ; à son réveil, il n'y a plus personne pour la lui confier.",
    specialite: "Mécanique",
    atouts: [
      "Répare les machines plus vite",
      "Ouvre certains mécanismes sans clé",
      "Ses outils s'usent moins",
    ],
    silhouette: "Combinaison de travail, harnais d'outils, cheveux attachés, gestes précis.",
  },
  {
    id: "ilian",
    nom: "Ilian Sorel",
    role: "Le biologiste",
    age: 51,
    image: ilian,
    avant:
      "Chercheur, spécialiste du vivant. Il connaît les plantes mieux que personne, et semble porter un poids qu'il ne partage avec personne.",
    specialite: "Botanique",
    atouts: [
      "Reconnaît les plantes comestibles, toxiques et médicinales",
      "Récolte davantage sur la flore",
      "Apaise plus facilement les créatures",
    ],
    silhouette: "Grand, maigre, blouse usée sous un long manteau, lunettes fêlées.",
  },
  {
    id: "teo",
    nom: "Teo Brandt",
    role: "Le débrouillard",
    age: 19,
    image: teo,
    avant:
      "Il n'aurait jamais dû se trouver là. Une nuit de panique, il s'est glissé dans une capsule vide, sans que personne le sache.",
    specialite: "Débrouille",
    atouts: [
      "Discret : la faune le repère moins",
      "Se faufile dans les passages étroits",
      "Fouille mieux les ruines des villes",
    ],
    silhouette: "Vêtements de ville déchirés, capuche, gros sac à dos, toujours une idée.",
  },
  {
    id: "nadia",
    nom: "Nadia Okoro",
    role: "L'agente",
    age: 42,
    image: nadia,
    avant:
      "Agente de sécurité du centre où dormaient les survivants. Droite et méfiante, elle se souvient de la dernière nuit mieux qu'elle ne le voudrait.",
    specialite: "Protection",
    atouts: [
      "Plus résistante",
      "Bloque plus efficacement",
      "Plus efficace au combat",
    ],
    silhouette: "Uniforme renforcé, posture droite, cicatrice, matériel de protection.",
  },
];
