# ENTROPÍA

Juego de acción en tercera persona sobre **romper el tiempo**. Detienes balas en el aire, haces que una pared rota se rearme, regresas tus pasos, esquivas en cámara lenta, rematas a enemigos congelados y detienes el mundo entero. Con parkour, cobertura, pelea cuerpo a cuerpo y **disparos donde cada bala vive en el tiempo**.

Hecho en **Unreal Engine 5.8 con C++**. Prototipo en cuartos grises (sin arte final todavía).

![Invertir una bala: regresa al cañón y la pared se rearma](docs/capturas/invertir-bala.jpg)

## Qué se puede hacer hoy

| | |
|---|---|
| ![Sobremarcha](docs/capturas/sobremarcha.jpg) **Sobremarcha (V):** el mundo va lento y tú normal. | ![Burbuja](docs/capturas/burbuja.jpg) **Burbuja (Q):** dentro, el tiempo se detiene (balas, cajas, ecos). |
| ![Dash](docs/capturas/dash.jpg) **Dash (Shift)** con estela, también en el aire. | ![Tiempo de reflejo](docs/capturas/tiempo-de-reflejo.jpg) **Esquiva perfecta:** dash justo antes de que te pegue una bala → *tiempo de reflejo*. |
| ![Remate](docs/capturas/remate.jpg) **Remate** por la espalda: desde la fase 3b le **rompes el cuello** y cae como muñeco de trapo. | ![Remate congelado](docs/capturas/remate-congelado.jpg) **Detener + remate:** congelas al maniquí y el golpe se guarda hasta que su tiempo vuelve. |
| ![Paso Inverso](docs/capturas/paso-inverso.jpg) **Paso Inverso (F):** regresas por donde viniste, con tu pose al revés. | Y además: **Eco** (un fantasma repite tus últimos segundos), **Invertir objeto** (R), **destrucción** que se rebobina pieza por pieza, **Ojo lento**. |
| ![Mundo detenido](docs/capturas/mundo-detenido.jpg) **Detener el mundo (N y luego Q):** todo se para menos tú; el mundo queda gris e índigo y tú a color. | ![Impacto](docs/capturas/mundo-impacto.jpg) **El impacto:** al empezar, un destello en negativo (cielo naranja) y una onda que sale de ti. |

### Parkour y pelea (fase 3b)

| | |
|---|---|
| ![Gato](docs/capturas/gato.jpg) **Parkour (Espacio frente a algo):** dash vault, *el gato* (kong), subir, trepar y correr por la pared. | ![Orilla](docs/capturas/orilla.jpg) **Orillas:** te cuelgas, te mueves de lado, subes o te sueltas (C). |
| ![Cobertura](docs/capturas/cobertura.jpg) **Cobertura (C junto a una pared):** baja o alta; al apuntar te asomas por arriba o por la esquina. | ![Golpes](docs/capturas/golpes.jpg) **Combo de 3 golpes (clic sin apuntar):** el tercero tumba. |
| ![Golpe con enfoque](docs/capturas/golpe-enfoque.jpg) **Golpe con enfoque** de frente (cámara lenta y cámara cerca) · **romper el cuello** por la espalda. | ![Agarrar](docs/capturas/agarrar.jpg) **Agarrar (E):** desarmas al dummy y lo sometes; clic = romper el cuello, E = soltar con empujón. |

### Disparos con el tiempo (fase 4)

Las balas no son rayos invisibles: son objetos que el tiempo puede congelar, rebobinar y devolver.

| | |
|---|---|
| ![Revólver](docs/capturas/revolver.jpg) **Revólver "Aurora"** (el arma del viejo): 6 tiros, lento, pega muy fuerte. Apuntar + clic. | ![Fusil](docs/capturas/fusil.jpg) **Fusil:** automático, 30 tiros. **Rueda** cambia de arma, **R** (sin nada que invertir en la mira) recarga. |
| ![Lluvia detenida](docs/capturas/lluvia-detenida.jpg) **Lluvia detenida:** con el mundo detenido (N y Q), tus balas salen 1.5 m y se quedan flotando; al volver el tiempo salen todas juntas. | ![Último en caer](docs/capturas/ultimo-en-caer.jpg) **Último en caer:** si tu bala tumba al último enemigo de la sala, cámara lenta y la cámara se acerca. |
| ![Sala 8](docs/capturas/sala8.jpg) **Sala 8 · Tiroteo** (tecla 8): 4 dummies a 12, 18, 25 y 30 m, un muro y cajas que se rompen. | **Devuélvesela:** R a una bala enemiga y regresa a su cañón… y le pega a él. **Recarga inversa:** R a una bala tuya y vuelve a tu cargador. **Tiros guardados:** dispárale a alguien detenido y todos los impactos llegan juntos cuando su tiempo vuelve. |

Los modelos de armas vienen de Fab y **no están en el repositorio** (la licencia no deja publicarlos): se importan con `Tools\import-armas.ps1`. Sin ellos, las armas se ven como cubos y todo funciona igual.

También: **correr** (mantener Shift), **agacharse** (C), **caídas** (rodar si caes moviéndote, aterrizaje duro, daño si caes de muy alto; en la sala 6 hay una escalera hasta un balcón de 10 m para practicarlas) y **dummies armados** que se cubren, se asoman y te disparan (sala 7).

## Controles

| Tecla | Qué hace |
|---|---|
| Clic derecho | Apuntar (cámara al hombro, mira que selecciona; las balas enemigas cerca se frenan: *Ojo lento*) |
| Clic izquierdo | Sin apuntar: **golpes** (combo de 3) · apuntando: **disparar** (mantener con el fusil) · o **rematar / romper el cuello** si aparece el aviso |
| Rueda del ratón | Cambiar de arma (revólver / fusil) |
| Espacio | Saltar · frente a algo: **parkour** (vallas, el gato, subir, trepar, pared) · en el aire cerca de una orilla: **colgarse** · colgado: subir |
| C | **Agacharse** · junto a una pared: **cubrirse** · colgado: soltarse |
| E | **Agarrar** a un maniquí (lo desarma y lo somete) · otra vez: soltar |
| R (mantener) | Invertir lo que apuntas: regresa en el tiempo · sin nada que invertir en la mira: **recargar** |
| F (mantener) | Paso Inverso: tú regresas en el tiempo |
| Shift | Dash · mantener: **correr** |
| V | Sobremarcha |
| Q | Burbuja que detiene el tiempo · apuntando a algo: **Detenerlo** (4 s) · con **N** (nivel 2): **detener todo el mundo** |
| H / G | Crear eco / borrar ecos |
| Ctrl+R | Rebobinar todo (pruebas) |
| 1 · 2 · 3 · 4 · 5 · 6 · 7 · 8 · 0 | Salas: Eco · Inverso · Brecha · Física · Reflejos · Parkour · Combate · Tiroteo · inicio |
| Z / X / B | Sala 4: disparar · bala invertida · romper lo de enfrente |

## Cómo abrirlo

1. Unreal Engine 5.8 y Visual Studio 2022 (C++).
2. Clonar con Git LFS (`git lfs install` y luego `git clone`).
3. Abrir `entropy.uproject`, abrir el nivel `Lvl_ThirdPerson` y dar Play. El actor **Temporal Sandbox** arma las salas y las teclas.

Compilar desde la consola (con el editor **cerrado**) y correr las pruebas:

```powershell
powershell -ExecutionPolicy Bypass -File Tools\build-test.ps1        # compila + 53 pruebas automáticas
powershell -ExecutionPolicy Bypass -File Tools\run-tour.ps1          # el juego se prueba solo y toma capturas
powershell -ExecutionPolicy Bypass -File Tools\run-tour.ps1 -ExtraArgs '-EntropiaTour=caos'   # cosas raras a propósito
```

## Cómo está hecho (resumen)

- **Registro Temporal** (`UTemporalRecorderComponent`): graba cada frame (posición, velocidad, pose) y lo reproduce al revés. Es la base de todos los poderes.
- **Árbitro del tiempo** (`UTemporalDilationSubsystem`): cada poder pide una velocidad de tiempo y gana la más lenta. Así los poderes se combinan sin pisarse.
- **Fractura propia** (Voronoi) con pedazos que se rebobinan, y escombros y polvo calculados con fórmula (son reversibles exactos).
- **Distorsión del tiempo**: un post-process con HLSL propio (ondas, burbujas, y el mundo detenido: gris e índigo con el jugador a color por Custom Stencil).
- **Parkour**: el C++ mide el obstáculo (alto y fondo) y elige el movimiento; mueve la cápsula por una curva y la animación solo se ve. Las animaciones están en un archivo editable (`Content/Entropia/Datos/DA_Animaciones`) para cambiar las provisionales sin tocar código.
- **Armas**: los números de cada arma (cargador, cadencia, daño, dispersión, retroceso) y sus modelos están en `Content/Entropia/Datos/DA_Armas`, editable sin código. Las balas son actores con Registro Temporal (por eso se pueden detener, rebobinar y devolver).
- Código en `Source/entropy/`, por carpetas: `Core/` (tiempo, energía, vida, animaciones), `Poderes/`, `Movimiento/` (correr, caídas, parkour, cobertura), `Pelea/` (golpes, agarrar), `Armas/` (revólver, fusil, lógica de cargador y dispersión), `Mundo/` (salas, destrucción, enemigos de práctica), `FX/` y `Dev/` (banco de pruebas, tours y pruebas). Diseños y planes de cada fase en `docs/superpowers/`. Plan para ordenar el código y aprender: [`docs/plan-modular-y-aprendizaje.md`](docs/plan-modular-y-aprendizaje.md).

## Hoja de ruta

✅ 1 Pulir el tiempo · ✅ 2 Destrucción y partículas · ✅ 3 Moverse, reflejos y remates · ✅ 3b Parkour, cobertura, pelea y detener el mundo · 🧪 4 Disparos con el tiempo (en prueba) · ⏳ 4b Golpes básicos y remates realistas · ⏳ 5 Enemigos e IA · ⏳ 6 Lo difícil (cobertura, IA avanzada) · ⏳ 7 Pulido · después: ramas de habilidades (con inspiración de JoJo: DIO, Jotaro, Pucci), movimiento más real e interacción con el entorno.
