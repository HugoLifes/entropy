# Fase 3b · Parkour, movimiento, pelea y Detener nivel 2 — Plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Correr, agacharse, caídas, parkour (dash vault, gato, subir, colgarse, pared), cobertura, dummy armado, combo de golpes, agarrar/desarmar/someter, romper el cuello y Detener nivel 2 (The World).

**Architecture:** Carpetas nuevas `Movimiento/` (locomoción, caídas, parkour, cobertura) y `Pelea/` (golpes, agarre). El C++ mueve al personaje por trayectorias calculadas (la animación solo es el aspecto). Todas las animaciones salen de un `UEntropiaAnimationSet` (Data Asset `DA_Animaciones` que el editor genera si falta; si no, valores por defecto con las animaciones de la plantilla). La lógica de decisión es pura y se prueba; lo visual, con el tour.

**Tech Stack:** UE 5.8 C++, CharacterMovement (modo `MOVE_None` mientras el parkour mueve al personaje), montajes dinámicos (`PlaySlotAnimationAsDynamicMontage`), post-proceso con Custom Stencil, Automation Tests, tours.

**Spec:** `docs/superpowers/specs/2026-10-17-fase3b-parkour-design.md`

## Global Constraints
- No tocar `Variant_*` ni `UE_5.8`. Comentarios en español. Nada de nombres genéricos en namespaces anónimos; nada de `using namespace` (unity build).
- **Velocidades:** 450 normal · 700 corriendo · 220 agachado · 320 apuntando. Giro 360°/s.
- **Parkour:**
  - dash vault: 50-130 cm de alto, ≤ 70 cm de fondo;
  - gato: corriendo, 50-130 cm de alto, 70-220 cm de fondo;
  - subir: ≤ 130 cm;
  - trepar: 130-230 cm;
  - pared: 230-340 cm corriendo;
  - colgarse: borde a 120-230 cm sobre los pies.
- **Caídas:** < 400 cm normal · 400-900 cm rueda si |v horizontal| > 200, si no aterrizaje duro 0.5 s · > 900 cm aterrizaje duro + 10 de daño por metro extra.
- **Cobertura:** bajo ≥ 70 cm, alto ≥ 160 cm, distancia ≤ 120 cm.
- **Dummy armado:** cada 2.5 s se asoma (0.3 s), dispara 2 balas (1800 cm/s, separadas 0.35 s) y vuelve.
- **Golpes:** combo de 3, ventana de 0.5 s, alcance 150 cm, cono de 70°, aturde 0.6 s, el tercero tumba.
- **Agarre:** ≤ 150 cm, dummy de pie; sometido 4 s.
- **Detener nivel 2:** 60 de energía, 4 s reales, mundo a 0.001, onda 0.8 s de ida y 0.6 s de regreso.
- **Teclas:**
  - Shift: tocar = dash, mantener = correr.
  - C: agacharse / cubrirse.
  - Espacio: saltar / parkour.
  - E: agarrar.
  - H: eco.
  - Clic izquierdo: sin apuntar = golpe; apuntando = Detener; con aviso = remate.
  - N: nivel de Detener.
  - 6 y 7: salas nuevas.
- **Verificación de cada tarea:** `Tools\build-test.ps1` → `Result: Succeeded`, sin `warning C`, `TESTS: N ok, 0 fail`. Más los dos tours terminando en `done` sin errores, y que las líneas de verificación de los tours anteriores sigan iguales.

## Review Focus
1. **Parkour + poderes a la vez** (F, dash, Sobremarcha, morir, teletransporte a medio salto o colgado) → nunca queda en `MOVE_None` ni atorado. Caos `parkour_con_todo`.
2. **Espacio sin nada enfrente** → salto normal (el parkour no se roba el salto). Tour `salto_normal`.
3. **Cobertura con el muro que desaparece / se rompe** → sales de cobertura sin quedar pegado. Caos `cobertura_rompible`.
4. **Agarre mientras el dummy está congelado o es golpeado** → no hay dos animaciones peleando; se suelta al morir/teleport. Caos `agarre_raro`.
5. **Detener nivel 2 interrumpido** (morir, teletransporte, nivel cambiado en medio) → color y tiempo vuelven a la normalidad. Caos `mundo_detenido_raro`.

---

### Task 1: Set de animaciones y locomoción (correr, agacharse, peso)
**Files:** Create `Core/EntropiaAnimationSet.h/.cpp`, `Movimiento/EntropiaLocomotionComponent.h/.cpp` · Modify `entropy.Build.cs` (carpetas `Movimiento`, `Pelea`), `Poderes/TemporalAimComponent.cpp` (ya no escribe la velocidad), `Dev/EntropiaControlsComponent.*` (Shift soltar, C), `Dev/EntropiaPlayerPowers.*`, `FX/TemporalFX.cpp` (genera `DA_Animaciones`), `Dev/EntropiaTests.cpp`
**Produces:** `FEntropiaAnimSlot { TSoftObjectPtr<UAnimSequenceBase> Anim; float PlayRate=1, StartTime=0, BlendIn=0.15f, BlendOut=0.2f; bool bLoop=false; }` · `UEntropiaAnimationSet::Get()` (DA o valores por defecto) · `EntropiaAnim::Play(ACharacter*, const FEntropiaAnimSlot&) -> float duración` / `Stop` · `UEntropiaLocomotionComponent { SetSprint(bool), ToggleCrouch(), IsSprinting(), IsCrouched(), static float ComputeWalkSpeed(bool bSprint, bool bCrouch, float AimAlpha) }`
- [ ] Test `Entropia.Locomotion.WalkSpeed`: (false,false,0)=450; (true,false,0)=700; (false,true,0)=220; (true,false,1)=320 (apuntar gana a correr); (false,true,1)=220.
- [ ] RED → implementar → GREEN. Tour: pasos `correr`, `agacharse`. Commit.

### Task 2: Caídas
**Files:** Create `Movimiento/EntropiaFalls.h/.cpp` (lógica pura) · Modify `EntropiaLocomotionComponent` (apex, `LandedDelegate`, rodar procedural, aterrizaje duro, daño)
**Produces:** `enum class EEntropiaLanding { Normal, Roll, Hard }` · `EntropiaFalls::Decide(float FallHeight, float HorizontalSpeed, float& OutDamage) -> EEntropiaLanding`
- [ ] Test `Entropia.Fall.Outcome`: 300 → Normal, 0 daño; 600 moviéndose (300) → Roll, 0; 600 quieto (50) → Hard, 0; 1200 → Hard, 30.
- [ ] Tour `caida_rodando`, `caida_dura` (desde un balcón de la sala 6). Commit.

### Task 3: Parkour (vault, gato, subir, trepar) y sala 6
**Files:** Create `Movimiento/EntropiaParkour.h/.cpp` (pura), `Movimiento/EntropiaParkourComponent.h/.cpp`, `Mundo/TemporalParkourRoom.h/.cpp` · Modify `entropyCharacter.h/.cpp` (`Jump()` pregunta al parkour), `Dev/EntropiaRoomsComponent` (sala 6)
**Produces:** `struct FEntropiaObstacle { bool bHit; float Height; float Depth; bool bLandingClear; bool bTopClear; }` · `enum class EEntropiaTraversal { None, DashVault, Kong, StepUp, Climb, WallRun }` · `EntropiaParkour::Classify(const FEntropiaObstacle&, bool bSprinting)` · `EntropiaParkour::PathPoint(EEntropiaTraversal, Start, Edge, End, Alpha) -> FVector` · `UEntropiaParkourComponent { bool TryStart(); bool IsBusy() const; EEntropiaTraversal GetCurrent() const; }`
- [ ] Tests `Entropia.Parkour.Classify` (cada umbral y "nada" para obstáculo de 30 cm o pared de 400 cm sin correr) y `Entropia.Parkour.PathArcsOverObstacle` (Alpha 0 = inicio, 1 = fin, a la mitad está sobre el borde + margen).
- [ ] Tour sala 6: `salto_normal`, `dash_vault`, `gato`, `subir`, `trepar`. Commit.

### Task 4: Orillas y pared
**Files:** Modify `EntropiaParkourComponent` (colgarse, de lado, subir, soltar, auto-agarre al caer, correr por la pared)
**Produces:** `bool IsHanging() const; void Drop(); void SetTourInput(FVector)`
- [ ] Test `Entropia.Parkour.LedgeReach` (borde 150 sobre pies → sí; 100 → no; 260 → no).
- [ ] Tour `colgarse`, `orilla_de_lado`, `orilla_sube`, `pared_arriba`. Commit.

### Task 5: Cobertura
**Files:** Create `Movimiento/EntropiaCoverComponent.h/.cpp` · Modify controles (C)
**Produces:** `enum class EEntropiaCover { None, Low, High }` · `EntropiaCover::KindForHeight(float)` · `UEntropiaCoverComponent { bool TryEnter(); void Exit(); bool IsInCover() const; }`
- [ ] Test `Entropia.Cover.Kind` (60 → None, 90 → Low, 200 → High).
- [ ] Tour `cubrirse_bajo`, `asomarse`, `cubrirse_alto`. Caos `cobertura_rompible`. Commit.

### Task 6: Dummy armado y sala 7
**Files:** Create `Mundo/TemporalArmedDummy.h/.cpp`, `Mundo/TemporalCombatRoom.h/.cpp` · Modify `TemporalMannequin` (`ReceiveHit`, `Stagger`, virtual hooks)
**Produces:** `struct FArmedDummyRoutine { enum EStep { Covered, Stepping, Firing, Returning }; float Time; int32 ShotsFired; bool Advance(float Dt, int32& OutShots); }` · `ATemporalArmedDummy { bool IsArmed() const; void Disarm(const FVector& Dir); }`
- [ ] Test `Entropia.ArmedDummy.Routine` (2.5 s cubierto → se asoma 0.3 s → 2 disparos separados 0.35 s → regresa; con Dt 0 no avanza).
- [ ] Tour `sala7`, `dummy_dispara`. Commit.

### Task 7: Golpes, romper el cuello y golpe con enfoque
**Files:** Create `Pelea/EntropiaMeleeComponent.h/.cpp` · Modify `Poderes/TakedownComponent` (tipo de remate + cámara), `TemporalMannequin` (`ReceiveNeckBreak`), `EntropiaPlayerPowers::PrimaryAction`
**Produces:** `struct FEntropiaCombo { int32 Next(float Now); }` (0 = no golpea) · `enum class EEntropiaTakedown { NeckBreak, FocusStrike }` · `UTakedownComponent::ChooseKind(bool bBehind, bool bFrozen, bool bSubdued)`
- [ ] Tests `Entropia.Melee.ComboWindow` (clic → 1; a 0.3 s → 2; a 0.6 s → 3; tras el 3 o tras 1.0 s sin clic → 1 otra vez) y `Entropia.Takedown.KindChoice`.
- [ ] Tour `golpes_combo`, `romper_cuello`, `golpe_enfoque`. Commit.

### Task 8: Agarrar, desarmar y someter
**Files:** Create `Pelea/EntropiaGrabComponent.h/.cpp` · Modify controles (E, H para el eco), `TemporalMannequin`/`ArmedDummy` (sometido)
**Produces:** `static bool CanGrab(Attacker, Target, AttackerFwd, Dist, bStanding)` · `UEntropiaGrabComponent { bool TryGrab(); void Release(bool bShove); bool IsHolding() const; ATemporalMannequin* GetHeld() const; }`
- [ ] Test `Entropia.Grab.Condition`.
- [ ] Tour `agarrar_desarmar`, `sometido_cuello`. Caos `agarre_raro`. Commit.

### Task 9: Detener nivel 2 (The World)
**Files:** Create `Poderes/TemporalWorldStopComponent.h/.cpp` · Modify `FX/TemporalFX.cpp` (`M_TemporalWorldStop`), `Config/DefaultEngine.ini` (`r.CustomDepth=3`), `Core/TemporalDilation.cpp` (mínimo global 0.0005), `EntropiaPlayerPowers` (nivel, Q), controles (N), HUD
**Produces:** `EntropiaWorldStop::WaveRadius(float T, float Duration, float MaxRadius)`; `UTemporalWorldStopComponent { bool Start(); void Stop(); bool IsActive() const; }`
- [ ] Test `Entropia.WorldStop.WaveShape` (0 al inicio, máximo a 0.8 s, se mantiene, regresa a 0 al final).
- [ ] Tour `mundo_detenido` (captura en negativo, captura gris con jugador a color). Caos `mundo_detenido_raro`. Commit.

### Task 10: Cierre
- [ ] Ayuda en pantalla, HANDOFF (clases, teclas, sección 20), instrucciones para Hugo (descargar el Game Animation Sample y las animaciones de artes marciales; cómo usar `DA_Animaciones`), tours completos, revisión final.
