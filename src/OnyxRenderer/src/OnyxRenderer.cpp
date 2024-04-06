#include "OnyxRenderer.h"

#include <iostream>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/imaging/hd/tokens.h>

#include "../../hdOnyx/include/mesh.h"
#include "DiffuseMaterial.h"
#include "OnyxHelper.h"

using namespace Onyx;


OnyxRenderer::OnyxRenderer()
{
    m_EmbreeDevice = rtcNewDevice(NULL);
    m_EmbreeScene = rtcNewScene(m_EmbreeDevice);

    m_ProjectionMat.SetIdentity();
    m_ViewMat.SetIdentity();

    m_ProjectionMatInverse.SetIdentity();
    m_ViewMatInverse.SetIdentity();


    m_MaterialBuffer.emplace_back(
        PathMaterialPair{
            pxr::SdfPath::EmptyPath(),
            // Wymowny kolor materiału sygnalizujący brak powiązania geometria-materiał.
            std::make_unique<DiffuseMaterial>(pxr::GfVec3f(1.0, 0.0, 0.85))
        }
    );
}


uint OnyxRenderer::AttachGeometryToScene(const RTCGeometry& geometrySource)
{
    auto meshID = rtcAttachGeometry(m_EmbreeScene, geometrySource);

    if (meshID == RTC_INVALID_GEOMETRY_ID)
    {
        std::cout << "[Onyx] Związanie geometrii do sceny nie jest możliwe dla meshID: " << meshID << std::endl;
    }

    return meshID;
}


uint OnyxRenderer::AttachLightInstanceToScene(
    pxr::HdOnyxInstanceData* instanceData,
    const pxr::GfVec3f& totalEmissionPower)
{
    if(!m_RectLightPrimitiveScene.has_value())
    {
        PrepareRectLightGeometrySource();
    }

    // Tworzymy nową geometrię typu - instance
    // Korzystamy w ten sposób z możliwości utworzenia wirtualnej kopii bazowej geometrii
    // z własnym przekształceniem, zamiast modyfikacji bazowej geometrii transformacją.
    RTCGeometry rectInstanceGeometrySource = rtcNewGeometry(m_EmbreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);

    // Ustawiamy źródło instancji - bazowy obiekt geometrii
    rtcSetGeometryInstancedScene(rectInstanceGeometrySource, m_RectLightPrimitiveScene.value());
    rtcSetGeometryTimeStepCount(rectInstanceGeometrySource, 1);

    // Wywołanie funkcji SetGeometryTransform jest możliwe tylko i wyłącznie na
    // instancjach. Korzystająć z prymitywnego typu geometrii - triangle w Embree
    // możemy osiągnąć poprawną transformację, transformując wierzchołki (punkty) geometrii.
    // Jednak, lepszym rozwiązaniem jest wykorzystanie instancingu.
    rtcSetGeometryTransform(
        rectInstanceGeometrySource,
        0,
        RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,
        instanceData->TransformMatrix.GetArray()
    );

    // Powiązujemy małą strukturę z instancją. Podczas testu intersekcji,
    // możemy otrzymać poniższy wskaźnik do struktury powiązany z instancją.
    rtcSetGeometryUserData(rectInstanceGeometrySource, instanceData);

    // Finalnie wnioskujemy o utworzenie geometrii.
    rtcCommitGeometry(rectInstanceGeometrySource);

    // Doczepiamy instancję światła do sceny silnika.
    // Do rozróżniania obiektów światła służy nam struktura pomocnicza instancji.
    AttachGeometryToScene(rectInstanceGeometrySource);
}


void OnyxRenderer::AttachOrUpdateMaterial(
    const pxr::GfVec3f& diffuseColor,
    const float& IOR,
    const pxr::SdfPath& materialPath,
    bool newMaterial)
{
    if (!newMaterial)
    {
        // Szukamy materiału do edycji, materiał powinien już istnieć.
        for (auto& material : m_MaterialBuffer)
        {
            if (material.first != materialPath) continue;

            material.second.reset();

            material.second = IOR > 1.0
            ? std::make_unique<DiffuseMaterial>(DiffuseMaterial(diffuseColor))
            : std::make_unique<DiffuseMaterial>(DiffuseMaterial(diffuseColor));
        }
    }

    // Jeśli materiał do edycji nie został znaleziony lub wymaga stworzenia na nowo,
    // dodajemy nowy obiekt do mapy.
    m_MaterialBuffer.emplace_back(
        IOR > 1.0
        ? PathMaterialPair(materialPath, std::make_unique<DiffuseMaterial>(DiffuseMaterial(diffuseColor)))
        : PathMaterialPair(materialPath, std::make_unique<DiffuseMaterial>(DiffuseMaterial(diffuseColor)))
    );

}


void OnyxRenderer::DetachGeometryFromScene(uint geometryID)
{
    // Odpinamy geometrię od sceny.
    rtcDetachGeometry(m_EmbreeScene, geometryID);
}


uint OnyxRenderer::GetIndexOfMaterialByPath(const pxr::SdfPath& materialPath) const
{
    // Iterujemy przez dostępne materiały szukając materiału którego ścieżka odpowiada
    // ścieżce powiązania (binding) materiału. Funkcja jest wywoływana z poziomu synchronizacji geometrii.
    for (int index = 0; index < m_MaterialBuffer.size(); index++)
    {
        if(m_MaterialBuffer[index].first != materialPath) continue;

        return index;
    }

    // W przypadku braku prawidłowego powiązania używamy indeksu domyślnego materiału.

    // Bufor przechowuje domyślny materiał na początku wektora.
    return 0;
}


OnyxRenderer::~OnyxRenderer()
{
    rtcReleaseScene(m_EmbreeScene);
    rtcReleaseDevice(m_EmbreeDevice);
}


void OnyxRenderer::SetCameraMatrices(pxr::GfMatrix4d projMatrix, pxr::GfMatrix4d viewMatrix)
{
    m_ProjectionMat = projMatrix;
    m_ViewMat = viewMatrix;

    m_ProjectionMatInverse = m_ProjectionMat.GetInverse();
    m_ViewMatInverse = m_ViewMat.GetInverse();
}


void writeNormalDataAOV(uint8_t* pixelDataStart, pxr::GfVec3f normal)
{
    pixelDataStart[0] = uint(((normal.data()[0] + 1.0) / 2.0) * 255);
    pixelDataStart[1] = uint(((normal.data()[1] + 1.0) / 2.0) * 255);
    pixelDataStart[2] = uint(((normal.data()[2] + 1.0) / 2.0) * 255);
    pixelDataStart[3] = 255;
}


pxr::GfVec3f TurboColorMap(float x) {
    float r = 0.1357 + x * ( 4.5974 - x * ( 42.3277 - x * ( 130.5887 - x * ( 150.5666 - x * 58.1375 ))));
    float g = 0.0914 + x * ( 2.1856 + x * ( 4.8052 - x * ( 14.0195 - x * ( 4.2109 + x * 2.7747 ))));
    float b = 0.1067 + x * ( 12.5925 - x * ( 60.1097 - x * ( 109.0745 - x * ( 88.5066 - x * 26.8183))));
    return {r, g, b};
}


void OnyxRenderer::WriteDataToSupportedAOV(const pxr::GfVec3f& colorOutput, const pxr::GfVec3f& normalOutput)
{}


void OnyxRenderer::PrepareRectLightGeometrySource()
{
    m_RectLightPrimitiveScene = rtcNewScene(m_EmbreeDevice);

    // Tworzymy bazową geometrię stworzoną z dwóch trójkątów na płaszczyźnie XY.
    RTCGeometry localRectangleGeometry = rtcNewGeometry(m_EmbreeDevice, RTC_GEOMETRY_TYPE_TRIANGLE);

    // Do zdefiniowania czworokąta wystarczą cztery dodatkowo indeksowane wierzchołki.
    // Każdy wierzchołek to punkt 3D.
    // Wypełniamy bufor utworzony przez Embree.
    auto* pointBuffer = (float*)rtcSetNewGeometryBuffer(
        localRectangleGeometry,
        RTC_BUFFER_TYPE_VERTEX,
        0,
        RTC_FORMAT_FLOAT3,
        3 * sizeof(float),
        4);

    // Punkt (0.0, 0.0, 0.0) to punkt przecięcia przekątnych.
    // Rozmiar w X oraz Y to 1 jednostka.
    // Zdefiniowane na płaszczyźnie XY (Z nie ulega zmianie).
    pointBuffer[0] = -0.5f; pointBuffer[1] = 0.5f; pointBuffer[2] = 0.f;
    pointBuffer[3] = 0.5f; pointBuffer[4] = 0.5f; pointBuffer[5] = 0.f;
    pointBuffer[6] = 0.5f; pointBuffer[7] = -0.5f; pointBuffer[8] = 0.f;
    pointBuffer[9] = -0.5f; pointBuffer[10] = -0.5f; pointBuffer[11] = 0.f;

    // Indeksujemy utworzone punkty tworząc dwa trójkąty.
    // Każdy trójkąt jest określony przez trzy indeksy punktów w pointBuffer.
    auto* indexBuffer = (unsigned*) rtcSetNewGeometryBuffer(localRectangleGeometry,
        RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
        3 * sizeof(unsigned),
        2);

    // Indeksowanie zgodne ze wskazówkami zegara (kolejność określa orientację trójkąta)
    indexBuffer[0] = 0; indexBuffer[1] = 1; indexBuffer[2] = 2;
    indexBuffer[3] = 2; indexBuffer[4] = 3; indexBuffer[5] = 0;

    // Wnioskujemy o stworzenie geometrii.
    rtcCommitGeometry(localRectangleGeometry);
    // Powiązujemy geometrię ze sceną.
    rtcAttachGeometry(m_RectLightPrimitiveScene.value(), localRectangleGeometry);

    // Wnioskujemy o zbudowanie obiektu sceny (czworokąta który będzie instancjonowany).
    rtcCommitScene(m_RectLightPrimitiveScene.value());
}


void OnyxRenderer::MainRenderingEntrypoint(pxr::HdRenderThread* renderThread)
{
    while (renderThread->IsPauseRequested())
    {
        if (renderThread->IsStopRequested())
        {
            std::cout << "[Onyx Render Thread] Zatrzymano wątek renderowania po pauzie." << std::endl;
            return;
        }

        std::cout << "[Onyx] Pauza wątku renderowania." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (renderThread->IsStopRequested()) {
        std::cout << "[Onyx] Zatrzymano wątek renderowania." << std::endl;
        return;
    }

    RenderAllAOV();
}


bool OnyxRenderer::RenderAllAOV()
{
    // Zatwierdzamy scenę w obecnej postaci przed wywołaniem testów intersekcji.
    // Modyfikacje sceny nie są możliwe. Modyfikacja sceny przez obiekty HdOnyx* jest możliwa
    // poprzez HdOnyxRenderParam. Obiekt ten wymusza StopRender na wątku renderowania.
    rtcCommitScene(m_EmbreeScene);

    // W przypadku braku AOV koloru wykonywnanie pełnego algorytmu nie jest konieczne.
    // Sprawdzamy dostępność tego kanału danych.
    uint aovIndex;
    bool hasColorAOV = m_RenderArgument.IsAvailable(pxr::HdAovTokens->color, aovIndex);

    // Wyciągamy bufory danych do których silnik będzie wpisywał rezultat renderowania różnych zmiennych.
    auto colorAovBufferData = m_RenderArgument.GetBufferData(pxr::HdAovTokens->color);
    auto normalAovBufferData = m_RenderArgument.GetBufferData(pxr::HdAovTokens->normal);

    bool writeColorAOV = colorAovBufferData.has_value();
    bool writeNormalAOV = normalAovBufferData.has_value();

    uint8_t* colorAovBuffer;
    uint8_t* normalAovBuffer;
    size_t colorElementSize, normalElementSize;
    if (writeColorAOV)
    {
        colorAovBuffer = static_cast<uint8_t*>(colorAovBufferData.value().first);
        colorElementSize = colorAovBufferData.value().second;
    }

    if (writeNormalAOV)
    {
        normalAovBuffer = static_cast<uint8_t*>(normalAovBufferData.value().first);
        normalElementSize = normalAovBufferData.value().second;
    }

    // Dla każdego pixela w buforze.
    for (auto currentY = 0; currentY < m_RenderArgument.height; currentY++)
    {
        for (auto currentX = 0; currentX < m_RenderArgument.width; currentX++)
        {
            // Obliczamy jedno-wymiarowy offset pixela w buforze 2D.
            uint32_t pixelOffsetInBuffer = (currentY * m_RenderArgument.width) + currentX;

            // Znajdujemy początek danych piksela w buforach AOV
            uint8_t* pixelDataNormal = writeNormalAOV
                ? &normalAovBuffer[pixelOffsetInBuffer * normalElementSize]
                : nullptr;

            uint8_t* pixelDataColor = writeColorAOV
                ? &colorAovBuffer[pixelOffsetInBuffer * colorElementSize]
                : nullptr;

            auto pixelColor = pxr::GfVec3f(0.0);
            auto pixelNormal = pxr::GfVec3f(0.0);

            // Generujemy promień z kamery.
            RTCRayHit primaryRayHit = OnyxHelper::GeneratePrimaryRay(
                currentX, currentY,
                m_RenderArgument.width, m_RenderArgument.height,
                m_ProjectionMatInverse, m_ViewMatInverse
            );

            // Dokonujemy testu intersekcji promienia ze sceną
            rtcIntersect1(m_EmbreeScene, &primaryRayHit, nullptr);

            // Jeżeli promień nie trafił w geometrię.
            if (primaryRayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
            {
                // Kończymy obliczenia dla piksela.
                if (writeColorAOV)
                {
                    uint8_t* colorPixelBufferData = &colorAovBuffer[pixelOffsetInBuffer * colorElementSize];
                    colorPixelBufferData[0] = 0; colorPixelBufferData[1] = 0;
                    colorPixelBufferData[2] = 0; colorPixelBufferData[3] = 255;
                }

                if (writeNormalAOV)
                {
                    writeNormalDataAOV(pixelDataNormal, pxr::GfVec3f(0.0));
                }

                // Przechodzimy do następnego piksela.
                continue;
            }

            // Pobieramy strukturę pomocniczą powiązaną z instancją na podstawie identyfikatora instancji
            // w scenie, który otrzymujemy przy intersekcji.
            auto* hitInstanceData = static_cast<pxr::HdOnyxInstanceData*>(
                rtcGetGeometryUserData(
                    // Identyfikator instancji zakłada jedno-poziomowy instancing ([0]).
                    // Zgodne z założeniem w HdOnyxMesh który tworzy geometrię.
                    rtcGetGeometry(m_EmbreeScene, primaryRayHit.hit.instID[0])
                )
            );


            pxr::GfVec3f hitWorldNormal = OnyxHelper::EvaluateHitSurfaceNormal(primaryRayHit, m_EmbreeScene);

            // Wpisujemy dane do normal AOV jeśli jest podpięte do silnika.
            if(writeNormalAOV)
            {
                writeNormalDataAOV(pixelDataNormal, hitWorldNormal);
            }

            if (writeColorAOV)
            {
                uint instanceID = primaryRayHit.hit.instID[0] * 1289;
                auto r = (instanceID % 256u);
                auto g = ((instanceID / 256) % 256);
                auto b = ((instanceID / (256u * 256u)) % 256u);

                if (!hitInstanceData->Light)
                {
                    auto& boundMaterial = m_MaterialBuffer[hitInstanceData->MaterialIndexInBuffer];
                    auto diffuseColor = boundMaterial.second->Evaluate();
                    pixelDataColor[0] = int(diffuseColor[0] * 255);
                    pixelDataColor[1] = int(diffuseColor[1] * 255);
                    pixelDataColor[2] = int(diffuseColor[2] * 255);
                    pixelDataColor[3] = 255;
                }
                else
                {
                    pixelDataColor[0] = 255;
                    pixelDataColor[1] = 0;
                    pixelDataColor[2] = 0;
                    pixelDataColor[3] = 255;
                }

            }
        }
    }

    return true;
}