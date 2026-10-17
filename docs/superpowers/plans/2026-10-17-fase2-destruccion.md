# Fase 2 · Destrucción, partículas y distorsión — Plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fractura Voronoi rebobinable, escombros y polvo reversibles, distorsión del tiempo para los poderes y un modo tour para que la IA pruebe el juego.

**Architecture:** La lógica pura (fractura, escombros, ondas) va en funciones sin mundo, probadas con Automation Tests. `ATemporalShard` y `UTemporalDebrisComponent` cuelgan del rompible. `UTemporalFXSubsystem` centraliza la distorsión (un MID en un post-process). El Sandbox agrega el tour.

**Tech Stack:** UE 5.8 C++, ProceduralMeshComponent, InstancedStaticMeshComponent, material de post-process generado en el editor (nodo Custom HLSL), Automation Tests.

**Spec:** `docs/superpowers/specs/2026-10-17-fase2-destruccion-design.md`

## Global Constraints
- `Source/entropy/` plano. Comentarios en español. Sin assets hechos a mano.
- Nada de nombres genéricos en namespaces anónimos (unity build).
- Tiempo real: `TemporalFX::RealDelta()`.
- Rompible: `NumShards = 10`, `FractureMode = Irregular` por defecto, rearmado a < 25 cm.
- Escombros: `NumChips = 14`, `NumDust = 10`, `Life = 2.5 s`, gravedad `-980`.
- Distorsión: 4 ondas + 1 aura, CVar `entropia.fx.distortion` (1 = encendido), `BlendWeight = 0` sin efectos.
- Verificación de cada tarea: `powershell -ExecutionPolicy Bypass -File Tools\build-test.ps1` → `Result: Succeeded`, sin `warning C`, y `TESTS: N ok, 0 fail`.

## Review Focus
1. **Un objeto se rompe mientras otro se rebobina** → los escombros de cada uno siguen a sus propios pedazos (el estado es por rompible, no global).
2. **Se rompe un objeto rotado o escalado** (el muro de 40 × 500 × 300) → las celdas cubren todo el volumen y los pedazos quedan alineados; el rearmado funciona.
3. **Distorsión sin jugador o sin cámara** (al empezar o al terminar Play) → no truena y el peso se queda en 0.
4. **El tour en una máquina lenta** → espera por tiempo real, no por frames; si algo falla, registra el paso y sigue hasta `done`.
5. **Muchas roturas seguidas** (B repetida en la sala 4) → ni el pool de ondas ni los escombros crecen sin límite.

---

### Task 1: Fractura Voronoi (lógica pura)
**Files:** Create `Source/entropy/TemporalFracture.h/.cpp` · Test `EntropiaTests.cpp`
**Produces:**
```cpp
struct FFracturePolygon { TArray<FVector> Points; bool bInterior = false; };
struct FFractureCell { TArray<FFracturePolygon> Faces; };
namespace TemporalFracture {
  ENTROPY_API FFractureCell MakeBox(const FVector& HalfSize);
  ENTROPY_API bool ClipCell(const FFractureCell& In, const FPlane& Plane, FFractureCell& Out); // conserva PlaneDot<=0; false si queda vacía
  ENTROPY_API TArray<FFractureCell> VoronoiFracture(const FVector& HalfSize, const TArray<FVector>& Seeds);
  ENTROPY_API TArray<FVector> MakeSeeds(const FVector& HalfSize, int32 Count, const FVector& LocalImpact, int32 RandomSeed);
  ENTROPY_API float CellVolume(const FFractureCell& Cell);
  ENTROPY_API FVector CellCentroid(const FFractureCell& Cell);
  ENTROPY_API TArray<FVector> CellVertices(const FFractureCell& Cell); // únicos (tolerancia 0.01)
}
```
- [ ] Pruebas:
  - `Entropia.Fracture.BoxVolume`: el volumen de `MakeBox(50,50,50)` es 1e6 (±1).
  - `Entropia.Fracture.ClipHalves`: cortar esa caja con el plano X = 0 da un volumen de 5e5 (±1) y una sola cara `bInterior`.
  - `Entropia.Fracture.VoronoiConservesVolume`: `MakeSeeds((20,250,150), 10, (0,0,0), 7)` → `VoronoiFracture` da 10 celdas no vacías y la suma de volúmenes es 8·20·250·150 (±0.5 %).
  - `Entropia.Fracture.Deterministic`: dos llamadas a `MakeSeeds` con la misma semilla dan los mismos puntos.
- [ ] RED (no compila) → implementar → GREEN → commit `feat: fractura Voronoi (TemporalFracture)`.

### Task 2: Pedazos irregulares en el rompible
**Files:** Create `TemporalShard.h/.cpp` · Modify `TemporalBreakable.h/.cpp`, `entropy.Build.cs` (+`"ProceduralMeshComponent"`), `entropy.uproject` (plugin `ProceduralMeshComponent` habilitado)
**Consumes:** Task 1. **Produces:**
```cpp
UCLASS(NotPlaceable) class ENTROPY_API ATemporalShard : public AActor {
  UProceduralMeshComponent* Mesh; UTemporalRecorderComponent* Recorder;
  void Build(const FFractureCell& Cell, const FVector& CellCentroid, UMaterialInterface* Outer, UMaterialInterface* Inner, float MemorySeconds);
};
UENUM() enum class ETemporalFractureMode : uint8 { Irregular, Cubes };
```
- `Build`: cada cara se triangula en abanico (vértices relativos al centroide, normal de la cara); sección 0 = exteriores y sección 1 = interiores; colisión convexa con `CellVertices − centroide`; `SetSimulatePhysics(true)` después de crear la colisión.
- `ATemporalBreakable::Break` con `Irregular`: `LocalImpact = InverseTransformPosition(ImpactPoint)` (sin escala); semillas con `MakeSeeds(Size/2, NumShards, LocalImpact, GetUniqueID())`; cada celda se vuelve un `ATemporalShard` en `Whole.TransformPosition(Centroid)`; la casa del pedazo es esa posición. Impulsos, enlaces y rearmado como hoy. `Shards` pasa a `TArray<TObjectPtr<AActor>>`.
- `Cubes` conserva el código de hoy.
- Material interior: `TemporalFX::MakeSolid(this, Color * 1.35)`.
- [ ] Compilar y correr las pruebas → verde. Commit `feat: rompibles con fractura irregular (ATemporalShard)`.

### Task 3: Escombros y polvo reversibles
**Files:** Create `TemporalDebrisComponent.h/.cpp` · Modify `TemporalBreakable.cpp` · Test `EntropiaTests.cpp`
**Produces:**
```cpp
struct FDebrisParticle { FVector Origin; FVector Velocity; float Size; float SpinDeg; };
UCLASS(ClassGroup=(Entropia)) class ENTROPY_API UTemporalDebrisComponent : public UActorComponent {
  UPROPERTY(EditAnywhere) int32 NumChips = 14, NumDust = 10; float Life = 2.5f;
  void Burst(const FVector& ImpactPoint, const FVector& Direction, const FLinearColor& Color, float FloorZ);
  void Hide();
  static FVector EvaluateChip(const FDebrisParticle& P, float T, float FloorZ); // balístico, z >= FloorZ + Size/2
  static float DustScale(float T, float Life);                                   // 0 en T=0, máx ~0.25 Life, 0 en Life
};
```
- Tick: el rompible le dice si está rebobinando (`bool bRewinding` lo pone `ATemporalBreakable::Tick`, que ya tickea, revisando `GetShardRecorders()`). Rebobinando: `T = max(0, T - RealDelta)`; si no: `T = min(Life, T + RealDelta)`. Las instancias se actualizan con `UpdateInstanceTransform` (marcando dirty solo en la última).
- [ ] Pruebas:
  - `Entropia.Debris.StartsAtOrigin`: `EvaluateChip(P, 0, -1000) == P.Origin`.
  - `Entropia.Debris.StopsAtFloor`: con T = 10, z = FloorZ + Size/2.
  - `Entropia.Debris.DustScaleShape`: `DustScale(0) == 0`, `DustScale(Life) == 0` y `DustScale(0.25 · Life) > 0.9 · máx`.
- [ ] RED → implementar → enganchar en `Break` y en `Reassemble` (→ `Hide`) → GREEN → commit `feat: escombros y polvo reversibles (ISM analítico)`.

### Task 4: Material de distorsión y `UTemporalFXSubsystem`
**Files:** Create `TemporalFXSubsystem.h/.cpp` · Modify `TemporalFX.cpp` (generar `M_TemporalDistortion` al arrancar el editor, como `M_TemporalGhost`) · Test `EntropiaTests.cpp`
**Produces:**
```cpp
struct FTemporalPulse { FVector World; float Age = 0, Duration = 0.6f, MaxRadius = 0.35f, Strength = 0.02f; };
struct ENTROPY_API FTemporalPulseSet {
  static constexpr int32 MaxPulses = 4; TArray<FTemporalPulse> Pulses;
  void Add(const FTemporalPulse& P);           // si hay 4, reemplaza el de mayor Age
  void Advance(float RealDelta);               // envejece y quita los vencidos
  static float RadiusAt(const FTemporalPulse& P); // MaxRadius * (1 - (1-a)^3), a = Age/Duration
  static float StrengthAt(const FTemporalPulse& P); // Strength * (1 - a)
};
UCLASS() class ENTROPY_API UTemporalFXSubsystem : public UTickableWorldSubsystem {
  void Pulse(const FVector& WorldLocation, float Strength, float Duration = 0.6f, float MaxRadius = 0.35f);
  void SetAura(AActor* Target, float Strength);
};
```
- Material (dominio PostProcess, `BL_SceneColorAfterTonemapping`): un nodo Custom (`CMOT_Float3`) con entradas `Pulse0..3`, `Aura` (VectorParameter) y `Thickness` (Scalar, 0.06), más `Dummy` = SceneTexture(PostProcessInput0). El HLSL calcula la UV de viewport desde `GetDefaultSceneTextureUV(Parameters, 14)`, suma los desplazamientos radiales de cada onda (banda suave de ancho `Thickness`) y del aura (`sin` con `View.RealTime`) y lee R, G y B con tres desplazamientos distintos (aberración). Se conecta a EmissiveColor.
- El subsystem crea en `OnWorldBeginPlay` un `AActor` con `UPostProcessComponent` (`bUnbound`, `Priority = 3`) y le agrega el MID (`Settings.WeightedBlendables`). En Tick, si la CVar está en 0 o no hay nada activo → `BlendWeight = 0`. Si no, proyecta con `PC->ProjectWorldLocationToScreen` / tamaño del viewport y escribe los parámetros. Si falta el PC, peso 0.
- [ ] Pruebas:
  - `Entropia.Pulse.RadiusGrows`: con `Age = 0` el radio es 0; con `Age = Duration` es `MaxRadius`; a la mitad, entre 0.8 y 0.95 × `MaxRadius`.
  - `Entropia.Pulse.ReplacesOldest`: con 5 `Add` quedan 4 y el más viejo ya no está.
  - `Entropia.Pulse.Expires`: `Advance` más allá de `Duration` → 0 ondas.
- [ ] RED → implementar → GREEN → commit `feat: distorsión del tiempo (M_TemporalDistortion + UTemporalFXSubsystem)`.

### Task 5: Enganchar la distorsión a los poderes y a la destrucción
**Files:** Modify `InverseStepComponent.cpp`, `OverdriveComponent.cpp`, `ObjectInversionComponent.cpp`, `TemporalSandbox.cpp` (burbuja), `TemporalBreakable.cpp`
**Consumes:** `UTemporalFXSubsystem::Pulse/SetAura`.

| Evento | Llamada |
|---|---|
| F empieza | `Pulse(jugador, -0.03, 0.5, 0.4)` + `SetAura(jugador, 0.012)` |
| F termina | `SetAura(nullptr, 0)` + `Pulse(jugador, 0.015, 0.35, 0.2)` |
| V enciende | `Pulse(jugador, 0.04, 0.8, 0.8)` |
| V apaga | `Pulse(jugador, -0.02, 0.4, 0.3)` |
| R empieza | `Pulse(objetivo, -0.025, 0.5, 0.25)` |
| Q burbuja | `Pulse(burbuja, 0.03, 0.6, 0.35)` |
| Romper | `Pulse(impacto, 0.035, 0.45, 0.3)` |
| Rearmar | `Pulse(centro, -0.03, 0.45, 0.25)` |

- [ ] Compilar y correr las pruebas → verde. Commit `feat: distorsión en F, V, R, Q, romper y rearmar`.

### Task 6: Modo tour y prueba por la IA
**Files:** Modify `TemporalSandbox.h/.cpp`, `ObjectInversionComponent.h/.cpp` (`bool BeginInversionOn(UTemporalRecorderComponent* Target)`), `Tools/run-tour.ps1` (nuevo)
- Tour con `FTimerHandle` encadenados y tiempo real (`FPlatformTime::Seconds`), pasos de ~2.5 s: los de la sección 6.5 del diseño. FPS: media y mínimo de `1/RealDelta` acumulados entre pasos (un `Tick` del Sandbox habilitado solo en el tour). Captura: `FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("Tour_%02d_%s"), N, Nombre), false, false)`. Al final, `ENTROPIA_TOUR done` y `FGenericPlatformMisc::RequestExit(false)`.
- `Tools/run-tour.ps1`: lanza `UnrealEditor.exe <uproject> /Game/ThirdPerson/Lvl_ThirdPerson -game -windowed -ResX=1280 -ResY=720 -EntropiaTour -log -ABSLOG=Saved\BuildTest\tour.log`, espera a que termine (timeout 240 s), imprime las líneas `ENTROPIA_TOUR` y los errores, y lista las capturas.
- [ ] Compilar y correr las pruebas → verde. Correr el tour → `done` presente y capturas generadas. Commit `feat: modo tour (-EntropiaTour) para pruebas automáticas en juego`.

### Task 7: Revisión del tour, afinado y entrega
- [ ] Ver las capturas y los FPS; arreglar lo que salga mal (cada arreglo con su prueba si es lógica).
- [ ] Actualizar `HANDOFF.md` (clases nuevas, Fase 2 en pruebas, teclas, cómo correr el tour) y la sección 15.
- [ ] Revisión final de la rama y fusión cuando Hugo apruebe.
