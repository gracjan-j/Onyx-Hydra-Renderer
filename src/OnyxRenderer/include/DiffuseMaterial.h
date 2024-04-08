#pragma once

#include "Material.h"


namespace Onyx
{

    class DiffuseMaterial: public Material
    {
    public:

        DiffuseMaterial() = delete;

        DiffuseMaterial(const pxr::GfVec3f& diffuseReflectance)
        : m_DiffuseReflectance{diffuseReflectance}
        {}


        /**
         * Metoda generuje próbkę materiału zgodnie z metodą Cosine Weighted Importance Sampling
         * dla materiału reprezentowanego przez Lambert BRDF.
         * @param N Wektor normalny poweirzchni który wyznacza górną hemisferę uderzonej powierzchni
         * @param random2D Dwa numery losowe do wygenerowania próbki
         * @return Próbka w world-space
         */
        pxr::GfVec3f Sample(const pxr::GfVec3f& N, const pxr::GfVec2f& random2D) override;


        /**
         * Metoda oblicza Lambert BRDF dla materiału.
         * Lambert BRDF = Diffuse Reflectance / PI
         * Jednak ze względu na zastosowanie Importance Samplingu PI skraca się z funkcją PDF
         * @param sample Wygenerowana próbka materiału.
         * @return Rezultat obliczenia funkcji BRDF dla przekazanej próbki.
         * @note Funkcja Lambert BRDF jest stała i niezależna od promienia padania (sample).
         */
        pxr::GfVec3f Evaluate(const pxr::GfVec3f& sample) override;


        /**
         * Metoda oblicza prawdopodobieństwo wygenerowania próbki.
         * Korzystamy z metody Cosine Weighted Importance Sampling dla funkcji Lambert(ian) BRDF
         * w efekcie iloraz skalowania mocy ścieżki ogranicza się do wartości Diffuse Reflectance.
         * Skraca sięPI oraz "cosine term" który skaluje moc w zalezności od kąta padania.
         * @param sample Próbka wygenerowana przez metodę Sample()
         * @return Prawdopodobieństwo 1.0 wynikające z redukcji opisanej powyżej.
         */
        float PDF(const pxr::GfVec3f& sample) override;

    private:

        pxr::GfVec3f m_DiffuseReflectance;
    };

}


