// ENTROPÍA · Set de animaciones (Fase 3b)
// Cada acción del parkour, la cobertura y la pelea tiene aquí su "espacio" de animación. El C++ mueve al personaje y
// la animación solo es el aspecto: Hugo cambia las provisionales (de la plantilla) por las buenas en el Data Asset
// /Game/Entropia/Datos/DA_Animaciones, sin tocar código. Si ese asset no existe, se usan los valores por defecto de aquí.
// Una animación vacía = no se reproduce nada (la acción igual funciona).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EntropiaAnimationSet.generated.h"

class ACharacter;
class UAnimSequenceBase;

/** Una animación y cómo reproducirla. */
USTRUCT(BlueprintType)
struct ENTROPY_API FEntropiaAnimSlot
{
	GENERATED_BODY()

	/** Secuencia o montaje (sobre el esqueleto del maniquí de Epic). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	TSoftObjectPtr<UAnimSequenceBase> Anim;

	/** 1 = normal. 0 = se queda congelada en StartTime (sirve para "poses"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float PlayRate = 1.f;

	/** Segundo de la animación donde empieza. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float BlendIn = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	float BlendOut = 0.2f;

	/** Se repite hasta que alguien la pare (poses y ciclos: agachado, colgado, apuntando). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bLoop = false;
};

UCLASS(BlueprintType)
class ENTROPY_API UEntropiaAnimationSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UEntropiaAnimationSet();

	/** El set en uso: DA_Animaciones si existe; si no, los valores por defecto. */
	static const UEntropiaAnimationSet* Get();

	// --- Moverse
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moverse") FEntropiaAnimSlot CrouchPose;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moverse") FEntropiaAnimSlot Roll;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moverse") FEntropiaAnimSlot HardLanding;

	// --- Parkour
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour") FEntropiaAnimSlot DashVault;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour") FEntropiaAnimSlot Kong;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour") FEntropiaAnimSlot StepUp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour") FEntropiaAnimSlot Climb;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour") FEntropiaAnimSlot WallRun;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour") FEntropiaAnimSlot Hang;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour") FEntropiaAnimSlot ClimbFromHang;

	// --- Cobertura
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cobertura") FEntropiaAnimSlot CoverLow;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cobertura") FEntropiaAnimSlot CoverHigh;

	// --- Pelea (jugador)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pelea") FEntropiaAnimSlot Punch1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pelea") FEntropiaAnimSlot Punch2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pelea") FEntropiaAnimSlot Punch3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pelea") FEntropiaAnimSlot NeckBreak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pelea") FEntropiaAnimSlot FocusStrike;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pelea") FEntropiaAnimSlot Grab;

	// --- Pelea (dummies)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummies") FEntropiaAnimSlot VictimHit;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummies") FEntropiaAnimSlot VictimNeckBreak;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummies") FEntropiaAnimSlot VictimSubdued;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummies") FEntropiaAnimSlot VictimDisarmed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummies") FEntropiaAnimSlot DummyAim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummies") FEntropiaAnimSlot DummyFire;

	// --- Armas del jugador (Fase 4): apuntar quieto, disparar y recargar (cuerpo entero, por ahora)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armas") FEntropiaAnimSlot RevolverAim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armas") FEntropiaAnimSlot RevolverFire;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armas") FEntropiaAnimSlot RevolverReload;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armas") FEntropiaAnimSlot RifleAim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armas") FEntropiaAnimSlot RifleFire;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armas") FEntropiaAnimSlot RifleReload;
};

namespace EntropiaAnim
{
	/** Ruta del Data Asset que Hugo edita. */
	ENTROPY_API extern const TCHAR* SetObjectPath;

	/** Reproduce la animación en el slot por defecto del personaje. Devuelve cuánto dura (0 si no hay animación). */
	ENTROPY_API float Play(ACharacter* Character, const FEntropiaAnimSlot& Slot);

	/** Para lo que esté sonando en el slot por defecto. */
	ENTROPY_API void Stop(ACharacter* Character, float BlendOut = 0.2f);

	/** ¿Está sonando esta animación ahora? */
	ENTROPY_API bool IsPlaying(const ACharacter* Character, const FEntropiaAnimSlot& Slot);
}
