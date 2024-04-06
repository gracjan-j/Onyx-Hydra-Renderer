#include "light.h"

#include <pxr/base/gf/matrix4f.h>

#include "renderParam.h"

#include <pxr/imaging/hd/sceneDelegate.h>

#include "mesh.h"


pxr::HdOnyxLight::HdOnyxLight(SdfPath const& id)
: HdLight(id)
{}


pxr::HdOnyxLight::~HdOnyxLight()
{}


pxr::HdDirtyBits pxr::HdOnyxLight::GetInitialDirtyBitsMask() const
{
    return HdLight::AllDirty;
}


void pxr::HdOnyxLight::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    // Pobieramy unikalne ID prima typu RectLight (jest to jedyny typ akceptowany przez Render Delegate).
    auto& primID = GetId();

    // Flaga wskazująca na to czy światło zostało zsynchronizowane pierwszy raz.
    // Wymusi działanie wszystkich ścieżek procesu tworzenia światła.
    bool newLight = *dirtyBits & HdLight::DirtyResource;

    bool dirtyTransformFlag = (*dirtyBits & HdLight::DirtyTransform);
    bool dirtyParamsFlag    = (*dirtyBits & HdLight::DirtyParams);

    // Flaga wskazująca na konieczność regeneracji geometrii światła biorąc pod uwagę
    bool sizeChanged = false;

    // Jeśli parametry światła uległy zmianie
    if (newLight || dirtyParamsFlag)
    {
        // Pobieramy dane parametrów które wspiera silnik.
        // Metoda zwróci wartości domyślne jeśli wartość nie jest manualnie wprowadzona przez użytkownika w scenie.
        float width = sceneDelegate->GetLightParamValue(primID, HdLightTokens->width).Get<float>();
        float height = sceneDelegate->GetLightParamValue(primID, HdLightTokens->height).Get<float>();

        float intensity = sceneDelegate->GetLightParamValue(primID, HdLightTokens->intensity).Get<float>();
        float exposure = sceneDelegate->GetLightParamValue(primID, HdLightTokens->exposure).Get<float>();

        GfVec3f color = sceneDelegate->GetLightParamValue(primID, HdLightTokens->color).Get<GfVec3f>();

        // Moc światła jest wyrażana w Nitach. Formuła zgodna z dokumentacją UsdLux w OpenUSD.
        m_TotalEmissivePower = intensity * powf(2.0, exposure) * color;

        // Na podstawie parametrów rozmiaru obliczamy transformację skalowania wymaganą
        // do wykonania korekcji macierzy transformacji samego obiektu.
        m_ParameterScalingTransformation = GfMatrix4d().SetScale(GfVec3d(width, height, 1.0));
    }

    if (newLight || dirtyTransformFlag)
    {
        // Pobieramy nową macierz transformacji.
        m_InstanceTransformation = sceneDelegate->GetTransform(primID);

        // Mnożymy nową macierz przez macierz skalującą zdefiniowaną przez parametry rozmiaru obiektu.
        m_InstanceTransformation = m_ParameterScalingTransformation * m_InstanceTransformation;
    }

    auto* onyxRenderParam = static_cast<HdOnyxRenderParam*>(renderParam);

    // Transformacja uwzględnia skalowanie parametrów oraz macierzy transformacji.
    // Moc emisji uwzględnia intensity oraz exposure wraz z kolorem emisji.
    // Ustawiamy flagę Light w celu rozróżnienia instancji geometrii i świateł
    // które są zdefiniowanej w tej samej globalnej scenie silnika.
    m_LightInstanceData = HdOnyxInstanceData{
        .Light = true,
        .DataIndexInBuffer = 0,
        .SmoothNormalsArray = nullptr,
        .TransformMatrix = GfMatrix4f(m_InstanceTransformation),
    };


    // Dodajemy instancję światła do sceny.
    // Metoda zwróci indeks pod jakim światło
    auto lightIndexInBuffer = onyxRenderParam->GetRendererHandle()->AttachLightInstanceToScene(
        &m_LightInstanceData,
        m_TotalEmissivePower
    );

    // Aktualizujemy indeks do bufora danych świateł po wysłaniu danych do silnika.
    m_LightInstanceData.DataIndexInBuffer = lightIndexInBuffer;

    *dirtyBits = HdLight::Clean;
}