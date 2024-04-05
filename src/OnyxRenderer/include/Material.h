#pragma once


#include <pxr/base/gf/vec3f.h>


namespace Onyx
{
    /**
     * Klasa abstrakcyjna materiału reprezentowanego przez funkcję BXDF (BRDF / BTDF)
     * który pozwala na ewaluację wymiany energi na powierzchni obiektów oraz generowanie
     * kierunków odbicia / załamania dla których wymiana energii jest najmniej stratna (Importance Sampling).
     */
    class Material
    {
    public:
        virtual ~Material() = default;

        /**
         * Za pomocą tej metody materiał powinien generować kierunek odbicia / załamania
         * na podstawie wykonania procesu Importance Sampling.
         * @return Kierunek odbicia załamania w świecie sceny (world-space).
         */
        virtual pxr::GfVec3f Sample() = 0;


        /**
         * Za pomocą tej metody materiał powinien obliczyć wymianę energi dla wektora wejściowego i wyjściowego
         * biorąc pod uwagę swoją charakterystykę.
         */
        virtual pxr::GfVec3f Evaluate() = 0;


        /**
         * Za pomocą tej metody materiał powinien zwrócić prawdopodobieństwo dla wygenerowanego kierunku
         * wg. funkcji PDF (Probability Distribution Function) materiału.
         * @param sample Kierunek odbicia / załamania dla którego obliczone zostanie prawdopodobieństwo
         * @return Szacunek prawdopodobieństwa dla kierunku.
         */
        virtual float PDF(pxr::GfVec3f sample) = 0;
    };

}
