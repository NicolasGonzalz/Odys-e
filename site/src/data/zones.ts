// Les cinq zones, version publique (source : docs/ZONES.md) : ambiance seulement.
// Sans spoiler : ni ce qu'on y comprend, ni les gardiens, ni les artefacts. La dernière reste un mystère.
import type { ImageMetadata } from "astro";

import zone1 from "../assets/zone-1.webp";
import zone2 from "../assets/zone-2.webp";
import zone3 from "../assets/zone-3.webp";
import zone4 from "../assets/zone-4.webp";
import zone5 from "../assets/zone-5.webp";

export interface Zone {
  numero: number;
  nom: string;
  paysage: string;
  danger: number; // de 1 à 5
  image: ImageMetadata;
  ambiance: string;
  mystere?: boolean; // zone dont on ne montre presque rien
}

export const zones: Zone[] = [
  {
    numero: 1,
    nom: "L'Abri",
    paysage: "Campagne, lisière",
    danger: 1,
    image: zone1,
    ambiance:
      "Collines douces, prairies qui ont recouvert les champs, vergers redevenus sauvages, brume du matin. Calme, presque beau. C'est ici que l'on se réveille.",
  },
  {
    numero: 2,
    nom: "La Ville engloutie",
    paysage: "Ville avalée par la forêt",
    danger: 2,
    image: zone2,
    ambiance:
      "Tours éventrées couvertes de lierre, arbres qui poussent dans les avenues, métro inondé, lumière verte filtrée par la canopée. Les enseignes sont encore lisibles.",
  },
  {
    numero: 3,
    nom: "Les Terres noyées",
    paysage: "Plaine inondée, marais",
    danger: 3,
    image: zone3,
    ambiance:
      "Le fleuve a changé de lit et les routes sont sous l'eau. Lucioles et spores flottent au-dessus des marais ; au loin, d'immenses serres à moitié effondrées.",
  },
  {
    numero: 4,
    nom: "Le Site des Portes",
    paysage: "Haut plateau, montagne",
    danger: 4,
    image: zone4,
    ambiance:
      "Un plateau froid, balayé par la neige et le vent. Béton, métal, et d'immenses structures éteintes que la nature n'a pas réussi à reprendre.",
  },
  {
    numero: 5,
    nom: "Le Cœur",
    paysage: "???",
    danger: 5,
    image: zone5,
    ambiance:
      "Personne n'en revient avec des réponses simples. On raconte qu'au-delà du plateau pousse une forêt plus vieille que tout le reste.",
    mystere: true,
  },
];
