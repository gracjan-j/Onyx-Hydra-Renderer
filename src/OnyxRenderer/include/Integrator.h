#pragma once


namespace Onyx
{
    /**
     * Klasa abstrakcyjna obiektu wykonującego integrację za pomocą metody Monte-Carlo.
     * Integratory implementujące algorytmy wykorzystujące metodę Monte-Carlo
     * powinny dziedziczyć z tej klasy.
     */
    class Integrator
    {
    public:
        virtual ~Integrator() = default;

        /**
         * Za pomocą tej metody integrator powinien wykonać jedną iterację algorytmu.
         */
        virtual void PerformIteration() = 0;


        /**
         * Za pomocą tej metody integrator powinien informować o zakończeniu procesu integracji.
         * @return True jeśli proces integracji został zakończony, False jeśli integracja jest nadal możliwa.
         */
        virtual bool IsConverged() = 0;


        /**
         * Za pomocą tej metody integrator przywraca swój stan do stanu początkowego.
         * Takiej operacji może wymagać silnik w przypadku zmiany danych wejściowych oraz unieważnienia
         * efektu dotychczasowej integracji.
         */
        virtual void ResetState() = 0;
    };

}