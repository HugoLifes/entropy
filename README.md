# ENTROPÍA

Juego de acción en tercera persona sobre **romper el tiempo**. Detienes balas en el aire, haces que una pared rota se rearme, regresas tus pasos, esquivas en cámara lenta y rematas a enemigos congelados.

Hecho en **Unreal Engine 5.8 con C++**. Prototipo en cuartos grises (sin arte final todavía).

![Invertir una bala: regresa al cañón y la pared se rearma](docs/capturas/invertir-bala.jpg)

## Qué se puede hacer hoy

| | |
|---|---|
| ![Sobremarcha](docs/capturas/sobremarcha.jpg) **Sobremarcha (V):** el mundo va lento y tú normal. | ![Burbuja](docs/capturas/burbuja.jpg) **Burbuja (Q):** dentro, el tiempo se detiene (balas, cajas, ecos). |
| ![Dash](docs/capturas/dash.jpg) **Dash (Shift)** con estela, también en el aire. | ![Tiempo de reflejo](docs/capturas/tiempo-de-reflejo.jpg) **Esquiva perfecta:** dash justo antes de que te pegue una bala → *tiempo de reflejo*. |
| ![Remate](docs/capturas/remate.jpg) **Remate** por la espalda (cámara lenta y muñeco de trapo). | ![Remate congelado](docs/capturas/remate-congelado.jpg) **Detener + remate:** congelas al maniquí y el golpe se guarda hasta que su tiempo vuelve. |
| ![Paso Inverso](docs/capturas/paso-inverso.jpg) **Paso Inverso (F):** regresas por donde viniste, con tu pose al revés. | Y además: **Eco** (un fantasma repite tus últimos segundos), **Invertir objeto** (R), **destrucción** que se rebobina pieza por pieza, **Ojo lento**. |

## Controles

| Tecla | Qué hace |
|---|---|
| Clic derecho | Apuntar (cámara al hombro, mira que selecciona; las balas cerca se frenan: *Ojo lento*) |
| Clic izquierdo | **Detener** lo que está en la mira (4 s) · o **rematar** si aparece "CLIC: REMATE" |
| R (mantener) | Invertir lo que apuntas: regresa en el tiempo |
| F (mantener) | Paso Inverso: tú regresas en el tiempo |
| Shift | Dash |
| V | Sobremarcha |
| Q | Burbuja que detiene el tiempo |
| E / G | Crear eco / borrar ecos |
| Ctrl+R | Rebobinar todo (pruebas) |
| 1 · 2 · 3 · 4 · 5 · 0 | Salas: Eco · Inverso · Brecha · Física · Reflejos · inicio |
| Z / X / B | Sala 4: disparar · bala invertida · romper lo de enfrente |

## Cómo abrirlo

1. Unreal Engine 5.8 y Visual Studio 2022 (C++).
2. Clonar con Git LFS (`git lfs install` y luego `git clone`).
3. Abrir `entropy.uproject`, abrir el nivel `Lvl_ThirdPerson` y dar Play. El actor **Temporal Sandbox** arma las salas y las teclas.

Compilar desde la consola (con el editor **cerrado**) y correr las pruebas:

```powershell
powershell -ExecutionPolicy Bypass -File Tools\build-test.ps1        # compila + 35 pruebas automáticas
powershell -ExecutionPolicy Bypass -File Tools\run-tour.ps1          # el juego se prueba solo y toma capturas
powershell -ExecutionPolicy Bypass -File Tools\run-tour.ps1 -ExtraArgs '-EntropiaTour=caos'   # cosas raras a propósito
```

## Cómo está hecho (resumen)

- **Registro Temporal** (`UTemporalRecorderComponent`): graba cada frame (posición, velocidad, pose) y lo reproduce al revés. Es la base de todos los poderes.
- **Árbitro del tiempo** (`UTemporalDilationSubsystem`): cada poder pide una velocidad de tiempo y gana la más lenta. Así los poderes se combinan sin pisarse.
- **Fractura propia** (Voronoi) con pedazos que se rebobinan, y escombros y polvo calculados con fórmula (son reversibles exactos).
- **Distorsión del tiempo**: un post-process con HLSL propio (ondas, burbujas).
- Todo en `Source/entropy/`. Diseños y planes de cada fase en `docs/superpowers/`. Plan para ordenar el código y aprender: [`docs/plan-modular-y-aprendizaje.md`](docs/plan-modular-y-aprendizaje.md).

## Hoja de ruta

✅ 1 Pulir el tiempo · ✅ 2 Destrucción y partículas · ✅ 3 Moverse, reflejos y remates · ⏳ 3b Parkour · ⏳ 4 Armas · ⏳ 5 Enemigos e IA · ⏳ 6 Lo difícil (cobertura, IA avanzada) · ⏳ 7 Pulido · después: ramas de habilidades (con inspiración de JoJo: DIO, Jotaro, Pucci), movimiento más real e interacción con el entorno.
