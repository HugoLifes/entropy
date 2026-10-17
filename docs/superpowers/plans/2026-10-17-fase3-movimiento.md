# Fase 3 · Moverse, reflejos y remates — Plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Cámara sobre el hombro con mira que selecciona, Detener (clic izquierdo), dash, vida, sala 5 con torretas que te siguen, esquiva perfecta con tiempo de reflejo, Ojo lento y remates a un maniquí temporal.

**Architecture:** Un árbitro del tiempo (`UTemporalDilationSubsystem`) reemplaza las escrituras directas a `CustomTimeDilation` y a la dilatación global: cada poder pide y suelta velocidades, gana la más lenta. Cada poder nuevo es un `UActorComponent` del jugador que el Sandbox agrega (`FindOrAddComponent`), igual que en la Fase 1. La lógica pura (árbitro, vida, dash, geometría bala-cápsula, condición de remate) va en funciones estáticas probadas con Automation Tests; lo visual se prueba con el tour.

**Tech Stack:** UE 5.8 C++, CharacterMovement, montajes de la plantilla (`AM_Dash`, `AM_ComboAttack`), ragdoll con `PA_Mannequin`, Slate (HUD en NativePaint), Automation Tests, modo tour.

**Spec:** `docs/superpowers/specs/2026-10-17-fase3-movimiento-design.md`

## Global Constraints
- `Source/entropy/` plano. Comentarios en español. Sin assets hechos a mano. No tocar `Variant_*` ni `UE_5.8`.
- Nada de nombres genéricos en namespaces anónimos (unity build): usar namespaces con nombre (`EntropiaReflex`, `TemporalCombat`…).
- Tiempo real de los poderes: `TemporalFX::RealDelta()`.
- Nadie escribe `CustomTimeDilation` ni `SetGlobalTimeDilation` fuera de `UTemporalDilationSubsystem`.
- Valores: Detener 4 s / 15 energía / máx. 3 · Dash 1400 cm/s, 0.18 s, recarga 0.6 s, 1 en el aire · i-frames 0.25 s · ventana de esquiva 0.35 s · reflejo 0.3 durante 1.5 s · Ojo lento 350 cm, 0.25, 6/s · vida 100, bala 20, regen tras 4 s a 25/s, protección 1.5 s · remate 180 cm, 120°, 0.2 durante 0.4 s · maniquí se levanta a los 4 s · torretas sala 5: 1800 cm/s, cada 1.2 s, giro 120°/s.
- Teclas: RMB apuntar · LMB rematar o Detener · R invertir · Shift dash · Ctrl+R rebobinar todo · 5 sala 5.
- Verificación de cada tarea: `powershell -ExecutionPolicy Bypass -File Tools\build-test.ps1` → `Result: Succeeded`, sin `warning C`, `TESTS: N ok, 0 fail`. Las tareas con pasos de tour además: `Tools\run-tour.ps1` y `Tools\run-tour.ps1 -ExtraArgs '-EntropiaTour=caos'` terminan en `done`, sin `Error`/`Ensure`/`Accessed None`, y los actores/registros del caos vuelven a su base.

## Review Focus
1. **Tiempos lentos encimados** (bala dentro de una burbuja + Ojo lento + Detener; Sobremarcha + reflejo + remate) → nadie descongela lo que otro congeló; el mundo y el jugador vuelven a 1 cuando todos sueltan. Prueba: `Entropia.Dilation.*` + paso caos `todo_lento_junto`.
2. **El actor muere con pedidos vivos** (bala detenida que la torreta borra por `MaxBullets`, burbuja que expira, jugador que reaparece) → el árbitro limpia los punteros muertos sin tronar. Paso caos `detener_x6_y_borrar`.
3. **Morir a media acción** (durante el dash, el Paso Inverso, el remate o con R mantenida) → reapareces de pie, vida llena, sin poderes pegados ni cámara lenta pegada. Paso caos `morir_en_todo`.
4. **Apuntar a una bala que se mueve rápido** → con el Ojo lento la bala entra en la mira (cono de 4°) y Detener la congela. Paso tour `detener_bala`.
5. **Remate a un maniquí congelado dentro de una burbuja** → el golpe se guarda y sale al terminar la burbuja; nunca queda tirado para siempre ni se levanta congelado. Paso tour `remate_congelado`.

---

### Task 1: Árbitro del tiempo
**Files:** Create `TemporalDilation.h/.cpp` · Modify `TimeBubble.h/.cpp`, `OverdriveComponent.cpp` · Test `EntropiaTests.cpp`
**Produces:**
```cpp
/** Pedidos de velocidad por clave y por fuente; gana el más lento. Lógica pura. */
struct ENTROPY_API FTemporalDilationLedger {
  bool Request(const void* Key, const void* Source, float Dilation); // true si cambió el efectivo
  bool Release(const void* Key, const void* Source);
  TArray<const void*> ReleaseSource(const void* Source);             // claves que cambiaron
  float Get(const void* Key) const;                                   // 1 si no hay pedidos
  bool Has(const void* Key) const;
  void RemoveKey(const void* Key);
};
UCLASS() class ENTROPY_API UTemporalDilationSubsystem : public UTickableWorldSubsystem {
  static UTemporalDilationSubsystem* Get(const UObject* WorldContext);
  void Request(AActor* Actor, const void* Source, float Dilation);
  void Release(AActor* Actor, const void* Source);
  void ReleaseSource(const void* Source);
  float GetDilation(const AActor* Actor) const;        // efectivo propio (sin compensación)
  bool IsFrozen(const AActor* Actor) const;            // < 0.05
  void RequestGlobal(const void* Source, float WorldDilation);
  void ReleaseGlobal(const void* Source);
  float GetGlobalDilation() const;
  static float PlayerTimeFor(float OwnDilation, float GlobalDilation); // Own / Global
};
```
Física (movida de `ATimeBubble`): al pasar a < 0.05 guarda velocidades y apaga la simulación; en cámara lenta quita la gravedad, escala la velocidad y en Tick aplica gravedad × d²; al volver la restaura. El jugador recibe `PlayerTimeFor`. Tick limpia actores muertos.
- [ ] Test `Entropia.Dilation.SlowestWins`: Request(K,A,0.25) → Get 0.25; Request(K,B,0) → 0; Release(K,B) → 0.25; Release(K,A) → 1 y `Has` false.
- [ ] Test `Entropia.Dilation.ReleaseSource`: A pide K1=0.5 y K2=0; B pide K2=0.3; ReleaseSource(A) devuelve {K1,K2}; Get(K1)=1, Get(K2)=0.3.
- [ ] Test `Entropia.Dilation.PlayerCompensation`: PlayerTimeFor(1,0.25)=4; PlayerTimeFor(0,0.25)=0; PlayerTimeFor(1,1)=1.
- [ ] Correr → fallan (no compila / no existe). Implementar. Correr → pasan.
- [ ] `ATimeBubble`: Affect/Release usan el árbitro con Source = la burbuja; EndPlay `ReleaseSource`. Se borran FrozenBodies/SlowedBodies y el bucle de gravedad. Agregar `TWeakObjectPtr<AActor> OnlyTarget` (solo afecta a ese actor; si lo suelta se destruye) y `bool bDrawField = true`.
- [ ] `UOverdriveComponent`: `RequestGlobal(this, WorldDilation)` / `ReleaseGlobal(this)`.
- [ ] build-test + tour normal + tour caos (burbuja, Sobremarcha) → limpios. Commit `feat: árbitro del tiempo (gana el más lento)`.

### Task 2: Apuntar, cámara al hombro y mira
**Files:** Create `TemporalAimComponent.h/.cpp` · Modify `ObjectInversionComponent.h/.cpp`, `TemporalSubsystem.h/.cpp`, `TemporalHUDWidget.h/.cpp`, `TemporalSandbox.h/.cpp`, `EntropiaTests.cpp`
**Produces:**
```cpp
UCLASS() class UTemporalAimComponent : public UActorComponent {
  float ArmLength = 300, AimArmLength = 150; FVector ShoulderOffset = {0,55,45}, AimShoulderOffset = {0,65,55};
  float AimWalkSpeed = 320; float MaxAngleDegrees = 4; float MaxDistance = 3500; FLinearColor HighlightColor;
  void SetAiming(bool); bool IsAiming() const;
  AActor* FindTarget() const;              // nueva búsqueda (cámara → mira)
  AActor* GetHoveredTarget() const;        // la del frame (resaltada)
  void LockTarget(AActor*); void UnlockTarget(); // el resaltado se queda fijo (mientras inviertes)
  static bool IsBetterAim(float EdgeA, float CenterA, float EdgeB, float CenterB); // movida
};
// UTemporalSubsystem: RegisterTimeTarget(AActor*), UnregisterTimeTarget(AActor*), GetTimeTargets()
```
Candidatos = dueños de Registros + objetivos de tiempo registrados. La búsqueda y el resaltado del grupo se mudan desde `UObjectInversionComponent` (que ahora pide el objetivo al Aim y lo bloquea mientras invierte). Al apuntar: `bOrientRotationToMovement=false`, `bUseControllerDesiredRotation=true`; la velocidad de caminar se escribe cada frame salvo durante el dash. HUD: mira en el centro (blanca / ámbar / cian si `IsFrozen`).
- [ ] Mover la prueba `Entropia.Aim.PrefersCrosshairOverBigNearby` a `UTemporalAimComponent::IsBetterAim` → falla (no existe) → mover el código → pasa.
- [ ] Sandbox: RMB apuntar, Ctrl+R rebobinar todo (Shift ya no). Paso tour `apuntar` (RMB + mirar a una caja).
- [ ] build-test + tours. Commit `feat: cámara al hombro, apuntar y mira que selecciona`.

### Task 3: Detener (clic izquierdo)
**Files:** Create `TimeStopComponent.h/.cpp` · Modify `TemporalSandbox.cpp`, `TemporalHUDWidget.cpp`
**Produces:** `UTimeStopComponent { float Cost=15, StopSeconds=4; int32 MaxStopped=3; bool StopAimed(); bool StopActor(AActor*); void ReleaseActor(AActor*); void ReleaseAll(); bool IsStoppedByMe(const AActor*) const; int32 NumStopped() const; }`
Cada detención = `ATimeBubble` con `OnlyTarget`, `Dilation 0`, `bDrawField=false`, radio = clamp(extensión × 1.1, 25, 250). Clic sobre algo detenido por ti = soltarlo (sin costo). La cuarta suelta la más vieja. Color de poder `Detener` en la barra.
- [ ] Pasos tour `detener_bala` (disparo + StopActor a la bala más nueva; la captura la muestra quieta), `detener_suelta`. Caos `detener_x6_y_borrar` (6 detenciones seguidas + `ClearBullets`).
- [ ] build-test + tours. Commit `feat: Detener lo que tienes en la mira`.

### Task 4: Dash
**Files:** Create `TemporalDashComponent.h/.cpp` · Modify `TemporalSandbox.cpp`, `EntropiaTests.cpp`
**Produces:**
```cpp
UCLASS() class UTemporalDashComponent : public UActorComponent {
  float DashSpeed=1400, DashSeconds=0.18f, Cooldown=0.6f, InvulnerableSeconds=0.25f;
  bool TryDash(); bool IsDashing() const;
  static FVector ComputeDashDirection(const FVector& MoveInput, const FVector& Facing); // plano XY, normalizado; si MoveInput≈0 usa Facing
  static bool CanDash(float SinceLastDash, float InCooldown, bool bFalling, bool bAirDashUsed);
};
```
Durante el dash: velocidad fija cada tick (el movimiento del personaje tickea después: `AddTickPrerequisiteComponent`), sin fricción ni gravedad; al terminar sale a la velocidad de caminar. `AM_Dash` con root motion ignorado. 2-3 copias de estela + onda chica. No dash durante el Paso Inverso ni el remate.
- [ ] Test `Entropia.Dash.DirectionFallsBackToFacing`: input (0,0,0), facing (0,1,0) → (0,1,0); input (3,0,1) → (1,0,0).
- [ ] Test `Entropia.Dash.CooldownAndAirDash`: CanDash(0.3,0.6,false,false)=false; (0.7,0.6,false,false)=true; (0.7,0.6,true,true)=false; (0.7,0.6,true,false)=true.
- [ ] Sandbox Shift. Paso tour `dash`. Caos `spam_dash_x10`.
- [ ] build-test + tours. Commit `feat: dash con Shift`.

### Task 5: Vida y balas que te pegan
**Files:** Create `TemporalHealthComponent.h/.cpp`, `TemporalCombat.h/.cpp` · Modify `TemporalProjectile.h/.cpp`, `TemporalHUDWidget.cpp`, `TemporalDashComponent.cpp`, `EntropiaTests.cpp`
**Produces:**
```cpp
UCLASS() class UTemporalHealthComponent { float MaxHealth=100, RegenDelay=4, RegenPerSecond=25; float Health=100;
  bool ApplyDamage(float Amount);  // false si invulnerable o muerto; OnDeath una sola vez
  void SetInvulnerable(float Seconds); bool IsInvulnerable() const; bool IsDead() const;
  void ResetHealth(); void Advance(float RealDelta); float GetFraction() const; float GetHurtFlash() const;
  FOnTemporalDeath OnDeath; };
namespace TemporalCombat {
  ENTROPY_API bool SegmentHitsCapsule(const FVector& A, const FVector& B, const FVector& Center, float HalfHeight, float Radius, float& OutFraction);
  ENTROPY_API float TimeToHitCapsule(const FVector& Pos, const FVector& Vel, const FVector& Center, float HalfHeight, float Radius, float MaxSeconds); // -1 si no pega
}
// ATemporalProjectile: bool bHurtsPlayer=false; float Damage=20; float PlayerPush=350; FVector GetRealVelocity() const;
```
La bala con `bHurtsPlayer` revisa la cápsula del jugador en su tramo antes que las paredes: si pega y hay daño, empuja, estalla en rojo y desaparece; si eres invulnerable te atraviesa. El dash da `SetInvulnerable(0.25)`.
- [ ] Tests `Entropia.Health.DamageAndDeathOnce` (5 balas de 20 → muerto y OnDeath 1 vez; otra bala → false), `Entropia.Health.RegenAfterDelay` (daño 40, Advance 3.9 → 60; Advance 1 → 85; nunca > 100), `Entropia.Health.InvulnerableBlocksDamage`.
- [ ] Tests `Entropia.Combat.SegmentHitsCapsule` (de frente pega con fracción ≈ (dist−radio)/largo; a 1 m de lado no pega; por encima de la cabeza no pega), `Entropia.Combat.TimeToHitCapsule` (a 500 cm a 2500 cm/s → ≈ 0.183 s; alejándose → −1; más allá de MaxSeconds → −1).
- [ ] HUD: barra roja de vida encima de la energía.
- [ ] build-test. Commit `feat: vida y balas que te pegan`.

### Task 6: Torretas que te siguen y sala 5
**Files:** Create `TemporalReflexRoom.h/.cpp` · Modify `TemporalTurret.h/.cpp`, `TemporalSandbox.h/.cpp`
**Produces:** `ATemporalTurret { bool bTrackPlayer=false; float TurnRate=120, FireInterval=1.2f; void SetTrackingEnabled(bool); void FireTowards(const FVector& Dir); }` (tick con su tiempo propio: congelada no gira ni dispara; se registra como objetivo de tiempo) · `ATemporalReflexRoom { void TeleportPlayerHere(); void ResetRoom(); void RespawnPlayer(); const TArray<…>& GetTurrets()/GetMannequins(); }` en `(0,-3500,3000)`: piso 2400×1600, 3 torretas al fondo mirando a la entrada, 4 columnas, maniquíes (Task 9). Las torretas solo siguen si estás dentro y te ven.
- [ ] Sandbox: tecla 5; `OnDeath` → al siguiente frame `RespawnPlayer` (suelta poderes, vida llena, 1.5 s de protección). Pasos tour `sala5`, `muerte_reaparece`. Caos `morir_en_todo`.
- [ ] build-test + tours. Commit `feat: sala 5 con torretas que te siguen`.

### Task 7: Esquiva perfecta y tiempo de reflejo
**Files:** Modify `TemporalDashComponent.h/.cpp`, `TemporalHUDWidget.cpp`
**Produces:** `float DodgeWindow=0.35f, ReflexDilation=0.3f, ReflexSeconds=1.5f; bool IsReflexActive() const; void StartReflex();` Al empezar el dash: si alguna bala con `bHurtsPlayer` tiene `TimeToHitCapsule(..., DodgeWindow) >= 0` → reflejo (`RequestGlobal`), onda fuerte, texto "TIEMPO DE REFLEJO".
- [ ] Paso tour `esquiva_perfecta` (bala hacia ti a 4 m + dash → log `ENTROPIA_TOUR reflex=1`). Caos `todo_lento_junto` (V + reflejo + remate + burbuja + soltar todo → global 1).
- [ ] build-test + tours. Commit `feat: esquiva perfecta y tiempo de reflejo`.

### Task 8: Ojo lento
**Files:** Create `SlowEyeComponent.h/.cpp` · Modify `TemporalSandbox.cpp`, `TemporalHUDWidget.cpp`
**Produces:** `USlowEyeComponent { float Radius=350, BulletDilation=0.25f, CostPerSecond=6; int32 NumSlowed() const; }` Mientras apuntas: pide 0.25 para cada bala a < 3.5 m y la suelta al salir; gasta solo si frena alguna; sin energía o al dejar de apuntar suelta todo.
- [ ] Paso tour `ojo_lento` (apuntar con balas llegando → log `slowed=N`).
- [ ] build-test + tours. Commit `feat: Ojo lento al apuntar`.

### Task 9: Maniquí temporal y remates
**Files:** Create `TemporalMannequin.h/.cpp`, `TakedownComponent.h/.cpp` · Modify `TemporalReflexRoom.cpp`, `TemporalSandbox.cpp`, `TemporalHUDWidget.cpp`, `EntropiaTests.cpp`
**Produces:**
```cpp
UCLASS() class ATemporalMannequin : public ACharacter { float RecoverSeconds=4; bool bLookAround=false;
  void SetupFromPlayer(const USkeletalMeshComponent* PlayerMesh); void ReceiveTakedown(const FVector& Dir, float Impulse);
  bool IsKnockedDown() const; bool HasPendingHit() const; void ResetMannequin(); };
UCLASS() class UTakedownComponent { float MaxDistance=180, BehindAngle=120, SlowDilation=0.2f, SlowSeconds=0.4f, Impulse=900;
  ATemporalMannequin* GetAvailableTarget() const; bool TryTakedown(); bool IsBusy() const;
  static bool CanTakedown(const FVector& Attacker, const FVector& Target, const FVector& TargetForward, bool bFrozen, float InMaxDistance, float InBehindAngle); };
```
Maniquí: cuerpo y animación del jugador, material gris, sin controlador (`bRunPhysicsWithNoController`), objetivo de tiempo. Golpe: ragdoll (`Ragdoll`, impulso desde la pelvis), cápsula sin colisión; a los 4 s de su tiempo vuelve de pie a su sitio con onda. Congelado: el golpe queda guardado hasta que su dilatación > 0.05.
- [ ] Test `Entropia.Takedown.BehindOrFrozen`: atacante 150 cm detrás → true; 150 cm enfrente → false; enfrente congelado → true; 250 cm detrás → false; de lado (90°) → false.
- [ ] LMB: si hay remate, remata; si no, Detener. HUD "CLIC: REMATE". Pasos tour `remate_espalda`, `remate_congelado`, `maniqui_se_levanta`.
- [ ] build-test + tours. Commit `feat: maniquí temporal y remates`.

### Task 10: Cierre
- [ ] Texto de ayuda con las teclas nuevas; HANDOFF (§3 clases, §5 estado, §15 fase 3 en revisión, §19 notas de la fase 3, teclas).
- [ ] Tour normal + caos completos; revisar capturas. Commit `docs: fase 3 en el handoff`.
