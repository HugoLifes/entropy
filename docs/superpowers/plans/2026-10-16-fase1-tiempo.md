# Fase 1 · Pulir el tiempo — Plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Energía compartida con barra en pantalla, Paso Inverso más ligero y suave, Invertir objeto (R apuntado), Sobremarcha (V) y Eco suave.

**Architecture:** Componentes nuevos en el jugador (`UTemporalEnergyComponent`, `UTemporalTrailComponent`, `UObjectInversionComponent`, `UOverdriveComponent`) que agrega `ATemporalSandbox`, igual que hoy agrega Recorder e InverseStep. Un `UTemporalSubsystem` lleva la lista de Recorders. La lógica pura se prueba con Automation Tests; lo visual lo prueba Hugo en Play.

**Tech Stack:** Unreal 5.8 C++ (módulo `entropy`, estructura plana en `Source/entropy/`), UMG/Slate (`NativePaint`), Automation Tests.

**Spec:** `docs/superpowers/specs/2026-10-16-fase1-tiempo-design.md`

## Global Constraints
- Todo el código en `Source/entropy/` (plano, sin Public/Private). Comentarios en español, estilo de los archivos existentes.
- Sin assets nuevos hechos a mano; materiales con `TemporalFX::MakeTranslucent` / `MakeSolid` (`M_TemporalGhost`).
- Teclas con `InputComponent->BindKey` en `ATemporalSandbox`.
- Nada de nombres genéricos en namespaces anónimos (unity build).
- Energía: máx **100**, recarga **20/s** tras **1 s** sin gastar, medida en tiempo real (`FApp::GetDeltaTime()`).
- Costos: F **34/s** (mín 15) · R **15/s** (mín 10) · V **25/s** (mín 20) · Q **30** de golpe · E **10** de golpe.
- Estela: pool **4**, intervalo **0.09 s**, vida **0.30 s**, color F `(0.25, 0.45, 0.95, 0.22)`, color V `(0.2, 0.9, 0.8, 0.2)`.
- Apuntar: ángulo < **4°**, distancia < **3500 cm**, resaltado ámbar alfa **0.35**.
- Sobremarcha: dilatación global **0.25**, jugador `CustomTimeDilation` **4**.

**Comando de compilación (editor cerrado):**
```
& 'D:\unreal projets\UE_5.8\Engine\Build\BatchFiles\Build.bat' entropyEditor Win64 Development "-Project=D:\unreal projets\entropy\entropy.uproject" -WaitMutex -NoHotReloadFromIDE
```
Esperado: `Result: Succeeded`, sin `warning C`.

**Comando de pruebas (editor cerrado):**
```
& 'D:\unreal projets\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\unreal projets\entropy\entropy.uproject' -ExecCmds="Automation RunTests Entropia;Quit" -unattended -nopause -nullrhi -nosplash -NoSound -log
```
Esperado en la salida: `Test Completed. Result={Success}` para cada prueba `Entropia.*` y ninguna `Result={Fail}`.

## Review Focus
1. **La energía se acaba a mitad de un poder** → F, R y V terminan solos y dejan todo como estaba (dilatación en 1, post-process en 0, objetos sin rebobinar). Prueba: `Entropia.Energy.DrainStopsAtZero` (Task 1) y comprobar en Play (Task 9).
2. **El objetivo de R desaparece a mitad del rebobinado** (la bala vuelve al cañón y se destruye; el rompible se rearma y destruye sus pedazos) → sin crash, R termina limpio. Prueba: `Entropia.Group.IgnoresDestroyed` (Task 2) y weak pointers en Task 6.
3. **Salir de Play o teletransportarse (1/2/3/4/0) con V o R activos** → `EndPlay` deja la dilatación global en 1; teletransportar suelta R (V puede seguir). Cubierto en Tasks 6 y 7.
4. **Encender F con V activo (o al revés)** → se apaga el otro antes; nunca quedan los dos post-process ni dilataciones mezcladas. Cubierto en Task 7.
5. **Pedir un poder sin energía suficiente** → no se activa, la barra parpadea en rojo y no se gasta nada. Prueba: `Entropia.Energy.SpendFailsWhenShort` (Task 1).

---

### Task 0: Infraestructura de pruebas
**Files:**
- Create: `Source/entropy/EntropiaTests.cpp`

- [ ] **Step 1:** Crear el archivo con `#if WITH_DEV_AUTOMATION_TESTS` y una prueba `IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEntropiaSmokeTest, "Entropia.Smoke", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)` que haga `TestTrue("smoke", true)`.
- [ ] **Step 2:** Compilar (comando arriba) → `Result: Succeeded`.
- [ ] **Step 3:** Correr las pruebas → `Entropia.Smoke ... Result={Success}`. Si el filtro no encuentra la prueba, revisar los flags (`ApplicationContextMask`) antes de seguir.
- [ ] **Step 4:** Commit `test: infraestructura de Automation Tests (Entropia.*)`.

### Task 1: Energía compartida
**Files:**
- Create: `Source/entropy/TemporalEnergyComponent.h/.cpp`
- Test: `Source/entropy/EntropiaTests.cpp`

**Interfaces — Produces:**
```cpp
UCLASS(ClassGroup=(Entropia), meta=(BlueprintSpawnableComponent))
class ENTROPY_API UTemporalEnergyComponent : public UActorComponent {
  UPROPERTY(EditAnywhere) float MaxEnergy = 100.f, RegenPerSecond = 20.f, RegenDelay = 1.f;
  UPROPERTY(BlueprintReadOnly) float Energy = 100.f;
  bool CanAfford(float Amount) const;                 // Energy >= Amount
  bool Spend(float Amount);                           // gasto de golpe; false (y no gasta) si no alcanza
  bool Drain(float PerSecond, float RealDeltaTime);   // gasto continuo; false cuando llega a 0
  void Deny();                                        // DenyFlash = 0.3
  void Advance(float RealDeltaTime);                  // recarga y timers; lo llama TickComponent con FApp::GetDeltaTime()
  float GetFraction() const;
  float GetDenyFlash() const;
  void SetActivePower(FName Power);                   // NAME_None = reposo
  FName GetActivePower() const;
};
```
- `Spend`/`Drain` reinician el contador de recarga. `TickComponent` llama `Advance(FApp::GetDeltaTime())`, **nunca** usa el DeltaTime recibido.

- [ ] **Step 1: Pruebas** (en `EntropiaTests.cpp`, con `NewObject<UTemporalEnergyComponent>()` sin mundo):
  - `Entropia.Energy.SpendFailsWhenShort`: Energy=20 → `Spend(30)` es false y Energy sigue en 20; `Spend(10)` es true → 10.
  - `Entropia.Energy.DrainStopsAtZero`: Energy=10 → `Drain(34, 0.5)` es false y Energy == 0 (no negativa).
  - `Entropia.Energy.RegenAfterDelay`: Energy=50, `Spend(0.1)` → `Advance(0.5)` no recarga; `Advance(0.6)` (ya pasó 1.1 s) recarga > 50; muchos `Advance(1)` nunca pasan de 100.
- [ ] **Step 2:** Compilar y correr → las 3 pruebas fallan o no compilan (la clase no existe).
- [ ] **Step 3:** Implementar `UTemporalEnergyComponent`.
- [ ] **Step 4:** Compilar y correr → las 3 en `Result={Success}`.
- [ ] **Step 5:** Commit `feat: UTemporalEnergyComponent (energía compartida)`.

### Task 2: Subsystem de Recorders, grupos y tramo de rebobinado
**Files:**
- Create: `Source/entropy/TemporalSubsystem.h/.cpp`
- Modify: `Source/entropy/TemporalRecorderComponent.h/.cpp`
- Test: `Source/entropy/EntropiaTests.cpp`

**Interfaces — Produces:**
```cpp
UCLASS() class ENTROPY_API UTemporalSubsystem : public UWorldSubsystem {
  void Register(UTemporalRecorderComponent*); void Unregister(UTemporalRecorderComponent*);
  TArray<UTemporalRecorderComponent*> GetRecorders() const;  // solo válidos
};
// En UTemporalRecorderComponent:
void LinkWith(UTemporalRecorderComponent* Other);             // enlace en ambos sentidos (TWeakObjectPtr)
void GatherGroup(TSet<UTemporalRecorderComponent*>& OutGroup); // BFS; incluye this; ignora inválidos
bool GetRewindSegment(const FTemporalFrame*& OutFrom, const FTemporalFrame*& OutTo, float& OutAlpha) const;
// true solo si IsRewinding() y hay frame anterior: From = LastRewoundFrame, To = Frames.Last(), Alpha = el mismo de RewindStep
```
- `BeginPlay`: `GetWorld()->GetSubsystem<UTemporalSubsystem>()->Register(this)`; `EndPlay`: `Unregister`.
- `RewindStep` guarda el Alpha que ya calcula en un miembro `RewindAlpha` para `GetRewindSegment`.

- [ ] **Step 1: Pruebas** (Recorders con `NewObject`, sin mundo):
  - `Entropia.Group.Transitive`: A↔B, B↔C → `GatherGroup` de A = {A, B, C}; de C = {A, B, C}.
  - `Entropia.Group.IgnoresDestroyed`: A↔B, `B->MarkAsGarbage()` → el grupo de A = {A}.
- [ ] **Step 2:** Compilar y correr → fallan.
- [ ] **Step 3:** Implementar subsystem, registro, `LinkWith`, `GatherGroup` y `GetRewindSegment`.
- [ ] **Step 4:** Cambiar `ATemporalSandbox::GatherRewindables` para usar `UTemporalSubsystem::GetRecorders()` (dueños ≠ jugador) en vez de `TActorIterator`.
- [ ] **Step 5:** Compilar y correr → pasan; las anteriores siguen pasando.
- [ ] **Step 6:** Commit `feat: UTemporalSubsystem, grupos de rebobinado y tramo de rebobinado`.

### Task 3: Estela ligera con pool
**Files:**
- Create: `Source/entropy/TemporalTrailComponent.h/.cpp`

**Interfaces — Produces:**
```cpp
UCLASS(ClassGroup=(Entropia)) class ENTROPY_API UTemporalTrailComponent : public UActorComponent {
  UPROPERTY(EditAnywhere) int32 PoolSize = 4; float Life = 0.30f;
  void Emit(USkinnedAsset* Asset, const TArray<FTransform>& Pose, const FTransform& MeshWorldTransform, const FLinearColor& Color);
};
```
- Crea los `PoolSize` `UPoseableMeshComponent` de forma perezosa en el primer `Emit` (con el asset recibido), registrados, `SetUsingAbsoluteLocation/Rotation/Scale(true)`, sin colisión ni sombra, invisibles, un MID por slot (`TemporalFX::MakeTranslucent`). `Emit` usa el slot con más edad. El Tick baja el alfa con `(1 - Age/Life)²` y oculta el slot al llegar a `Life`. El Tick usa el DeltaTime del componente (el del dueño).
- Si el número de huesos no coincide con la pose, el slot se muestra en su pose de referencia (como hoy).

- [ ] **Step 1:** Implementar.
- [ ] **Step 2:** Compilar → `Result: Succeeded`, sin warnings.
- [ ] **Step 3:** Commit `feat: UTemporalTrailComponent (estela con pool fijo)`.
(Lo visual se verifica en Task 9.)

### Task 4: Paso Inverso con energía, estela, post-process propio y pose suave
**Files:**
- Modify: `Source/entropy/InverseStepComponent.h/.cpp`

**Interfaces — Consumes:** `UTemporalEnergyComponent` (Task 1), `UTemporalTrailComponent::Emit` (Task 3), `UTemporalRecorderComponent::GetRewindSegment` (Task 2).
**Produces:** `bool BeginInverseStep()` (devuelve si arrancó); `EndInverseStep()` sin cambios.

- [ ] **Step 1:** Quitar `MaxEnergy/Energy/Drain/Regen/MinEnergyToStart` propios; usar el `UTemporalEnergyComponent` del dueño (`FindComponentByClass` en BeginPlay). Nuevos `UPROPERTY`: `CostPerSecond = 34`, `MinToStart = 15`. Sin energía suficiente → `Energy->Deny()` y no arranca. Mientras está activo: `Drain(CostPerSecond, FApp::GetDeltaTime())`; si da false → `EndInverseStep()`. `SetActivePower("Inverso")` al empezar y `NAME_None` al terminar.
- [ ] **Step 2:** Post-process: crear en BeginPlay un `UPostProcessComponent` (`bUnbound = true`, `Priority = 1`) con los mismos ajustes de `ApplyCameraEffect` y cambiar solo su `BlendWeight`. Dejar de escribir en `Camera->PostProcessSettings`. El FOV kick se queda en la cámara.
- [ ] **Step 3:** Estela: reemplazar `SpawnAfterimage` por `Trail->Emit(...)` con `AfterimageInterval = 0.09`, color F. `bShowDebugHud = false` por defecto.
- [ ] **Step 4:** Pose suave: en `ApplyReversePose`, si `GetRewindSegment` da true y From/To tienen el mismo número de huesos que la marioneta, mezclar hueso por hueso (`BlendWith(To[i], Alpha)` sobre una copia de From); si no, usar `GetLastRewoundFrame().Pose` como hoy.
- [ ] **Step 5:** Compilar y correr las pruebas → todo `Success`, sin warnings.
- [ ] **Step 6:** Commit `feat: Paso Inverso con energía compartida, estela ligera, post-process propio y pose suave`.

### Task 5: Barra de energía (UMG) y costos de Q/E
**Files:**
- Create: `Source/entropy/TemporalHUDWidget.h/.cpp`
- Modify: `Source/entropy/TemporalSandbox.h/.cpp`, `Source/entropy/entropy.Build.cs` (agregar `"SlateCore"`)

**Interfaces — Produces:** `UTemporalHUDWidget : UUserWidget` con `void SetEnergy(UTemporalEnergyComponent*)` y `NativePaint`.
- Dibujo: barra 320×14 px centrada abajo (margen inferior 40 px), fondo `(0,0,0,0.55)`, relleno según `GetActivePower()`: `Inverso` azul `(0.3,0.6,1)`, `Inversion` ámbar `(1,0.65,0.15)`, `Sobremarcha` cian `(0.2,0.9,0.8)`, reposo blanco `(0.9,0.9,0.9)`. Con `GetDenyFlash() > 0`, el relleno se dibuja rojo `(1,0.15,0.1)`. Texto `ENERGÍA` encima con `FCoreStyle::GetDefaultFontStyle("Bold", 10)`.
- Sandbox `Setup`: agregar `UTemporalEnergyComponent` al jugador si no tiene (antes que InverseStep), crear el widget con `CreateWidget<UTemporalHUDWidget>(PC, UTemporalHUDWidget::StaticClass())`, `SetEnergy` y `AddToViewport`.
- `OnBubble`: `if (!Energy->Spend(30)) { Energy->Deny(); return; }`. `OnEcho`: igual con 10, después de las validaciones que ya existen.

- [ ] **Step 1:** Implementar el widget y el cableado.
- [ ] **Step 2:** Compilar y correr las pruebas → `Success`.
- [ ] **Step 3:** Commit `feat: barra de energía (UMG) y costos de burbuja y eco`.

### Task 6: Invertir objeto (R apuntado) + Shift+R
**Files:**
- Create: `Source/entropy/ObjectInversionComponent.h/.cpp`
- Modify: `TemporalBreakable.h/.cpp`, `TemporalProjectile.cpp`, `TemporalSandbox.h/.cpp`, `TemporalTestChamber.cpp` (pista de la sala 2), `TemporalPhysicsLab.cpp` (pista de la sala 4)

**Interfaces — Consumes:** `UTemporalSubsystem::GetRecorders`, `LinkWith`, `GatherGroup` (Task 2); `UTemporalEnergyComponent` (Task 1).
**Produces:**
```cpp
UCLASS(ClassGroup=(Entropia)) class ENTROPY_API UObjectInversionComponent : public UActorComponent {
  UPROPERTY(EditAnywhere) float MaxAngleDegrees = 4.f, MaxDistance = 3500.f, CostPerSecond = 15.f, MinToStart = 10.f;
  bool BeginInversion();   // false + Deny si no hay objetivo o energía
  void EndInversion();
  bool IsInverting() const;
};
// ATemporalBreakable:
TArray<UTemporalRecorderComponent*> GetShardRecorders() const;
```
- Apuntar (Tick, solo si no está invirtiendo): vista = `PC->GetPlayerViewPoint`. De los Recorders del subsystem (dueño ≠ jugador), quedarse con el de menor ángulo que cumpla < 4° y < 3500 cm, con un trace `ECC_Visibility` desde la vista hasta el dueño (ignorando al jugador y al dueño) que no choque con nada. Resaltar su grupo con `SetOverlayMaterial` (MID ámbar de `TemporalFX::MakeTranslucent`, alfa 0.35) en todos sus `UMeshComponent`; quitar el resaltado anterior (`SetOverlayMaterial(nullptr)`). Guardar los componentes resaltados con `TWeakObjectPtr`.
- `BeginInversion`: grupo fijo (`TArray<TWeakObjectPtr<UTemporalRecorderComponent>>`), `StartRewind` en cada uno, `SetActivePower("Inversion")`. Tick: `Drain(15, FApp::GetDeltaTime())`; termina si da false, si ninguno del grupo sigue `IsRewinding()` o si todos son inválidos. `EndInversion`: `StopRewind` en los válidos, `SetActivePower(NAME_None)`.
- `ATemporalBreakable::Break`: después de crear los pedazos, enlazar cada Recorder de pedazo con el primero (`LinkWith`). `ATemporalProjectile::StepForward`: después de `Breakable->Break(...)`, `Recorder->LinkWith(Shard)` para cada uno de `GetShardRecorders()`.
- Sandbox: agregar el componente al jugador. Handler de R pulsado: si `PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)` → lo de hoy (todos, sin costo, marcando `bRewindingAll = true`); si no → `BeginInversion()`. Al soltar R: si `bRewindingAll` → `StopRewind` a todos; si no → `EndInversion()`. `GoToRoom`, `OnRoom4` y `OnBackToStart` llaman `EndInversion()`.
- **Exclusión F/R:** `BeginInversion` llama `EndInverseStep()` si F está activo; `BeginInverseStep` llama `EndInversion()` si R está activo.
- Sala 2: cada plataforma es su propio objetivo (sin enlaces).
- Pistas: sala 2 → "apunta a una plataforma caída y MANTÉN R"; sala 4 → "apunta a la bala y MANTÉN R (vuelve al cañón y rearma lo que rompió) · Shift+R rebobina todo".
- Ayuda general (`ShowHelp`): `R (mantener): invertir lo que apuntas · Shift+R: todo · V: Sobremarcha`.

- [ ] **Step 1:** Implementar el componente, los enlaces y el cableado.
- [ ] **Step 2:** Compilar y correr las pruebas → `Success`.
- [ ] **Step 3:** Commit `feat: Invertir objeto (R apuntado con grupos) y Shift+R para todo`.

### Task 7: Sobremarcha (V)
**Files:**
- Create: `Source/entropy/OverdriveComponent.h/.cpp`
- Modify: `TemporalSandbox.h/.cpp`, `InverseStepComponent.cpp`

**Interfaces — Consumes:** Energía (Task 1), estela (Task 3).
**Produces:**
```cpp
UCLASS(ClassGroup=(Entropia)) class ENTROPY_API UOverdriveComponent : public UActorComponent {
  UPROPERTY(EditAnywhere) float WorldDilation = 0.25f, CostPerSecond = 25.f, MinToStart = 20.f, TrailInterval = 0.09f;
  UPROPERTY(EditAnywhere) FLinearColor TrailColor = FLinearColor(0.2f, 0.9f, 0.8f, 0.2f);
  void Toggle(); bool Activate(); void Deactivate(); bool IsActive() const;
};
```
- `Activate`: si `UInverseStepComponent::IsInverseActive()` → `EndInverseStep()`. `SetGlobalTimeDilation(WorldDilation)`, `Owner->CustomTimeDilation = 1/WorldDilation`, post-process propio (`bUnbound`, `Priority = 2`): saturación 0.5, `ColorGain (0.8, 1.1, 1.05)`, `SceneFringeIntensity` 2.5, viñeta 0.6. El `BlendWeight` sube y baja con `FInterpTo` en tiempo real (12 al entrar, 6 al salir). Estela cada `TrailInterval` s reales con la pose actual del cuerpo (`GetMesh()->GetBoneSpaceTransforms()`).
- `Deactivate` y `EndPlay`: dilatación global 1, `CustomTimeDilation` 1.
- Tick: `Drain(25, FApp::GetDeltaTime())`; si da false → `Deactivate()`.
- `UInverseStepComponent::BeginInverseStep`: si el dueño tiene `UOverdriveComponent` activo → `Deactivate()` antes de empezar.
- Sandbox: agregar el componente al jugador, `BindKey(EKeys::V, IE_Pressed, ..., OnOverdrive)` → `Toggle()`.

- **Tiempo real durante V:** en `UTemporalRecorderComponent::TickComponent`, el caso Rewinding usa `FMath::Max(DeltaTime, FApp::GetDeltaTime())` en lugar de `GetWorld()->GetDeltaSeconds()`. `ATimeBubble`: quitar `SetLifeSpan`; `Age += FApp::GetDeltaTime()` en Tick y `Destroy()` cuando `LifeSeconds > 0 && Age >= LifeSeconds`.

- [ ] **Step 1:** Implementar.
- [ ] **Step 2:** Compilar y correr las pruebas → `Success`.
- [ ] **Step 3:** Commit `feat: Sobremarcha (V): mundo lento, jugador normal, estela cian`.

### Task 8: Eco con pose suave
**Files:**
- Modify: `Source/entropy/EchoGhost.cpp`

- [ ] **Step 1:** En el bloque que ya mezcla la posición, mezclar también la pose: si `Frames[FrameIndex-1].Pose` y `Frames[FrameIndex].Pose` tienen el mismo número de huesos que `PoseMesh->BoneSpaceTransforms`, copiar la primera, hacer `BlendWith` hueso por hueso con `Alpha` y `MarkRefreshTransformDirty()`. `ApplyFrame` sigue igual para los saltos de bucle.
- [ ] **Step 2:** Compilar y correr las pruebas → `Success`.
- [ ] **Step 3:** Commit `feat: Eco con pose mezclada entre frames`.

### Task 9: Verificación final, documentación y entrega
- [ ] **Step 1:** Compilar el target del juego (`entropy Win64 Development`) → `Result: Succeeded`, sin warnings.
- [ ] **Step 2:** Correr todas las pruebas `Entropia.*` → todas `Success`.
- [ ] **Step 3:** Actualizar `HANDOFF.md` (tabla de clases, Estado, nueva sección de la Fase 1 con la lista de pruebas para Hugo) y marcar la Fase 1 como "🟡 en pruebas" en la sección 15.
- [ ] **Step 4:** `git push`.
- [ ] **Step 5:** Pedir a Hugo que abra el editor y pruebe: F (comparar `stat fps` con el valor de antes), R apuntando en las salas 2 y 4, Shift+R, V (y F durante V), Q/E con poca energía (parpadeo rojo), Eco suave. Después, revisar el log por el MCP (`GetLogEntries` con `Error|Warning`).
