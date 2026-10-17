# Fase 1 · Pulir el tiempo — Diseño

Fecha: 16 oct 2026 · Proyecto: ENTROPÍA (`entropy/`, Unreal 5.8, C++) · Usuario: Hugo (no programa C++; prueba en Play)

## 1. Intención (el porqué)
El "wow" de ENTROPÍA, en palabras de Hugo: **cómo se ve alterar el tiempo e interactuar con él**. Antes de pasar a destrucción, movimiento, armas y enemigos, los poderes de tiempo tienen que sentirse bien, verse bien y no bajar los FPS en su laptop (GTX 1650, 4 GB).

## 2. Metas
1. **Paso Inverso (F) más ligero y mejor visto**, manteniendo el estilo actual ("A": tinte azul, mundo deslavado y copias translúcidas). Hugo eligió A en la comparación visual, pero dijo que la estela "no le convence tanto" y que **bajan un poco los FPS**.
2. **Barra de energía sencilla** en pantalla (UMG), **una sola** compartida por todos los poderes.
3. **Invertir objeto (R)**: apuntar con el centro de la pantalla, el objeto se resalta, mantener R y **solo ese** objeto (y lo que depende de él) regresa en el tiempo.
4. **Sobremarcha (V)**: el mundo va lento y el jugador a velocidad normal, con estela estilo A. Es el poder que antes llamábamos "Sandevistan"; se renombró porque el original es de otra serie.
5. **Eco (E)**: animación suave (hoy la pose salta de frame en frame).
6. **Burbuja (Q)**: sin cambios de comportamiento; solo gasta energía de la barra.

## 3. No-metas (fuera de esta fase)
- Distorsión por refracción alrededor del personaje ("time distortion"): **va a la Fase 7 (pulido)**, por decisión de Hugo.
- Sonido, arte propio, Enhanced Input, vida y daño, enemigos.
- Cambiar el estilo visual por otro (B, C o D de la comparación quedaron descartados).

## 4. Anti-metas (sería un fracaso aunque funcione)
- Que los FPS bajen **más** que hoy al usar F o V.
- Que la Sobremarcha se vea como copia directa del Sandevistan (colores arcoíris, mismo nombre).
- Romper lo que Hugo ya probó y le gustó: el cuarto de pruebas (salas 1-3), la sala 4 de física, la burbuja que congela balas y ecos.
- Que R deje de servir para los puzzles de la sala 2 (plataformas) y la sala 4 (bala que vuelve y muro que se rearma).

## 5. Restricciones
- C++ en `entropy/Source/entropy/` (estructura plana). Sin crear assets a mano: solo contenido del motor y lo que el código genere (como `M_TemporalGhost`).
- Teclas con `InputComponent->BindKey` en `ATemporalSandbox` (Enhanced Input queda para después).
- Unity build: no usar nombres genéricos en namespaces anónimos (ver HANDOFF §13).
- Compilar con el editor cerrado; una sola sesión de IA compila a la vez.

## 6. Diseño por pieza

### 6.1 Energía compartida — `UTemporalEnergyComponent` (nuevo)
- Vive en el jugador (lo agrega el Sandbox). La energía sale de `UInverseStepComponent` y pasa aquí.
- API: `bool CanAfford(float)`, `bool Spend(float)` (gasto de golpe), `bool Drain(float PerSecond, float DeltaTime)` (gasto continuo; devuelve false cuando se acaba), `void Deny()` (aviso visual de "no alcanza"), `float GetFraction()`, `FName ActivePower` (para el color de la barra).
- Recarga: **20/s** después de **1 s** sin gastar. Máximo **100**.
- **Costos (decisión del builder, ajustables como UPROPERTY):**

| Poder | Costo | Mínimo para empezar |
|---|---|---|
| F · Paso Inverso | 34/s | 15 |
| R · Invertir objeto | 15/s | 10 |
| V · Sobremarcha | 25/s (tiempo real) | 20 |
| Q · Burbuja | 30 de golpe | 30 |
| E · Eco | 10 de golpe | 10 |

- El tiempo de la energía se mide en **tiempo real** (`FApp::GetDeltaTime()` sin dilatación), para que la Sobremarcha y la burbuja no cambien su ritmo.

### 6.2 Barra de energía — `UTemporalHUDWidget` (nuevo, UMG sin asset)
- `UUserWidget` creado en C++ con `CreateWidget` y dibujado en `NativePaint` (`FSlateDrawElement::MakeBox` / `MakeText`). Sin widget tree ni assets.
- Abajo al centro: barra de ~320 × 14 px, fondo oscuro, relleno del color del poder activo (azul = F, ámbar = R, cian = V, blanco = reposo). Al pedir un poder sin energía, la barra parpadea en rojo ~0.3 s.
- Se quita el texto de debug actual (`bShowDebugHud` pasa a `false`).
- Agregar `SlateCore` a `entropy.Build.cs`.

### 6.3 Estela ligera compartida — `UTemporalTrailComponent` (nuevo)
- Reemplaza el `SpawnActor<ATemporalAfterimage>` por copia. **Pool fijo** de `UPoseableMeshComponent` (por defecto **4**) creados una vez, con un MID propio cada uno, `CastShadow=false`, sin colisión, `bAbsoluteLocation/Rotation` (no siguen al jugador).
- `void Emit(const TArray<FTransform>& Pose, const FTransform& MeshWorldTransform, const FLinearColor& Color)`: toma el slot más viejo, copia la pose, lo coloca y reinicia su fade.
- Fade en el Tick del componente con el DeltaTime **del dueño** (el jugador), así en Sobremarcha se desvanece en tiempo real.
- Valores por defecto: intervalo **0.09 s**, vida **0.30 s**, color F = `(0.25, 0.45, 0.95, 0.22)`; es más tenue que hoy (`0.35` de alfa con 6 copias). Alfa con caída suave (cuadrática).
- Lo usan el Paso Inverso y la Sobremarcha. `ATemporalAfterimage` queda en el código pero sin usarse.

### 6.4 Paso Inverso (F) — cambios en `UInverseStepComponent`
- Energía → `UTemporalEnergyComponent`; estela → `UTemporalTrailComponent`.
- **Pose suave al revés:** el Recorder expone el tramo actual del rebobinado (`GetRewindSegment(From, To, Alpha)`) y la marioneta mezcla hueso por hueso (`FTransform::BlendWith`). Si el número de huesos no coincide, usa la pose sin mezclar (como hoy).
- **Post-process propio:** un `UPostProcessComponent` (`bUnbound`) del poder, con su propio `BlendWeight`, en lugar de escribir en `Camera->PostProcessSettings`. Así F y V no se pisan. Mismo look A (desaturar, tinte azul, fringe, viñeta).
- FOV: se queda el "kick" de F; la Sobremarcha **no** toca el FOV.

### 6.5 Invertir objeto (R) — `UObjectInversionComponent` (nuevo)
- **Lista de objetos con Registro:** nuevo `UTemporalSubsystem` (`UWorldSubsystem`) donde cada `UTemporalRecorderComponent` se registra en `BeginPlay` y sale en `EndPlay`. El Sandbox también lo usa para "rebobinar todo".
- **Apuntar (cada frame, si R no está activo):** desde la vista de la cámara, elegir el objeto registrado (sin contar al jugador) más cercano al centro de la pantalla: ángulo < **4°**, distancia < **35 m** y con línea de vista (trace de Visibility que ignore al objetivo; las balas no bloquean Visibility, así que funciona igual). Se elige por ángulo y no por trace directo para que las balas y los pedazos pequeños sean fáciles de apuntar.
- **Grupo:** al apuntar un objeto se toma su **grupo de causa**: los pedazos de un mismo rompible van juntos, y una bala va junto con los pedazos de lo que rompió. Implementación: `UTemporalRecorderComponent::LinkWith(Other)` (enlace en ambos sentidos, con weak pointers) y `GatherGroup(TSet&)` (recorrido BFS). `ATemporalBreakable::Break` enlaza sus pedazos entre sí; `ATemporalProjectile` enlaza la bala con los pedazos de lo que rompe (`ATemporalBreakable::GetShardRecorders()`).
- **Resaltado:** `SetOverlayMaterial` en todos los `UMeshComponent` del grupo con un MID de `M_TemporalGhost` en ámbar y alfa ~0.35. Al dejar de apuntar se restaura la capa anterior (normalmente nula).
- **Mantener R:** el objetivo queda **fijo** (aunque muevas la cámara), `StartRewind` en todo el grupo, energía 15/s. Al soltar, al acabarse la energía o cuando el grupo termina de rebobinar: `StopRewind`. Sin objetivo: `Deny()` y no pasa nada.
- **Shift+R = rebobinar todo** (lo de hoy), para pruebas. Se revisa con `PC->IsInputKeyDown(EKeys::LeftShift)` dentro del mismo handler de R. No gasta energía.
- Pistas de la sala 2 y de la sala 4 actualizadas: "apunta y mantén R".
- **Sala 2 · decisión de Hugo: una por una.** Cada plataforma es su propio objetivo (no se enlazan). Las plataformas caen al piso del vacío (z local −800), que queda dentro de los 35 m, así que se pueden apuntar ya caídas.

### 6.6 Sobremarcha (V) — `UOverdriveComponent` (nuevo)
- **V alterna** encender/apagar. Al encender: `SetGlobalTimeDilation(0.25)` y el jugador con `CustomTimeDilation = 1/0.25` (= 4), así se mueve y anima a velocidad normal. Al apagar, al acabarse la energía o en `EndPlay`: todo vuelve a 1.
- Post-process propio (`bUnbound`): estilo A pero en **cian verdoso**, viñeta suave y algo de aberración. Estela con `UTemporalTrailComponent`, color cian `(0.2, 0.9, 0.8, 0.2)`.
- **F y V se excluyen:** encender uno apaga el otro (decisión del builder: evita mezclar dilataciones y post-process).
- **F y R también se excluyen** (decisión del builder): empezar uno apaga el otro.
- **Durante V, los poderes del jugador van a velocidad real y el mundo lento** (decisión del builder): el rebobinado de R usa `max(DeltaTime, FApp::GetDeltaTime())` y la burbuja cuenta su vida (`LifeSeconds`) y su animación en tiempo real, en vez de `SetLifeSpan`. Los ecos sí van lentos.
- Se asume que el Registro del jugador sigue bien, porque su `DeltaTime` dilatado (0.25 × 4) es tiempo real. Las balas, los ecos y la física van lentos porque usan la dilatación global.

### 6.7 Eco (E) — `AEchoGhost`
- Pose mezclada entre `Frames[FrameIndex-1]` y `Frames[FrameIndex]` con el mismo `Alpha` que ya se usa para la posición.
- Cuesta 10 de energía (lo cobra el Sandbox antes de crearlo).

### 6.8 Burbuja (Q)
- Cuesta 30 (lo cobra el Sandbox). Sin otros cambios.

## 7. Lo que decide el builder
Además, de la revisión: F/R excluidos, poderes a tiempo real durante V, burbuja con vida en tiempo real.
 (marcado como decisión suya)
Los costos de energía, los colores exactos, el tamaño del pool (4), el intervalo y la vida de la estela, el ángulo de 4° y el alcance de 35 m para apuntar, que la Sobremarcha sea alterna (V prende y apaga) y que F y V se excluyan. Todo queda como `UPROPERTY` editable.

## 8. Cómo sabremos que está bien
- Compila sin warnings (editor y juego).
- Hugo prueba en Play: F se ve como antes pero más limpio y sin bajar FPS (`stat fps` antes y después); R rebobina solo lo que apuntas (salas 2 y 4); Shift+R rebobina todo; V hace el mundo lento con su estela; la barra baja, se recarga y parpadea en rojo al no alcanzar; el Eco se mueve suave.
- El log (por el MCP) sin errores nuevos.
