# Fase 2 · Destrucción, partículas y distorsión del tiempo — Diseño

Fecha: 17 oct 2026 · Proyecto: ENTROPÍA (`entropy/`, UE 5.8 C++) · Usuario: Hugo (prueba en Play; no programa)

## 1. Intención
El "wow" del juego es **cómo se ve alterar el tiempo e interactuar con él**. En esta fase:
- Romper cosas se debe ver bien: pedazos irregulares, escombros y polvo.
- Rearmar al revés tiene que verse igual de bien: el polvo y los escombros **regresan** junto con los pedazos.
- Los poderes ganan el efecto de **distorsión del tiempo** ("time shifter" / "refraction time"), que Hugo movió de la Fase 7 a esta.
- Hugo pidió que la arquitectura sea **fluida**: un solo lugar para los efectos, sin repetir código en cada poder, y que no baje los FPS en una GTX 1650.

Al terminar, **la IA prueba el juego sola** (modo tour con capturas y FPS) y propone qué más afinar.

## 2. Metas
1. **Fractura irregular** (estilo Voronoi) de `ATemporalBreakable`: celdas convexas, más pequeñas cerca del punto de impacto, con cara interior de otro color. Cada pedazo es un actor con física y Registro Temporal, así que R y Shift+R lo rearman como hoy.
2. **Escombros y polvo reversibles**: movimiento analítico en función del tiempo T (posición = p0 + v·T + ½g·T², con piso), dibujado con `UInstancedStaticMeshComponent` (una llamada de dibujo por tipo). Cuando los pedazos se rebobinan, T baja y el polvo vuelve al punto de impacto.
3. **Distorsión del tiempo**: material de post-process `M_TemporalDistortion`, generado por el editor como `M_TemporalGhost`, con:
   - hasta **4 ondas** (anillos que doblan la imagen hacia afuera o hacia adentro, con aberración cromática);
   - un **aura** (ondulación tipo calor alrededor de un punto en pantalla).
   Lo controla `UTemporalFXSubsystem` (`UTickableWorldSubsystem`); los poderes solo llaman `Pulse(...)` y `SetAura(...)`.
4. **Usos**:
   - F: onda hacia adentro al empezar, aura en el jugador mientras dura y onda chica al soltar.
   - V: onda grande al encender.
   - R: onda en el objetivo.
   - Q: onda al nacer la burbuja.
   - Romper: onda de impacto. Rearmar: onda hacia adentro.
5. **Modo tour** (`-EntropiaTour` en la línea de comandos): el Sandbox recorre las salas, usa los poderes por código, registra los FPS por paso (`ENTROPIA_TOUR ...` en el log), toma capturas (`FScreenshotRequest`) y cierra el juego. La IA lo lanza con `UnrealEditor.exe entropy.uproject -game` y revisa las capturas y el log.

## 3. No-metas
- Chaos Destruction / Geometry Collections: requiere assets fracturados a mano en el editor, sus pedazos no son actores (el Registro no los ve) y pesa más en la GTX 1650.
- Agarrar y lanzar objetos, detener o retroceder personas, asesinatos sigilosos: ideas anotadas para después (HANDOFF §15).
- Sonido.

## 4. Anti-metas
- Que romper algo cause un tirón visible: la fractura debe calcularse en < 2 ms para ~12 pedazos.
- Que la distorsión cueste FPS cuando no hay efectos activos: sin ondas ni aura, el peso del blendable debe quedar en 0.
- Romper lo que ya funciona: R apuntado, grupos (una bala junto con lo que rompió), la sala 4 y Shift+R.

## 5. Restricciones
- C++ en `Source/entropy/` (estructura plana), sin assets hechos a mano.
- Plugin `ProceduralMeshComponent` (viene con el motor) para las mallas de los pedazos.
- Unity build: nada de nombres genéricos en namespaces anónimos.
- Tiempo real con `TemporalFX::RealDelta()`.
- Calidad configurable con la CVar `entropia.fx.distortion` (0 = apagado).

## 6. Diseño por pieza

### 6.1 `TemporalFracture` (lógica pura, probada)
- `struct FFracturePolygon { TArray<FVector> Points; bool bInterior; }` y `struct FFractureCell { TArray<FFracturePolygon> Faces; }`.
- `FFractureCell MakeBox(FVector HalfSize)`.
- `bool ClipCell(const FFractureCell& In, const FPlane& Plane, FFractureCell& Out)`: conserva el lado `Plane.PlaneDot(P) <= 0` y agrega una tapa interior (polígono ordenado alrededor de su centro).
- `TArray<FFractureCell> VoronoiFracture(FVector HalfSize, const TArray<FVector>& Seeds)`: cada celda = la caja recortada por los bisectores con las demás semillas.
- `TArray<FVector> MakeSeeds(FVector HalfSize, int32 Count, FVector LocalImpact, int32 Seed)`: la mitad de las semillas cerca del impacto, el resto uniforme (determinista con `FRandomStream`).
- `float CellVolume(const FFractureCell&)` y `FVector CellCentroid(...)` para las pruebas y para centrar cada pedazo.

### 6.2 `ATemporalShard` (nuevo) y `ATemporalBreakable`
- Raíz `UProceduralMeshComponent`: sección 0 = caras exteriores (color del objeto) y sección 1 = interiores (color más claro). Colisión convexa (`AddCollisionConvexMesh`, `bUseComplexAsSimpleCollision = false`), física, `PhysicsActor`, y Recorder (`MaxRecordSeconds = ShardMemorySeconds`).
- Los vértices se centran en el centroide de la celda (el actor queda en el centroide).
- `ATemporalBreakable`: `FractureMode` (`Irregular` por defecto, `Cubes` = el de hoy) y `NumShards` = 10. El rearmado (todos los pedazos a < 25 cm de su sitio), los grupos y `GetShardRecorders` no cambian.

### 6.3 `UTemporalDebrisComponent` (nuevo, en el rompible)
- `Burst(FVector ImpactPoint, FVector Direction)`: crea `NumChips = 14` astillas (cubos de 6 a 12 cm, color del objeto) y `NumDust = 10` nubes (esferas translúcidas grises que crecen y se encogen). Todo en 2 ISM.
- Pura y probada: `static FVector EvaluateChip(const FDebrisParticle&, float T, float FloorZ)` y `static float DustScale(float T, float Life)`.
- Tick: si algún pedazo del rompible está rebobinando, `T -= RealDelta() * RewindSpeed` (mínimo 0); si no, `T += RealDelta()` hasta `Life = 2.5 s`. Al rearmarse se oculta. Solo tickea mientras hay algo visible.

### 6.4 Distorsión: `M_TemporalDistortion` y `UTemporalFXSubsystem`
- El material se genera en el editor (como `M_TemporalGhost`): dominio PostProcess y un nodo Custom (HLSL) que lee `SceneTextureLookup(uv, 14, false)` con UV desplazada; hay un nodo SceneTexture(PostProcessInput0) conectado como entrada para que la textura quede ligada. Parámetros: `Pulse0..Pulse3` (x, y = centro en UV de viewport; z = radio; w = fuerza, negativa = hacia adentro), `Aura` (x, y, radio, fuerza) y `Thickness`. El tiempo y la proporción de pantalla salen de `View.RealTime` y `View.ViewSizeAndInvSize`.
- `UTemporalFXSubsystem` (`UTickableWorldSubsystem`):
  - `Pulse(FVector WorldLocation, float Strength, float Duration, float MaxRadius)`: el radio crece de 0 a `MaxRadius` con *ease-out* y la fuerza se apaga al final. Hay 4 lugares; si están llenos se reemplaza el más viejo.
  - `SetAura(AActor* Target, float Strength)`: si `Strength = 0` se apaga.
  - Cada frame proyecta las posiciones del mundo a la pantalla, actualiza el MID y pone `BlendWeight = 0` si no hay nada activo.
  - Crea un actor ligero con `UPostProcessComponent` (`bUnbound`, prioridad 3) y el MID como blendable.
- Pura y probada: `FTemporalPulseSet` (añadir, envejecer, radio por tiempo, reemplazo del más viejo).

### 6.5 Modo tour (`ATemporalSandbox`)
- Si la línea de comandos trae `-EntropiaTour`, después de `Setup` corre una secuencia con temporizadores:
  1. sala 4;
  2. disparar;
  3. romper una caja (B);
  4. invertir la bala (con objetivo asignado por código);
  5. Shift+R;
  6. V encendido y disparar;
  7. sala 2: dejar caer una plataforma e invertirla;
  8. inicio: moverse, F y E.
- Cada paso: log `ENTROPIA_TOUR step=<n> name=<texto> fps_avg=<x> fps_min=<y>` y una captura `Tour_<n>_<nombre>.png`.
- Al final, `ENTROPIA_TOUR done` y salir.
- `UObjectInversionComponent::BeginInversionOn(UTemporalRecorderComponent*)` para apuntar por código.

## 7. Decisiones de la IA (Hugo pidió empezar sin preguntas)
Voronoi propio en vez de Chaos, escombros analíticos con ISM en vez de Niagara, un solo material de distorsión con 4 ondas y un aura, 10 pedazos por objeto, 14 astillas y 10 nubes, tour por línea de comandos. Todos los números quedan como `UPROPERTY` o CVar.

## 8. Cómo sabremos que está bien
- Pruebas automáticas nuevas en verde: fractura (volumen conservado, número de celdas, determinismo), escombros (T = 0 en el origen, piso) y ondas (radio y reemplazo).
- Compila sin warnings (editor y juego).
- El tour corre de principio a fin, sin errores en el log y con los FPS registrados; la IA revisa las capturas.
- Hugo lo prueba en Play.
