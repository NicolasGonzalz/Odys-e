// Site statique : Vercel sert simplement le dossier dist/ produit par `astro build`.
import { defineConfig } from "astro/config";

export default defineConfig({
  site: "https://odyssee.vercel.app",
});
