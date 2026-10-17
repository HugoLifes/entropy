// ENTROPÍA · Detener el tiempo, nivel 2 (Fase 3b, inspirado en Star Platinum: The World)
// Con la burbuja en nivel 2 (tecla N), Q detiene el tiempo de TODO el mundo unos segundos, menos el tuyo:
//   1) impacto: destello en negativo (cielo naranja y amarillo, sombras blancas);
//   2) una onda sale de ti y, detrás de ella, el mundo queda gris metálico e índigo;
//   3) solo tú tienes color, con un aura viva (púrpura, dorado y cian);
//   4) al final la onda regresa hacia ti y vuelve el color.
// Los golpes y remates a lo que está congelado se guardan y salen cuando el tiempo vuelve a correr.
// El mundo va a 0.001 por el árbitro del tiempo (tú, compensado, a velocidad normal).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TemporalWorldStopComponent.generated.h"

class UMaterialInstanceDynamic;
class UPostProcessComponent;
class UTemporalEnergyComponent;

namespace EntropiaWorldStop
{
	constexpr float ExpandSeconds = 0.8f;
	constexpr float ReturnSeconds = 0.6f;
	constexpr float FlashSeconds = 0.25f;

	/** Radio de la onda en el segundo T de un Detener que dura Duration (sale, se mantiene y regresa). Lógica pura. */
	ENTROPY_API float WaveRadius(float T, float Duration, float MaxRadius);

	/** Destello en negativo del impacto (1 al empezar, 0 a los 0.25 s). */
	ENTROPY_API float Flash(float T);
}

UCLASS(ClassGroup = (Entropia), meta = (BlueprintSpawnableComponent, DisplayName = "Temporal World Stop (Detener nivel 2)"))
class ENTROPY_API UTemporalWorldStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTemporalWorldStopComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float Cost = 60.f;
	/** Segundos reales que dura. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float Duration = 4.f;
	/** Hasta dónde llega la onda (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float MaxRadius = 6000.f;
	/** Velocidad del mundo mientras dura (0.001 = prácticamente detenido). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia") float WorldDilation = 0.001f;

	/** False (y la barra parpadea) si no alcanza la energía o ya está activo. */
	bool Start();
	void Stop();
	bool IsActive() const { return bActive; }
	float GetElapsed() const { return Elapsed; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetPlayerHighlight(bool bOn);
	void UpdateLook();

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> PostProcess;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Effect;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Aura;

	UPROPERTY()
	TObjectPtr<UTemporalEnergyComponent> Energy;

	FVector Origin = FVector::ZeroVector;
	float Elapsed = 0.f;
	bool bActive = false;
};
