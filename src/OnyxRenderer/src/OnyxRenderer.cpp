#include "OnyxRenderer.h"

#include <iostream>
#include <pxr/imaging/hd/renderThread.h>
#include <pxr/imaging/hd/tokens.h>

#include "DiffuseMaterial.h"
#include "OnyxHelper.h"

using namespace Onyx;


OnyxRenderer::OnyxRenderer()
{
    m_EmbreeDevice = rtcNewDevice(NULL);
    m_EmbreeScene = rtcNewScene(m_EmbreeDevice);

    m_MaterialDataBuffer.emplace_back(
        PathMaterialPair{
            pxr::SdfPath::EmptyPath(),
            // Wymowny kolor materiału sygnalizujący brak powiązania geometria-materiał.
            std::make_unique<DiffuseMaterial>(pxr::GfVec3f(1.0, 0.0, 0.85))
        }
    );

    const DataPayload payload = {
        .Scene = &m_EmbreeScene,
        .LightBuffer = &m_LightDataBuffer,
        .MaterialBuffer = &m_MaterialDataBuffer
    };

    m_Integrator = new OnyxPathtracingIntegrator(payload);
}


OnyxRenderer::~OnyxRenderer()
{
    rtcReleaseScene(m_EmbreeScene);
    rtcReleaseDevice(m_EmbreeDevice);
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

    m_LightDataBuffer.emplace_back(totalEmissionPower);

    m_ResetIntegratorState = true;

    // Zwracamy indeks pod jakim przechowujemy dane światła.
    return m_LightDataBuffer.size() - 1;
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
        for (auto& material : m_MaterialDataBuffer)
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
    m_MaterialDataBuffer.emplace_back(
        IOR > 1.0
        ? PathMaterialPair(materialPath, std::make_unique<DiffuseMaterial>(DiffuseMaterial(diffuseColor)))
        : PathMaterialPair(materialPath, std::make_unique<DiffuseMaterial>(DiffuseMaterial(diffuseColor)))
    );

    m_ResetIntegratorState = true;
}


void OnyxRenderer::DetachGeometryFromScene(uint geometryID)
{
    // Odpinamy geometrię od sceny.
    rtcDetachGeometry(m_EmbreeScene, geometryID);

    m_ResetIntegratorState = true;
}


uint OnyxRenderer::GetIndexOfMaterialByPath(const pxr::SdfPath& materialPath) const
{
    // Iterujemy przez dostępne materiały szukając materiału którego ścieżka odpowiada
    // ścieżce powiązania (binding) materiału. Funkcja jest wywoływana z poziomu synchronizacji geometrii.
    for (int index = 0; index < m_MaterialDataBuffer.size(); index++)
    {
        if(m_MaterialDataBuffer[index].first != materialPath) continue;

        return index;
    }

    // W przypadku braku prawidłowego powiązania używamy indeksu domyślnego materiału.
    // Bufor przechowuje domyślny materiał na początku wektora.
    return 0;
}


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

    if(m_ResetIntegratorState)
    {
        m_Integrator.value()->ResetState();
    }

    while(!m_Integrator.value()->IsConverged())
    {
        m_Integrator.value()->PerformIteration();
    }

    return true;
}