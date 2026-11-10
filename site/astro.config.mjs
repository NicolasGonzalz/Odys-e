// Site statique : Vercel sert simplement le dossier dist/ produit par `astro build`.
import { defineConfig } from "astro/config";

export default defineConfig({
  site: "https://odyssee-sage.vercel.app", // odyssee.vercel.app était déjà pris
});
