# Fase 3b · Parkour, movimiento real, pelea cuerpo a cuerpo y "Detener" nivel 2 — Diseño

Fecha: 17 oct 2026 · Proyecto: ENTROPÍA (`entropy/`, UE 5.8 C++) · Usuario: Hugo (prueba en Play; no programa)

## 1. Intención
Hugo: **"Parkour y más realismo al movimiento, posible combo de golpes, cubrirse en paredes, agacharse, correr, hacer el 'gato', 'dash' (el movimiento de parkour, no el desliz), agarrarse de orillas o salientes, caídas, agarrar y someter a los dummies o desarmarlos"**. También: **"hay que tomar en cuenta el posible nuevo diseño del Detener el tiempo"**.
La escena que imagina: un dummy armado se cubre y te dispara; lo descubres, **lo detienes y le rompes el cuello**, o le das **un golpe animado con enfoque**, o le **agarras el brazo** (estilo **Tang Soo Do**).
Del guion (`entropia-diseno/guion.md`): un hombre de casi 200 años que aparenta 60, un **asesino brutal** (Logan, Arkham Knight). El movimiento debe ser **realista y con peso**, no acrobático de caricatura. Remates: llaves, sumisiones y golpes (jab, recto, volado, gancho). Detener el tiempo al estilo **Star Platinum: The World**.
Hugo dijo "me gusta, arranca" sin más preguntas: las decisiones de la sección 6 son de la IA.

## 2. Metas (5 partes, en este orden; cada una termina con su prueba en el tour)
**A · Moverse con peso**
1. **Correr:** mantener Shift. Al presionar sigue saliendo el dash de antes; si lo mantienes, sigues corriendo.
2. **Agacharse:** C. Más lento y más bajo (pasas por huecos).
3. **Caídas:** de poca altura, normal. De altura media, si vas en movimiento **ruedas** (sin daño); si estás quieto, **aterrizaje duro** (te frenas un momento). De mucha altura, aterrizaje duro **con daño**.
4. **Peso:** aceleración y giro menos instantáneos, y la cámara sigue con un poco de retraso al correr.

**B · Parkour (Espacio = acción según lo que tengas enfrente)**
5. **Dash vault:** saltar por encima de una barda baja y delgada, con los pies por delante.
6. **El gato (kong / cat pass):** si vas corriendo y el obstáculo es más largo (una mesa, una caja), lo pasas con las manos por delante.
7. **Subir:** a algo bajo, de un paso; a algo alto, trepando.
8. **Agarrarse de orillas:** al saltar o caer cerca de un borde te cuelgas. Te mueves de lado, **Espacio** sube y **C** te suelta.
9. **Correr por la pared hacia arriba** (si vas corriendo hacia una pared alta) para alcanzar una orilla.
10. **Sala 6 · Parkour** (tecla 6) con bardas, cajas, muros, cornisas y un hueco para caer.

**C · Cubrirse y el dummy armado**
11. **Cubrirse:** C junto a una pared o una barda te pega a ella. Te mueves a lo largo y, al apuntar, te asomas; si no apuntas, quedas cubierto.
12. **Dummy armado:** un maniquí con pistola, cubierto detrás de un muro. Cada tanto se asoma, te dispara y se vuelve a cubrir. Congelado no hace nada.
13. **Sala 7 · Combate** (tecla 7) con muros de cobertura y 3 dummies armados.

**D · Pelea (estilo Tang Soo Do)**
14. **Combo de golpes:** clic izquierdo **sin apuntar**. Tres golpes seguidos si vas haciendo clic a tiempo; el tercero tumba. Los golpes empujan y aturden a los dummies y les interrumpen el disparo.
15. **Tres formas de acabarlos:**
    - **Romper el cuello:** por la espalda (sin que te vea), a un congelado o a uno sometido. Rápido y en silencio.
    - **Golpe con enfoque:** el remate de la Fase 3, ahora con la cámara acercándose al golpe. Es para un congelado de frente.
    - **Agarrar el brazo (E):** de cerca, le **quitas el arma** (cae al piso) y lo **sometes** unos segundos. Ahí puedes romperle el cuello (clic) o soltarlo (E).
16. **Clic izquierdo apuntando** sigue siendo **Detener** (con la mira). "CLIC: REMATE" gana cuando aparece.

**E · Detener el tiempo nivel 2 (Star Platinum: The World)**
17. Con la burbuja en **nivel 2** (tecla N cambia el nivel 1 ↔ 2), **Q detiene el tiempo de todo el mundo** unos segundos, menos el tuyo:
    - **Impacto:** destello en negativo, cielo naranja y amarillo, sombras blancas.
    - **Onda:** sale de ti y, detrás de ella, el mundo queda **gris metálico e índigo**.
    - **Aura:** solo tú tienes color (púrpura, dorado y cian).
    - **Final:** la onda regresa hacia ti y vuelve el color.
    - Las balas, torretas, dummies y cajas se quedan quietos; puedes rematarlos congelados.
18. Nivel 1 = la burbuja de siempre. El nivel 3 (grietas en el aire, destrucción que revienta al soltar) queda para la fase de pulido de poderes.

## 3. No-metas
Animaciones nuevas hechas por la IA (no puede descargarlas), cambio de sistema de movimiento a Mover, correr por la pared de lado, enemigos con IA de verdad (Fase 5: los dummies armados solo tienen una rutina), recoger y usar el arma del dummy (Fase 4), patadas reales (sin animación todavía), nivel 3 de Detener, sonido.

## 4. Anti-metas
- Que el parkour **se active solo** cuando no quieres. Solo Espacio lo dispara; al caer, solo agarras una orilla si vas hacia ella.
- Que se rompa lo de antes: el dash (tocar Shift), los poderes, los remates de la Fase 3, los tours normal y caos y las salas 1 a 5.
- Que el parkour atraviese paredes o deje al personaje atorado (siempre revisa espacio libre antes de moverlo).
- Que el Paso Inverso (F), el dash o los poderes dejen el parkour o la cobertura a medias. Siempre se puede salir.
- Que "Detener nivel 2" deje el mundo congelado o sin color si mueres, te teletransportas o se acaba la energía.

## 5. Restricciones
- C++ en `Source/entropy/` (carpetas Core, Poderes, Mundo, FX, Dev). No tocar `Variant_*` ni `UE_5.8`.
- **Animaciones intercambiables:** cada acción tiene un "espacio" de animación en un **set de animaciones** (`UEntropiaAnimationSet`, Data Asset `DA_Animaciones` que el editor crea solo si falta). Ahí Hugo cambia las provisionales por las buenas sin tocar C++. Mientras tanto se usan las de la plantilla.
- **El movimiento del parkour lo hace el C++** (trayectoria calculada sobre el obstáculo, con revisión de espacio libre). La animación solo es el aspecto, así que se puede cambiar sin romper nada.
- Tiempo del jugador: el parkour usa el DeltaTime del jugador (con la Sobremarcha se compensa solo).
- Pruebas automáticas para la lógica pura. Tour normal y caos con pasos nuevos.

## 6. Decisiones de la IA (Hugo puede cambiarlas)
- **Teclas:**
  - Shift: tocar = dash; mantener = correr.
  - C: agacharse; junto a una pared o barda, cubrirse.
  - Espacio: saltar o parkour.
  - E: agarrar.
  - El **Eco pasa a H** (está en pausa en el guion); G sigue borrando ecos.
  - Clic izquierdo: sin apuntar = golpes; apuntando = Detener; con aviso = remate.
  - N: nivel de Detener (prueba).
  - 6 y 7: salas nuevas.
- **"El gato" = kong vault / cat pass** (saltar un obstáculo con las manos por delante). Agarrar orillas al saltar ya cubre el "cat leap".
- **Velocidades:** caminar/trotar 450, correr 700, agachado 220. Giro 360°/s (antes 500).
- **Parkour:**
  - Dash vault: obstáculo de 50 a 130 cm de alto y hasta 70 cm de fondo, con piso del otro lado.
  - Gato: corriendo, mismo alto, de 70 a 220 cm de fondo.
  - Subir bajo: hasta 130 cm (sin espacio para pasar por encima).
  - Trepar alto: de 130 a 230 cm.
  - Correr por la pared: de 230 a 340 cm, solo corriendo.
  - Colgarse: borde entre 120 y 230 cm sobre los pies, al saltar o caer de frente.
- **Caídas:** menos de 4 m, normal. De 4 a 9 m: rueda si te mueves; si no, aterrizaje duro de 0.5 s. Más de 9 m: daño de 10 por metro extra (y aterrizaje duro).
- **Cobertura:** muro de 70 cm o más (bajo, agachado) o de 160 cm o más (alto, de pie). Distancia máxima 120 cm. Al apuntar te asomas: por arriba si es bajo, por el lado si estás en una esquina.
- **Dummy armado:**
  - Se asoma cada 2.5 s, dispara 2 balas (1800 cm/s, 20 de daño) y se cubre de nuevo.
  - Rutina sencilla, sin pensar.
  - Su pistola es un modelo hecho con cubos, pegado a su mano.
- **Golpes:**
  - Combo de 3 (`MM_Attack_01/02/03` de la plantilla) con ventana de 0.5 s.
  - Golpe a menos de 150 cm, en un cono de 70°.
  - Los dos primeros aturden 0.6 s; el tercero tumba (como el remate).
- **Agarrar:**
  - De frente o de espalda, a menos de 150 cm, si el dummy está de pie.
  - Desarmar si tiene arma.
  - Sometido 4 s: clic = romper el cuello; E = soltar con empujón.
- **Detener nivel 2:**
  - 60 de energía, 4 s reales, mundo a 0.001.
  - Efecto con un material de post-proceso nuevo (`M_TemporalWorldStop`). Para que solo tú tengas color se usa el **Custom Stencil** (`r.CustomDepth=3`).

## 7. Animaciones (el plan con Hugo)
- **Provisionales que ya existen:** salto, caída, aterrizaje (pose agachada y de aterrizaje duro), salto de pared, dash, golpes 1-3, golpe cargado, reacciones a golpes, muertes, pistola (apuntar y disparar).
- **Tarea de Hugo (aprendizaje, en paralelo):** descargar el **Game Animation Sample Project** (Epic, gratis, en Fab). Trae animaciones de captura de movimiento: agacharse, saltar vallas, trepar, aterrizajes, arranques y frenadas. Después, buscar animaciones de **artes marciales** (Mixamo o Fab) para patadas, llaves y desarmes.
- **Cuando lleguen:** se ponen en `DA_Animaciones` (en el editor) y, si conviene, se migra el sistema de movimiento de Epic (Motion Matching). Eso será una tarea aparte ("realismo", después de la 3b).

## 8. Cómo sabremos que está bien
- **Pruebas automáticas** (lógica pura):
  - clasificar el obstáculo (vault, gato, subir, trepar, pared, nada);
  - trayectoria del parkour (empieza, pasa por encima y termina donde debe);
  - caídas (rueda, duro, daño);
  - combo (ventana y siguiente golpe);
  - condición de agarre;
  - rutina del dummy armado (cubierto, asomado, disparo);
  - forma del efecto de Detener nivel 2 (radio de la onda en el tiempo).
- **Tour normal:** pasos nuevos para cada movimiento en la sala 6 y cada pelea en la sala 7, con capturas.
- **Tour caos:** parkour con F, dash y Sobremarcha a la vez; morir colgado o cubierto; Detener nivel 2 con todo encima. Que el mundo y el jugador vuelvan a 1.
- **Hugo lo prueba en Play.**
