// ENTROPÍA · Set de armas (Fase 4)
// Todos los números de las armas (cargador, cadencia, daño, dispersión, retroceso) y sus modelos, en un solo archivo
// editable: /Game/Entropia/Datos/DA_Armas (el editor lo crea solo si falta, como DA_Animaciones). Hugo ajusta aquí sin
// tocar C++. Si ese asset no existe, se usan los valores por defecto de aquí.
// Los modelos vienen de Fab y viven en Content/Entropia/ArmasFab (fuera de Git: la licencia no deja publicarlos);
// los importa Tools/import-armas.ps1. Si faltan, el arma se dibuja con un cubo.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EntropiaWeaponSet.generated.h"

class UStaticMesh;

/** Una pieza del modelo del arma (cuerpo, mira, cargador...). */
USTRUCT(BlueprintType)
struct ENTROPY_API FEntropiaWeaponPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Posición, giro y escala respecto al socket weapon_r de la mano derecha. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	FTransform Offset = FTransform::Identity;

	/** Es el cargador (se puede mover al recargar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	bool bMagazine = false;
};

/** Un arma: su modelo y cómo dispara. */
USTRUCT(BlueprintType)
struct ENTROPY_API FEntropiaWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	FName Name;

	/** Lo que muestra el HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entropia")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modelo")
	TArray<FEntropiaWeaponPart> Parts;

	/** Punta del cañón respecto al socket weapon_r. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modelo")
	FVector MuzzleOffset = FVector(0.f, 30.f, 6.f);

	/** Color propio (oscuro) para todo el modelo. Alfa 0 = conservar los materiales del modelo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modelo")
	FLinearColor Tint = FLinearColor(0.f, 0.f, 0.f, 0.f);

	/** Segundos entre tiro y tiro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disparo", meta = (ClampMin = "0.02"))
	float FireInterval = 0.45f;

	/** Mantener el clic = sigue disparando. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disparo")
	bool bAutomatic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disparo", meta = (ClampMin = "1"))
	int32 MagazineSize = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disparo", meta = (ClampMin = "0.1"))
	float ReloadSeconds = 1.6f;

	/** Daño al aguante (los maniquíes tienen 100). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dano")
	float TorsoDamage = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dano")
	float HeadDamage = 200.f;

	/** cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dano")
	float BulletSpeed = 8000.f;

	/** Empujón al cuerpo que tumba (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dano")
	float Impulse = 900.f;

	/** Dispersión en grados: quieto y sin disparar, y el tope. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punteria")
	float SpreadMin = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punteria")
	float SpreadMax = 2.5f;

	/** Cuánto crece por tiro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punteria")
	float SpreadPerShot = 1.2f;

	/** Grados por segundo que se cierra al dejar de disparar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punteria")
	float SpreadRecovery = 4.f;

	/** Patada de la cámara hacia arriba y de lado (grados). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punteria")
	float RecoilPitch = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punteria")
	float RecoilYaw = 0.6f;
};

UCLASS(BlueprintType)
class ENTROPY_API UEntropiaWeaponSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UEntropiaWeaponSet();

	/** El set en uso: DA_Armas si existe; si no, los valores por defecto. */
	static const UEntropiaWeaponSet* Get();

	/** El arma insignia del viejo: 6 tiros, lento, pega muy fuerte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador") FEntropiaWeaponStats Revolver;
	/** Automático, 30 tiros, rápido, pega menos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jugador") FEntropiaWeaponStats Rifle;
	/** La pistola de los dummies armados (solo se usa su modelo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummies") FEntropiaWeaponStats DummyPistol;

	/** Las texturas de los modelos importados se limitan a este tamaño (la GTX 1650 tiene 4 GB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendimiento")
	int32 MaxTextureSize = 1024;
};

namespace EntropiaWeapons
{
	/** Ruta del Data Asset que Hugo edita. */
	ENTROPY_API extern const TCHAR* SetObjectPath;
}
