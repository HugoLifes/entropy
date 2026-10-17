# Fase 4 · Disparos con el tiempo — Diseño

Fecha: 17 oct 2026 · Proyecto: ENTROPÍA (`entropy/`, UE 5.8 C++) · Usuario: Hugo (prueba en Play; no programa)

## 1. Intención
Hugo: **"arranca, ayúdate del guion y ayúdame un poco esta vez, que haga decir wow"**. No quiso preguntas: las decisiones de la sección 6 son de la IA.
Del guion (`entropia-diseno/guion.md`):
- Juego **RPG + disparos en tercera persona**: "mejor disparos que hack and slash" **[Hugo]**.
- Un hombre de casi 200 años, **asesino brutal** que quiere hacer lo correcto (Logan, Arkham Knight, Cable). Tenet: la inversión del tiempo.
- **Combate brutal [guion]:** "congelar a un enemigo a medio disparo, o **rebobinar su propia bala contra él**".
- Detener (Star Platinum: The World) ya existe en nivel 2. El nivel 3 de F hablará de "balas que vuelven a sus cañones".
- En el HANDOFF, la Fase 4 dice: **disparos primero**; golpes básicos y remates realistas después.

**La idea que hace "wow":** las balas de ENTROPÍA **no son rayos invisibles**. Son objetos que viven en el tiempo: se pueden congelar, rebobinar, devolver y guardar. Disparar debe sentirse sólido (retroceso, fogonazo, impacto, cuerpos que vuelan) y cada poder de tiempo debe tener una jugada con las balas.

## 2. Metas
**A · Armas que se sienten**
1. **Dos armas del jugador:**
   - **Revólver "Aurora"**, el arma insignia del viejo: 6 tiros, lento, pega muy fuerte y patea mucho.
   - **Fusil:** automático, 30 tiros, rápido, pega menos.
   - La **rueda del ratón** cambia de arma.
2. **Disparar:** apuntar (clic derecho) + **clic izquierdo**. El revólver dispara un tiro por clic; el fusil dispara mientras mantienes el clic.
   - La bala sale del cañón hacia lo que está en el centro de la mira (el punto que ve la cámara).
3. **Se siente:**
   - **Retroceso:** la cámara patea hacia arriba y regresa.
   - **Dispersión:** crece al disparar seguido y al moverte; la mira se abre.
   - **Fogonazo** en el cañón y **trazador** brillante (dorado) en cada bala.
   - **Chispas** al impactar.
   - **Marca de impacto** en la mira: blanca si pegaste, roja si lo tumbaste.
4. **Munición y recarga:**
   - El HUD muestra las balas: "REVÓLVER 6/6" o "FUSIL 30/30".
   - **R sin nada en la mira = recargar**. Si disparas con el arma vacía, también recarga sola.
   - Recargar tarda 1.6 s el revólver y 1.9 s el fusil, y la balas de reserva son infinitas (es un banco de pruebas).
5. **Armas visibles:**
   - El arma va en la mano derecha del personaje.
   - Los dummies armados usan una pistola de verdad en lugar del cubo.
   - Los modelos vienen de Fab y viven **solo en la compu de Hugo** (sección 5). Si faltan, el arma se dibuja con formas simples.

**B · Las balas dañan**
6. **Maniquíes y dummies tienen aguante (100):**
   - **Cabeza:** el revólver tumba de un tiro y el fusil hace triple daño.
   - **Torso:** el revólver hace 55 (2 tiros tumban) y el fusil 20 (5 tiros).
   - Si no lo tumba, el golpe lo **aturde** y le corta la rutina de disparo.
   - Si lo tumba, cae como **muñeco de trapo empujado por la bala**. A los 4 s de su tiempo, "el tiempo lo regresa" de pie y con el aguante lleno (como hoy).
7. **Las balas empujan** los cuerpos tirados y las cajas, y **rompen** los muros rompibles y los atraviesan, como ya hacen las de las torretas.
8. **Disparar a un congelado (Detener, burbuja o mundo detenido):**
   - Las balas **se quedan pegadas a él** sin hacer nada todavía.
   - Cuando su tiempo vuelve, **todos los impactos llegan juntos**: aguante, empujón y caída de una sola vez (estilo JoJo).

**C · Jugadas con el tiempo (el "wow")**
9. **"Lluvia detenida"** (Detener nivel 2, The World + revólver o fusil):
   - Con el mundo detenido, cada bala que disparas avanza 1.5 m y **se queda flotando** en el aire, brillando.
   - Puedes vaciar el cargador, moverte y disparar desde otro ángulo.
   - Cuando el tiempo vuelve, **todas salen a la vez**.
10. **"Devuélvesela"** (Invertir, R apuntando a una bala enemiga):
    - La bala regresa por su camino hasta el cañón del dummy que la disparó y **le pega a él** (le quita el aguante como un tiro a la cabeza del revólver: lo tumba).
    - Es la frase del guion: "rebobinar su propia bala contra él".
11. **"Recarga inversa"** (Tenet, R apuntando a una bala tuya):
    - La bala regresa a tu arma y **vuelve al cargador** (+1 bala, hasta el máximo).
    - Funciona con las balas incrustadas en la pared y con las que siguen volando. Ctrl+R (todo) también las devuelve.
12. **"Último en caer"** (cámara de remate):
    - Cuando tu bala tumba al **último** enemigo de pie de la sala, el mundo va a 0.15 durante 0.7 s reales y la cámara se acerca (la del golpe con enfoque).
    - Es el momento Max Payne o Sniper Elite.
13. **Lo que ya existía sigue funcionando con tus balas:**
    - La burbuja las frena o congela.
    - Detener congela una bala tuya en el aire.
    - El Ojo lento sigue siendo solo para las balas enemigas.

**D · Sala 8 · Tiroteo** (tecla 8)
14. Una arena para lucir todo:
    - 4 dummies armados detrás de coberturas a 12, 18, 25 y 30 m.
    - Un muro rompible frente a uno de ellos.
    - Cajas con física.
    - Tus coberturas.
    - Una pista en pantalla con las jugadas: Lluvia detenida, Devuélvesela, Recarga inversa y Último en caer.

## 3. No-metas (van después)
- **Golpes básicos** (jab, recto, volado, gancho), **llaves** y **remates con la Sobremarcha** (estilo Halo 5): **Fase 4b**. Dependen mucho de animaciones, y Hugo ya tiene las de Mixamo para el pulido.
- Animación de la parte de arriba del cuerpo por separado: apuntar caminando con los brazos al frente necesita un Anim Blueprint con capas (pulido).
- Economía de munición, recoger armas del piso, más armas (escopeta, francotirador).
- Enemigos con inteligencia de verdad (Fase 5).
- Sonido (todavía no hay audio en el proyecto).
- Subir los modelos de Fab a GitHub.

## 4. Anti-metas
- **Balas instantáneas (hitscan).** Rompen todo lo que hace especial al juego: cada bala debe ser un objeto que el tiempo pueda tocar.
- Que una bala del jugador **te lastime a ti** o se quede atorada dentro de tu cuerpo al nacer.
- Que el fusil baje los FPS en la GTX 1650:
  - Máximo **40 balas tuyas vivas**; al pasar el límite desaparece la más vieja.
  - Sin luces dinámicas por bala.
  - Las texturas de 4K del revólver se limitan a 1024.
- Que se rompa lo de antes:
  - Golpes sin apuntar.
  - Remates, agarre, cobertura y parkour.
  - Detener, la burbuja, el mundo detenido, F, R y Ctrl+R.
  - Las salas 1 a 7, las 47 pruebas y los tours normal y caos.
- Que un tiro "guardado" en un congelado se pierda o se dispare dos veces si el congelado se reinicia, se teletransporta o tú mueres.

## 5. Restricciones
- C++ en `Source/entropy/` (carpetas Core, Poderes, Mundo, FX, Dev, Movimiento, Pelea; se agrega **`Armas/`**). No tocar `Variant_*` ni `UE_5.8`.
- **El editor debe estar cerrado** al compilar. Hay una sola sesión compilando a la vez. Después de cada tarea: HANDOFF, commit y push de la rama.
- **Modelos de Fab:**
  - La licencia de Fab no deja publicarlos y el repo `HugoLifes/entropy` es **público**. Por eso los modelos importados van a `Content/Entropia/ArmasFab/`, que queda en el `.gitignore`.
  - Las fuentes están fuera del repo, en `D:\unreal projets\armas_assets\` (revolver.glb, fusil.fbx, pistola.fbx).
  - El script `Tools/import-armas.ps1` (en Git) las importa con el commandlet `ImportAssets`, sin abrir el editor. Ya se probó: las tres armas se importan sin errores.
- **Todo número ajustable sin código:**
  - Un set de armas `UEntropiaWeaponSet`, en el Data Asset `/Game/Entropia/Datos/DA_Armas`, que el editor crea solo si falta, como `DA_Animaciones`.
  - Cada arma lleva: piezas del modelo con su posición en la mano, cadencia, si es automática, cargador, recarga, daño al torso y a la cabeza, velocidad de la bala, dispersión y retroceso.
- **Las animaciones de pistola y fusil** vienen de la plantilla de Epic (`Characters/Mannequins/Anims/Pistol` y `Rifle`): apuntar, disparar y recargar. Se agregan como huecos nuevos en `DA_Animaciones`.
- Lógica pura (cargador, cadencia, dispersión, retroceso, daño por zona, balas que se quedan flotando en el mundo detenido, límite de balas) con **pruebas automáticas**. La integración se comprueba con **pasos del tour** normal y caos.

## 6. Decisiones de la IA (Hugo puede cambiarlas)
1. **Teclas:**
   - **Clic izquierdo apuntando = disparar.**
   - **Detener** se muda a **Q apuntando a algo**. Q sin nada en la mira hace lo de siempre: la burbuja en nivel 1 y el mundo detenido en nivel 2.
   - **R sin nada en la mira = recargar.** R apuntando a algo invierte, como siempre.
   - **Rueda del ratón = cambiar de arma.**
   - **8 = sala 8.**
2. **Revólver como arma insignia:** encaja con un viejo de 200 años y Logan. Es el modelo de Fab "Aurora's Blaze".
   - El fusil es el `fusil_6` del pack low poly. La pistola de los dummies es la `pistol_2` del mismo pack.
   - Las dos llevan un material oscuro propio para quitar el verde de juguete.
   - Se descartó la `Pistol_001` porque tiene más de un millón de triángulos.
3. **Velocidad de las balas:** revólver 8000 cm/s y fusil 9000 cm/s, lo bastante rápidas para sentirse reales y lo bastante lentas para que el tiempo las alcance. Las enemigas siguen a 1800.
4. **El arma en la mano:**
   - Va pegada al hueso `hand_r` con un ajuste que viene del set de armas.
   - Apuntando y quieto, el cuerpo entero toma la pose de apuntar de la plantilla. Si te mueves apuntando, vuelve la locomoción normal con el arma en la mano.
   - Lo mismo con disparar y recargar: son montajes de cuerpo entero. Separar la parte de arriba del cuerpo queda para el pulido (no-meta).
5. **Aguante de los maniquíes:** 100, y se llena al recuperarse. Cabeza = el impacto entra en los 30 cm de arriba de la cápsula.
6. **Lluvia detenida:**
   - La bala del jugador avanza en **tiempo real** los primeros 150 cm aunque el mundo esté detenido.
   - Después respeta el tiempo del mundo: se queda quieta, y la burbuja y Detener ya la tocan como a cualquier bala.
7. **Devuélvesela:**
   - Si una bala enemiga termina de rebobinarse en el cañón de un dummy que sigue en pie, lo tumba.
   - Si el dummy ya no tiene el arma, está tirado o ya no existe, la bala solo desaparece, como hoy.
8. **Recarga inversa:** solo cuentan las balas que disparaste con el arma que tienes ahora y que todavía están en la lista de vivas. Si ya cambiaste de arma, la bala vuelve al cargador de su arma.
9. **Último en caer:**
   - Cuenta los maniquíes del **mundo** (en las salas 7 y 8) que estén de pie y armados.
   - La cámara lenta la pide el árbitro del tiempo, con una fuente propia y la más lenta gana. Si ya hay un mundo detenido, no hace nada.
10. **Marca de impacto:** 0.25 s. Dispersión del revólver 0.3° quieto, y del fusil de 0.8° a 3°.
11. **Las balas del jugador no pegan al jugador** ni a sus ecos.
