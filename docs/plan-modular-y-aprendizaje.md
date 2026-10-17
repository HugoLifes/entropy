# Plan: ordenar el código y aprender en el camino

Fecha: 17 oct 2026. Para Hugo, que quiere aprender y poder hacer cosas por su cuenta.

## Por qué

Hoy todo funciona, pero el código está así:
- **82 archivos en una sola carpeta** (`Source/entropy/`).
- **`TemporalSandbox.cpp` hace demasiado**: teclas, crear salas, poderes del jugador y los dos tours. Son unas 1500 líneas.
- **Los números de los poderes** (cuánto dura, cuánto cuesta) solo se cambian en C++, así que hay que compilar para cada ajuste.
- **Las salas se construyen con código.** No se pueden editar en el editor de Unreal.

Ordenarlo nos sirve de dos formas. Agregar fases (parkour, armas, enemigos) se vuelve más fácil. Y tú puedes hacer muchas cosas **en el editor, sin C++**.

## Parte 1 · Ordenar el código (lo hace la IA, en pasos chicos)

Cada paso compila, pasa las pruebas y los dos tours antes de seguir. El juego no cambia.

| # | Paso | Para qué |
|---|---|---|
| 1 | **Carpetas:** `Core/` (Registro, árbitro, energía, vida), `Poderes/`, `Mundo/` (salas, rompibles, balas, torretas, maniquí), `FX/` (distorsión, estela, HUD), `Dev/` (Sandbox, tours, pruebas) | Encontrar las cosas rápido |
| 2 | **Partir el Sandbox:** las teclas a un componente `UEntropiaInputComponent`; los tours a `EntropiaTourNormal.cpp` y `EntropiaTourCaos.cpp`; crear las salas en un `UEntropiaRoomsSubsystem` | Que cada archivo haga una sola cosa |
| 3 | **Clase base de poder** `UTemporalPowerComponent`: costo, iniciar/terminar, color en la barra, "no se puede ahora" | Cada poder nuevo (los de JoJo) se hace copiando un patrón |
| 4 | **Ajustes en el editor:** un Data Asset `DA_Poderes` con todos los números (dash, Detener, reflejo, Ojo lento, vida…) | **Tú** ajustas sin compilar |
| 5 | **Blueprints hijos:** `BP_Maniqui`, `BP_Torreta`, `BP_Burbuja`, `BP_Bala` | **Tú** cambias mallas, colores y efectos desde el editor |
| 6 | **Enhanced Input:** las teclas como assets (`IA_Detener`, `IA_Dash`…) en vez de `BindKey` | Cambiar controles y usar mando |
| 7 | **Salas como niveles:** las salas pasan a ser un nivel hecho a mano en el editor con los Blueprints del paso 5 (el código ya no las construye) | **Tú** diseñas niveles |

**Orden sugerido:** 1 y 2 antes del parkour. El 3 junto con el parkour, porque correr por la pared también es un "poder" de movimiento. Del 4 al 7, de a uno, cuando quieras aprender esa parte.

## Parte 2 · Aprender (tareas para ti, de fácil a difícil)

Cada tarea es útil para el juego y la puedes hacer **sin C++**. Al terminarla, me dices qué hiciste y yo reviso o conecto lo que falte.

| Nivel | Tarea | Qué aprendes | Cuándo |
|---|---|---|---|
| 1 | Moverte por el editor: abrir el nivel, ver los actores, cambiar valores en el panel *Details* del Temporal Sandbox (por ejemplo `BubbleSeconds`) y dar Play | El editor, propiedades, Play | Ya |
| 1 | Cambiar el color de la burbuja o del maniquí (`VisualColor`, materiales) | Materiales e instancias | Ya |
| 2 | Hacer un **nivel de prueba en gris** (cubos de *Modeling Mode*) con rampas, paredes y bordes para el parkour | Diseño de niveles (blockout) | Antes de la Fase 3b |
| 2 | Ajustar los números de `DA_Poderes` hasta que el dash y el reflejo se sientan bien | Balance de juego | Después del paso 4 |
| 3 | Un efecto **Niagara** propio (chispas de bala congelada, polvo del dash) | Efectos visuales | Cuando quieras |
| 3 | Buscar en Fab o Mixamo animaciones de parkour (trepar, saltar barda, correr en pared) e importarlas | Animación: importar y *retarget* | Fase 3b |
| 4 | Un **montaje** (AnimMontage) de remate nuevo con su *notify* | Animación avanzada | Fase 3b / 4 |
| 4 | El **HUD con UMG** en el diseñador (barras, mira, texto) en vez del dibujo en C++ | Interfaz | Pulido |
| 5 | Sonido: un MetaSound o Sound Cue para Detener, el dash y el reflejo | Audio | Pulido |
| 5 | Leer un componente chico de C++ (`TemporalHealthComponent`) con la IA explicándolo línea por línea | Leer C++ de Unreal | Cuando quieras |

**Lo que sigue siendo trabajo de la IA:** física, el árbitro del tiempo, la fractura, shaders HLSL, las pruebas automáticas, la IA de enemigos y todo lo que necesite compilar C++.

**Lo que es mejor que hagas tú**, porque requiere ojo y gusto: cómo se ve, cómo se siente (números), animaciones, niveles, sonido y probar en Play.

## Tareas importantes que vienen (reconteo)

1. Partir el Sandbox y ordenar en carpetas (Parte 1, pasos 1 y 2).
2. **Fase 3b · Parkour:** bordes, saltar por encima, correr en pared y piruetas, con un nivel de prueba hecho por ti.
3. Fase 4 Armas → 5 Enemigos e IA → 6 Lo difícil → 7 Pulido.
4. Pulido por áreas: ramas de habilidades (JoJo: DIO, Jotaro, Pucci), movimiento más real, interacción con el entorno.
5. Pendientes chicos anotados en `HANDOFF.md` (secciones 16 a 19).
