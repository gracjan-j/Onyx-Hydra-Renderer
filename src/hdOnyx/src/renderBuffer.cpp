#include "renderBuffer.h"

#include <iostream>


PXR_NAMESPACE_OPEN_SCOPE


HdOnyxRenderBuffer::HdOnyxRenderBuffer(const SdfPath& bprimId)
: HdRenderBuffer(bprimId)
{}

bool HdOnyxRenderBuffer::Allocate(const GfVec3i& dimensions, HdFormat format, bool multiSampled)
{
    _Deallocate();

    m_Dimensions = dimensions;
    m_DataFormat = format;

    m_DataVector.resize(m_Dimensions[0] * m_Dimensions[1] * m_Dimensions[2] * HdDataSizeOfFormat(format));

    return true;
}

void HdOnyxRenderBuffer::_Deallocate()
{
    if (IsMapped())
    {
        std::cout << "[hdOnyx] Czyszczony bufor jest używany!" << std::endl;
    }

    // Ustawiamy pierwotny, niezainicjalizowany stan bufora.
    m_Dimensions = GfVec3i(0, 0, 0);
    m_DataFormat = HdFormatInvalid;

    // Dane wektora są czyszczone. Rozmiar wektora jest zresetowany do 0.
    m_DataVector.clear();

    // Resetujemy liczbę użytkowników bufora.
    m_MappedUsersCount.store(0);

    // Resetujemy stan ukończenia danych w buforze.
    m_Converged.store(false);
}


unsigned int HdOnyxRenderBuffer::GetWidth() const
{
    return m_Dimensions[0];
}

unsigned int HdOnyxRenderBuffer::GetHeight() const
{
    return m_Dimensions[1];
}

unsigned int HdOnyxRenderBuffer::GetDepth() const
{
    return m_Dimensions[2];
}


HdFormat HdOnyxRenderBuffer::GetFormat() const
{
    return m_DataFormat;
}


bool HdOnyxRenderBuffer::IsMultiSampled() const
{
    return false;
}


void* HdOnyxRenderBuffer::Map()
{
    // Zwiększamy liczbę użytkowników bufora przed udostępnieniem danych.
    m_MappedUsersCount.fetch_add(1);

    return static_cast<void*>(m_DataVector.data());
}

void HdOnyxRenderBuffer::Unmap()
{
    m_MappedUsersCount.fetch_sub(1);
}

bool HdOnyxRenderBuffer::IsMapped() const
{
    // Jeśli liczba użytkowników jest większa od zera, bufor jest w użytku.
    return m_MappedUsersCount.load() > 0;
}


bool HdOnyxRenderBuffer::IsConverged() const
{
    return m_Converged.load();
}


void HdOnyxRenderBuffer::Resolve()
{
    // Dane nie wymagają zmian przed wyświetleniem.
}


PXR_NAMESPACE_CLOSE_SCOPE
