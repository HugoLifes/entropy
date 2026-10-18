# Fase 4 · Disparos con el tiempo — Plan de implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:**
- Revólver y fusil con balas físicas que dañan a los maniquíes.
- Retroceso, dispersión, munición y recarga.
- Cuatro jugadas con el tiempo: Lluvia detenida, Devuélvesela, Recarga inversa y Último en caer.
- Sala 8 · Tiroteo.

**Architecture:**
- Carpeta nueva `Armas/` con:
  - el set de armas (`UEntropiaWeaponSet`, Data Asset `DA_Armas`);
  - la lógica pura (`EntropiaGuns`, `FEntropiaWeaponState`);
  - el componente del jugador `UEntropiaWeaponComponent`.
- Las balas siguen siendo `ATemporalProjectile` (con Registro), ahora en modo "del jugador". Los maniquíes ganan aguante y guardan los tiros si están congelados.
- Los modelos de Fab viven en `Content/Entropia/ArmasFab/` (ignorado por Git) y los importa `Tools/import-armas.ps1`. Si faltan, se usan formas simples.

**Tech Stack:** UE 5.8 C++, Interchange (commandlet `ImportAssets`), montajes dinámicos, Custom Stencil, Automation Tests, tours.

**Spec:** `docs/superpowers/specs/2026-10-17-fase4-disparos-design.md`

## Global Constraints
- **Código:**
  - No tocar `Variant_*` ni `UE_5.8`. Comentarios en español.
  - Unity build: sin `using namespace` y sin nombres genéricos en namespaces anónimos.
  - Carpeta `Armas/` agregada a `PublicIncludePaths`.
- **Armas (valores por defecto en `UEntropiaWeaponSet`):**
  - **Revólver:**
    - Disparo: semi, 6 tiros, cadencia 0.45 s, recarga 1.6 s.
    - Daño: torso 55, cabeza 200.
    - Bala: 8000 cm/s, empujón 900.
    - Dispersión: 0.3° → 2.5° (+1.2 por tiro, −4°/s).
    - Retroceso: 3.2° arriba, ±0.6° de lado.
  - **Fusil:**
    - Disparo: auto, 30 tiros, cadencia 0.1 s, recarga 1.9 s.
    - Daño: torso 20, cabeza 60.
    - Bala: 9000 cm/s, empujón 450.
    - Dispersión: 0.8° → 3° (+0.25 por tiro, −5°/s).
    - Retroceso: 0.7° arriba, ±0.35° de lado.
  - **Al moverte:** dispersión +1°.
  - **El retroceso regresa** a 8°/s.
- **Balas:**
  - Hay máximo 40 balas del jugador vivas.
  - Los primeros 150 cm van en tiempo real.
  - La cabeza son los 30 cm de arriba de la cápsula.
  - Los maniquíes tienen aguante 100.
  - Último en caer: mundo a 0.15 durante 0.7 s reales, más `StartFocus(0.9)`.
- **Teclas:**
  - Clic izquierdo apuntando = disparar (mantener = automático).
  - Q apuntando a algo = Detener.
  - R sin nada en la mira = recargar.
  - Rueda del ratón = cambiar de arma.
  - 8 = sala 8.
- **Verificación de cada tarea:**
  - `Tools\build-test.ps1` → `Result: Succeeded`, sin `warning C`, `TESTS: N ok, 0 fail`.
  - Tours normal y caos (`tours.sh ambos`) terminan en `done` y sin errores nuevos.
  - Las líneas de verificación de los tours anteriores siguen iguales, salvo las que esta fase cambia a propósito: Detener se mudó a Q, así que las que usaban `PrimaryAction` apuntando pasan a `TimeStop->StopAimed()`.

## Review Focus
1. **Morir, teletransportarse o reiniciar la sala con tiros guardados en un congelado:** no se pierden ni se duplican. Si la sala se reinicia, se descartan. Caos `tiros_guardados_reinicio`.
2. **Cambiar de arma, morir o teletransportarse a media recarga:** la recarga se cancela sin dar balas gratis ni dejar el arma bloqueada. Caos `recarga_interrumpida`.
3. **Lluvia detenida y luego morir o teletransportarse antes de que vuelva el tiempo:** las balas flotantes siguen vivas y caen bien. No te pegan a ti ni dejan el mundo detenido. Caos `lluvia_y_muerte`.
4. **Recarga inversa con Ctrl+R sobre 30+ balas:** el cargador nunca pasa del máximo, y una bala vuelve a SU arma. Caos `recarga_inversa_todo`.
5. **El fusil sin parar 10 s:** máximo 40 balas vivas y los FPS no caen más de 15 % respecto al paso anterior. Normal `fusil_rafaga`.

---

### Task 1: Set de armas, lógica pura e importación de modelos
**Files:**
- Create: `Armas/EntropiaWeaponSet.h/.cpp`, `Armas/EntropiaGuns.h/.cpp`, `Tools/import-armas.ps1`
- Modify: `entropy.Build.cs`, `.gitignore` (`Content/Entropia/ArmasFab/`), `Core/EntropiaAnimationSet.h/.cpp` (6 huecos), `Dev/EntropiaTests.cpp`

**Interfaces · Produces:**
- `FEntropiaWeaponPart { TSoftObjectPtr<UStaticMesh> Mesh; FTransform Offset; bool bMagazine; }`
- `FEntropiaWeaponStats`:
  - `FName Name; FText DisplayName; TArray<FEntropiaWeaponPart> Parts;`
  - `FVector MuzzleOffset; FLinearColor Tint` (alfa 0 = conservar el material del modelo);
  - `float FireInterval; bool bAutomatic; int32 MagazineSize; float ReloadSeconds;`
  - `float TorsoDamage, HeadDamage, BulletSpeed, Impulse;`
  - `float SpreadMin, SpreadMax, SpreadPerShot, SpreadRecovery, RecoilPitch, RecoilYaw;`
  - `FEntropiaAnimSlot` no va aquí; las animaciones van en `DA_Animaciones`.
- `UEntropiaWeaponSet : UPrimaryDataAsset` con `Revolver`, `Rifle` y `DummyPistol` (`FEntropiaWeaponStats`) y `static const UEntropiaWeaponSet* Get()`. El editor lo crea en `/Game/Entropia/Datos/DA_Armas` si falta (mismo patrón que `DA_Animaciones`).
- `struct FEntropiaWeaponState`:
  - campos `Ammo`, `Cooldown`, `ReloadLeft`, `Bloom`;
  - `void Init(const FEntropiaWeaponStats&)`;
  - `bool TryFire(const FEntropiaWeaponStats&, bool bTriggerHeld, bool bJustPressed)` (gasta 1; falso si vacío, recargando o en enfriamiento; los semi necesitan `bJustPressed`);
  - `void Tick(const FEntropiaWeaponStats&, float Dt)`;
  - `bool StartReload(const FEntropiaWeaponStats&)` (falso si lleno o ya recargando);
  - `void CancelReload()`;
  - `bool IsReloading() const`;
  - `int32 Refund(const FEntropiaWeaponStats&, int32 Count)` (devuelve las que sí entraron; si estaba recargando, la recarga sigue);
  - `float SpreadDegrees(const FEntropiaWeaponStats&, bool bMoving) const`.
- `namespace EntropiaGuns`:
  - `bool IsHeadshot(float ImpactZ, float CapsuleCenterZ, float HalfHeight)` (≥ arriba − 30);
  - `float StepDistance(float Speed, float Dt, float RealDt, float Traveled, float FreeTravel)`;
  - `int32 OldestToEvict(int32 NumLive, int32 Cap)` (cuántas quitar).
  - `struct FEntropiaStoredShots` (Task 2).
- Huecos en `UEntropiaAnimationSet` (grupo "Armas"): `RevolverAim`, `RevolverFire`, `RevolverReload` (de `Anims/Pistol`: `MF_Pistol_Idle_ADS` en bucle, `MM_Pistol_Fire`, `MM_Pistol_Reload`) y `RifleAim`, `RifleFire`, `RifleReload` (de `Anims/Rifle`).
- `Tools/import-armas.ps1`:
  - importa `D:\unreal projets\armas_assets\{revolver.glb, fusil.fbx, pistola.fbx}` a `/Game/Entropia/ArmasFab/{Revolver,Fusil,Pistola}` con `-run=ImportAssets -replaceexisting -nosourcecontrol`;
  - aborta si el editor está abierto;
  - limita las texturas del revólver a 1024 (`MaxTextureSize`) con una propiedad del set que el editor aplica al crear `DA_Armas`. Si no se puede desde el script, es un `Ruling`.

- [ ] **Step 1: Pruebas que fallan** en `Dev/EntropiaTests.cpp`:
  - `Entropia.Gun.MagazineAndReload`:
    - El revólver: 6 `TryFire` (con `Tick` de 0.45 s entre cada uno) dan true, y el 7.º da false.
    - `StartReload` da true y `IsReloading()` también. Tras `Tick(1.59)` sigue recargando; tras `Tick(0.02)`, `Ammo==6`.
    - `StartReload` con el cargador lleno da false.
    - `Refund(…,3)` con `Ammo==5` devuelve 1 y deja `Ammo==6`.
  - `Entropia.Gun.TriggerAndCadence`:
    - El revólver con `bJustPressed=false` y el gatillo sostenido no dispara un segundo tiro.
    - El fusil sostenido 1.0 s (Tick 0.01 × 100) dispara 10 ± 1 tiros.
  - `Entropia.Gun.SpreadBloom`:
    - Fusil: quieto da `SpreadDegrees == 0.8`.
    - Tras 20 tiros la dispersión es ≤ 3.0 y > 2.9.
    - Tras `Tick(1.0)` sin disparar vuelve a 0.8.
    - Moviéndote suma +1.
  - `Entropia.Gun.HitZoneAndDamage`:
    - `IsHeadshot(171, 100, 90)` da true e `IsHeadshot(150, 100, 90)` da false.
    - Revólver: 1 tiro a la cabeza basta para tumbar (100 aguante) y 2 al torso tumban (55 × 2 ≥ 100).
    - Fusil: con 4 tiros al torso no tumba y con 5 sí.
  - `Entropia.Gun.FreeTravelAndCap`:
    - `StepDistance(8000, 0.000016, 0.016, 0, 150)`: 128 ± 1, es decir 8000 × 0.016 limitado a lo que falta de 150.
    - Con `Traveled=140` da 10 + 8000 × 0.000016 ≈ 10.1.
    - Con `Traveled=200` da 8000 × 0.000016.
    - `OldestToEvict(41, 40) == 1` y `OldestToEvict(40, 40) == 0`.
- [ ] **Step 2:** `Tools\build-test.ps1`. Expected: BUILD FAILED, porque faltan los tipos.
- [ ] **Step 3: Implementar.** Además:
  - agregar la carpeta y las dependencias;
  - el `.gitignore`;
  - el script;
  - la creación de `DA_Armas` en el editor, con la referencia a las mallas importadas:
    - Revólver: `/Game/Entropia/ArmasFab/Revolver/revolver/StaticMeshes/revolver`.
    - Fusil: `/Game/Entropia/ArmasFab/Fusil/fusil_6`, `fusil_scope_6`, `fusil_charguer_6` (este con `bMagazine`).
    - Pistola: `/Game/Entropia/ArmasFab/Pistola/pistol_2` y `pistol_charger_2`.
    - El fusil y la pistola llevan `Tint` oscuro.
- [ ] **Step 4:** `Tools\build-test.ps1`. Expected: `TESTS: 52 ok, 0 fail`.
- [ ] **Step 5:** Correr `Tools\import-armas.ps1`. Expected: 3 veces `Success - 0 error(s)`, y `git status` no muestra `ArmasFab`.
- [ ] **Step 6:** Commit `feat(armas): set de armas, lógica de cargador/dispersión/daño e importación de modelos`.

### Task 2: Balas que dañan, aguante y tiros guardados
**Files:**
- Modify: `Mundo/TemporalProjectile.h/.cpp`, `Mundo/TemporalMannequin.h/.cpp`, `Mundo/TemporalArmedDummy.h/.cpp`, `Dev/EntropiaTourNormal.cpp`, `Dev/EntropiaTourCaos.cpp`, `Dev/EntropiaTests.cpp`

**Interfaces:**
- **Consumes:** `EntropiaGuns::IsHeadshot` y `StepDistance`.
- **Produces:**
  - `ATemporalProjectile`:
    - `bool bFromPlayer`;
    - `int32 WeaponIndex`;
    - `float TorsoDamage, HeadDamage, Impulse`;
    - `float FreeTravel = 150` (solo del jugador);
    - `FOnBulletEvent` nativo estático o un callback hacia el dueño: `TFunction<void(ATemporalProjectile*, EEntropiaBulletEvent, AActor* Victim)> OnEvent`, con `EEntropiaBulletEvent { Hit, KnockedDown, Stored, Returned }`.
  - `ATemporalMannequin`:
    - `float Toughness = 100` y `float GetToughness() const`;
    - `EEntropiaShotResult ReceiveBulletHit(const FVector& Dir, float Damage, float Impulse, ATemporalProjectile* Bullet)`, con `EEntropiaShotResult { Ignored, Hurt, KnockedDown, Stored }`;
    - `int32 NumStoredShots() const`.
  - `ATemporalArmedDummy`:
    - `void ReceiveReturnedBullet(const FVector& Dir)`: Devuélvesela; lo tumba si está de pie y armado.
- **Comportamiento:**
  - Las balas del jugador ignoran al jugador y sus ecos.
  - Al pegarle a la cápsula de un `ATemporalMannequin` de pie llaman `ReceiveBulletHit`:
    - Si devuelve `Stored`, la bala se incrusta **pegada** a él (`AttachToActor`, estado `Embedded`).
    - Si no, desaparece con chispas.
  - Las balas enemigas siguen atravesando maniquíes, como hoy.
  - **Congelado:** suma daño e impulso y guarda las balas. Al volver su tiempo (en `Tick`, cuando ya no `IsFrozen`):
    - aplica el total una sola vez;
    - si el aguante llega a ≤ 0, `KnockDown(dir promedio, impulso total)`; si no, `ReceiveHit(dir, false)`;
    - destruye las balas pegadas con chispas.
  - `ResetMannequin` y `Recover` descartan los tiros guardados, destruyen las balas pegadas y dejan el aguante en 100.
  - `ReturnToGun`:
    - si `bFromPlayer`, llama `OnEvent(Returned)`;
    - si `Gun` es `ATemporalArmedDummy`, llama `ReceiveReturnedBullet(-Dir)`.
  - `StepForward` usa `StepDistance(Speed, Dt, TemporalFX::RealDelta(), Traveled, bFromPlayer ? FreeTravel : 0)`.
  - Las balas del jugador tienen color dorado y `RenderCustomDepth` con stencil 1, para verse a color en el mundo detenido.

- [ ] **Step 1: Prueba que falla:**
  - `Entropia.Gun.StoredShotsSum` sobre la lógica pura `FEntropiaStoredShots` (en `EntropiaGuns.h`):
    - `Add(Dir, Damage, Impulse)` tres veces: `TotalDamage()` suma y `AverageDirection()` está normalizado.
    - `Clear()` deja `Num()==0`.
  - Tours que fallan:
    - Normal: `tiro_cabeza` (dummy de la sala 7 con una bala del jugador de 200 a la cabeza: `tumbado=1`); `tiro_torso` (55: `tumbado=0 aguante=45`); `tiro_congelado` (Detener + 3 tiros del fusil al torso: `guardados=3 tumbado=0`; tras soltar, `aguante=40`); `devuelvesela` (una bala del dummy invertida con `Inversion->BeginInversionOn` hasta el cañón: `dummy_tumbado=1`).
    - Caos: `tiros_guardados_reinicio` (guardados=2, después `ResetRoom` → `guardados=0`, `balas_pegadas=0` y `aguante=100`).
- [ ] **Step 2:** Compilar y correr los tours. Expected: las pruebas o las líneas fallan (faltan los campos o salen valores 0).
- [ ] **Step 3: Implementar** lo descrito en Interfaces.
- [ ] **Step 4:** `build-test` (53 ok) más los tours. Expected:
  - `tiro_cabeza tumbado=1`;
  - `tiro_torso aguante=45`;
  - `tiro_congelado guardados=3` y luego `aguante=40`;
  - `devuelvesela dummy_tumbado=1`;
  - caos `guardados=0 balas_pegadas=0 aguante=100`.
- [ ] **Step 5:** Commit `feat(armas): las balas dañan, aguante de maniquíes, tiros guardados y Devuélvesela`.

### Task 3: El arma del jugador (disparar, recargar, cambiar, sentir)
**Files:**
- Create: `Armas/EntropiaWeaponComponent.h/.cpp`
- Modify: `Dev/EntropiaPlayerPowers.h/.cpp`, `Dev/EntropiaControlsComponent.h/.cpp`, `FX/TemporalHUDWidget.h/.cpp`, los tours

**Interfaces:**
- **Consumes:** Task 1 (`UEntropiaWeaponSet`, `FEntropiaWeaponState`) y Task 2 (`ATemporalProjectile::OnEvent` y sus campos).
- **Produces:** `UEntropiaWeaponComponent` con:
  - `void TriggerPressed(); void TriggerReleased(); bool Reload(); void CycleWeapon(int32 Dir);`
  - `int32 GetCurrentIndex() const; const FEntropiaWeaponStats& CurrentStats() const;`
  - `const FEntropiaWeaponState& CurrentState() const; const FEntropiaWeaponState& StateOf(int32 Index) const;`
  - `float GetSpreadDegrees() const; float GetHitMarkerAlpha() const; bool LastMarkerWasKill() const;`
  - `int32 NumLiveBullets() const; const TArray<TWeakObjectPtr<ATemporalProjectile>>& GetLiveBullets() const;`
  - `void CancelAll()` (suelta el gatillo, cancela la recarga y quita la cámara lenta del remate).
- **Comportamiento:**
  - **Disparar:**
    - Solo apuntando. El punto de mira se calcula con un trazo desde la cámara hasta 10 000 cm, y la dirección es (punto − cañón) más la dispersión, en un cono aleatorio.
    - El cañón es el socket `weapon_r` más `MuzzleOffset`.
    - Cada disparo:
      - aplica el retroceso (`AddPitchInput` negativo y el yaw aleatorio) y lo regresa poco a poco;
      - lanza un `TemporalFX::Burst` dorado en el cañón;
      - quita la bala más vieja si hay más de 40.
  - **Arma sin balas:** al hacer clic se recarga sola.
  - **Partes del arma:** son `UStaticMeshComponent` pegados a `weapon_r` con su `Offset`. Si la malla no carga, usa un cubo escalado.
    - El `Tint` se aplica con `TemporalFX::MakeSolid` si el alfa es mayor que 0.
    - Todas las partes van con stencil 1.
  - **Recarga inversa:** `OnEvent(Returned)` llama `StateOf(Bullet->WeaponIndex).Refund(1)` y muestra "+1" en el HUD durante 0.6 s.
  - **Último en caer:** cuando una bala tumba a un `ATemporalArmedDummy` y no queda ningún otro del mismo `Owner` de pie (no tumbado, sin tiro pendiente, sin desplomarse) y armado:
    - pide `RequestGlobal(this, 0.15)` durante 0.7 s reales y llama `Aim->StartFocus(0.9)`;
    - no hace nada si `WorldStop->IsActive()`.
  - **Animaciones:**
    - Apuntando, quieto (velocidad < 10) y sin cobertura, agachado, parkour, agarre, remate ni golpe: pose `*Aim`.
    - Disparar y recargar reproducen `*Fire` y `*Reload` solo en ese mismo estado.
    - Solo quita su propia pose (patrón `IsPlaying` del dummy).
  - **Cambiar de arma** cancela la recarga.
- **Controles:**
  - `LeftMouseButton` al soltar → `TriggerReleased`.
  - `PrimaryAction`: primero el remate; si estás apuntando, `TriggerPressed`; si no, el golpe.
  - **Q:** si estás apuntando y `Aim->FindTarget()` encuentra algo, `TimeStop->StopAimed()`; si no, lo de siempre.
  - **R sin Ctrl:** si `Aim->FindTarget()==nullptr`, `Weapons->Reload()`; si no, invierte.
  - **Rueda:** `MouseScrollUp`/`MouseScrollDown` → `CycleWeapon(±1)`.
  - **Texto de ayuda (`ShowHelp`):**
    - Línea 1001: `PELEA Y PODERES · Clic izq.: golpes (apuntando: disparar) · Rueda: cambiar arma · R: invertir (sin nada en la mira: recargar) · Q: burbuja (apuntando a algo: detener) · E: agarrar · F: Paso Inverso · Ctrl+R: todo · V: Sobremarcha · N: nivel 2 = detener todo · H: eco · G: borrar ecos · T: cajas`.
    - Línea 1002: agregar `· 8: Tiroteo`.
- **HUD:**
  - Abajo a la derecha, "REVÓLVER 6/6" o "RECARGANDO…" y "+1" en dorado.
  - Las rayitas de la mira se separan con la dispersión: `TickGap += spread × 6 px`.
  - Marca de impacto: una X de 4 rayitas, blanca o roja, que dura 0.25 s.
- **`PlayerPowers`:**
  - crea `Weapons` después de `Aim`;
  - `CancelAll`/`EndForTeleport` llaman `Weapons->CancelAll()`.
- **Tours:** los pasos viejos que usaban el clic apuntando para Detener ahora llaman `TimeStop->StopAimed()`.

- [ ] **Step 1: Tours que fallan:**
  - Normal:
    - `revolver_disparo`: apuntar a un dummy y clic → `balas=5 vivas>=1` y una captura.
    - `recargar`: R sin objetivo → `recargando=1`, y 1.7 s después `balas=6`.
    - `cambiar_arma`: rueda → `arma=Rifle balas=30`.
    - `fusil_rafaga`: mantener 3 s → `vivas<=40`, más los FPS del paso.
    - `recarga_inversa`: 3 tiros a la pared, `BeginInversionOn` de uno → `balas` sube en 1.
    - `ultimo_en_caer`: en la sala 7, tumbar 2 dummies con `ReceiveTakedown` y disparar a la cabeza del 3.º → `camara_lenta=1`.
    - `q_detener`: apuntando a un dummy, `OnBubble` → `detenido=1`.
  - Caos:
    - `recarga_interrumpida`: R, cambiar de arma a los 0.5 s y volver → no recarga sola y `balas=` las de antes. Y R, morir y reaparecer → `recargando=0` y el arma dispara después.
    - `recarga_inversa_todo`: el fusil vacía 30 en la pared, Ctrl+R 3 s → `balas<=30`.
- [ ] **Step 2:** Compilar y correr los tours. Expected: falla (no existe `Weapons`).
- [ ] **Step 3:** Implementar.
- [ ] **Step 4:** `build-test` (53 ok) más los tours. Expected:
  - Las líneas de arriba.
  - `fusil_rafaga vivas<=40` y `fps_min` ≥ 85 % del `fps_min` del paso anterior.
  - Las líneas viejas, iguales.
- [ ] **Step 5:** Commit `feat(armas): revólver y fusil del jugador, recarga, retroceso, HUD, Recarga inversa y Último en caer`.

### Task 4: Lluvia detenida y dummies con pistola de verdad
**Files:**
- Modify: `Mundo/TemporalArmedDummy.cpp` (pistola desde `DA_Armas.DummyPistol`, con un cubo si falta), `Mundo/TemporalProjectile.cpp` (si hace falta para que flote y brille), los tours

**Interfaces · Consumes:** `UEntropiaWeaponSet::Get()->DummyPistol` y `StepDistance`/`FreeTravel` de la Task 2.

- [ ] **Step 1: Tours que fallan:**
  - Normal:
    - `lluvia_detenida`: N, Q (el mundo se detiene), 6 tiros del revólver hacia un dummy:
      - En ese momento: `flotando=6`, con la distancia de cada bala al cañón entre 140 y 260.
      - Después del tiempo (paso `lluvia_suelta`): `tumbado=1` y una captura.
    - `dummy_pistola`: `malla_pistola=1` (la pistola del dummy usa la malla del set y no el cubo, si `ArmasFab` existe).
  - Caos:
    - `lluvia_y_muerte`: lluvia detenida y luego morir.
      - Esperado: `global=1.00 stencil=0`, sin errores y `vida_llena=1`.
      - Las balas flotantes siguen su camino y no te pegan.
- [ ] **Step 2:** Compilar y correr los tours. Expected: falla.
- [ ] **Step 3:** Implementar:
  - La pistola del dummy sale del set: varias partes pegadas al `Gun`, que pasa a ser un `USceneComponent` raíz con hijos. El desarme les da física a todas las partes juntas, o solo a la principal y se ocultan las demás; se elige la más simple y va como `Ruling`.
  - Ajustes de la lluvia que salgan de los tours.
- [ ] **Step 4:** `build-test` más los tours. Expected: las líneas de arriba y las viejas iguales.
- [ ] **Step 5:** Commit `feat(armas): lluvia detenida y pistola de verdad para los dummies`.

### Task 5: Sala 8 · Tiroteo
**Files:**
- Create: `Mundo/TemporalShootingRoom.h/.cpp`
- Modify: `Dev/EntropiaRoomsComponent.h/.cpp` (`ShootingRoomOffset (0,-10500,3000)`, `GoToRoom(8)`), `Dev/EntropiaControlsComponent.*` (tecla 8), los tours

**Interfaces · Produces:** `ATemporalShootingRoom`, el mismo patrón que `ATemporalCombatRoom`:
- `TeleportPlayerHere`, `ResetRoom`, `GetDummies`, `RoomToWorld`, `PlacePlayer`;
- `const TArray<TObjectPtr<ATemporalBreakable>>& GetBreakables()`.

**La sala:**
- Mide 3600 × 1800 y tiene paredes de 600.
- Tus coberturas están a x=500.
- 4 dummies detrás de muros a 1200, 1800, 2500 y 3000, alternando a los lados.
- Un `ATemporalBreakable` de 1×3×2 m frente al dummy de 1800.
- 3 cajas con física.
- **Pista (mensaje 1009):** `SALA 8 · TIROTEO: apunta + clic = disparar · rueda = cambiar arma · N y Q = mundo detenido: dispara y verás la LLUVIA DETENIDA · R a una bala enemiga = DEVUÉLVESELA · R a tus balas = RECARGA INVERSA · tumba al último y mira`.

- [ ] **Step 1: Tours que fallan:**
  - Normal: `sala8` (`GoToRoom(8)`: `dummies=4 rompibles=1`, más una captura).
  - Caos: `sala8_teleport_lluvia` (lluvia detenida y luego tecla 7: el tiempo vuelve, sin errores).
- [ ] **Step 2:** Correr los tours. Expected: falla.
- [ ] **Step 3:** Implementar.
- [ ] **Step 4:** `build-test` más los tours. Expected: las líneas.
- [ ] **Step 5:** Commit `feat(armas): sala 8 · Tiroteo`.

### Task 6: Documentación
**Files:**
- `README.md`: capturas en `docs/capturas/` (revolver, lluvia-detenida, devuelvesela, sala8), la tabla de teclas, la sección "Disparos (fase 4)" y la hoja de ruta.
- Fuera del repo:
  - `HANDOFF.md`: §3 clases nuevas, §5, §15 "DÓNDE NOS QUEDAMOS" y §21 de la fase. Incluye el "Para Hugo": correr `Tools\import-armas.ps1` en otra compu y cómo ajustar `DA_Armas`.
  - La memoria.
- [ ] Commit `docs: fase 4 disparos` y push de la rama.
