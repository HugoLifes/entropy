# Fase 3 · Moverse, reflejos y remates — Diseño

Fecha: 17 oct 2026 · Proyecto: ENTROPÍA (`entropy/`, UE 5.8 C++) · Usuario: Hugo (prueba en Play; no programa)

## 1. Intención
Hugo, sobre lo que hace sentir increíble al personaje: **"el dash y esquivar balas en cámara lenta, sorprender o detener al enemigo y asesinarlo, si te dispara poder ralentizar las balas… la Fase 3 es muy importante"**.
Y sobre la mira: **"hay que tener una mira o una manera de seleccionar objetos que podamos detener como las balas, por eso es importante la mira también en esta fase"**.
El parkour (subir paredes y bordes, pasar por encima, piruetas) es grande y **va después de estas 3 partes** (decisión de Hugo).

## 2. Metas (3 partes, en este orden; cada una se prueba con el tour antes de seguir)
**A · Moverse y apuntar**
1. **Cámara sobre el hombro:** la cámara se corre a la derecha del personaje. Con **clic derecho mantenido = apuntar**: la cámara se acerca, caminas más lento y el personaje mira hacia donde apuntas.
2. **Mira (siempre visible):** un punto de mira en el centro de la pantalla que **selecciona** el objeto sobre el que actúan tus poderes: balas, cajas, pedazos, plataformas, maniquíes y torretas. Lo seleccionado se resalta (ámbar). La mira cambia de color: blanca (nada), ámbar (algo seleccionable), cian (eso ya está detenido). Para atrapar balas, el Ojo lento (punto 9) las frena mientras apuntas.
3. **Detener (clic izquierdo) — poder nuevo, pedido por Hugo:** lo que tienes en la mira se queda **congelado en el tiempo** 4 s, dentro de una burbujita propia. Cuesta 15 de energía; máximo 3 cosas detenidas a la vez (la cuarta suelta la más vieja). Otro clic sobre algo detenido lo suelta antes. Si lo inviertes con R, sale de su burbujita y regresa al pasado.
4. **Invertir (R mantenida)** sigue igual, pero apunta con la misma mira.
5. **Dash (Shift):** un impulso corto en la dirección en que te mueves (o hacia adelante si estás quieto), también en el aire (una vez por salto). Usa la animación `AM_Dash` de la plantilla, deja estela y una onda chica de distorsión. No gasta energía; tiene recarga (0.6 s).
6. "Rebobinar todo" pasa de Shift+R a **Ctrl+R**, porque Shift ahora es el dash.

**B · Reflejos**
7. **Vida simple** (hace falta para que esquivar importe): 100 de vida; una bala quita 20; se recupera sola tras 4 s sin daño; en 0 reapareces al inicio de la sala. Barra roja encima de la de energía. **Regresar en el tiempo (F) NO te devuelve la vida** (decisión de Hugo; quizá después).
8. **Las balas pueden pegarte:** las de las torretas que te apuntan te hacen daño y te empujan un poco.
9. **Sala 5 · Reflejos** (tecla 5): un cuarto con 3 torretas que te siguen y disparan cada ~1.2 s, columnas para cubrirte y maniquíes para la parte C.
10. **Esquiva perfecta:** si haces dash cuando una bala te va a pegar en menos de 0.35 s, entras en **"tiempo de reflejo"**: el mundo va lento (0.3) durante 1.5 s reales, tú vas normal y hay una onda fuerte de distorsión. Durante el dash eres invulnerable 0.25 s. Es un premio al buen timing: no gasta energía.
11. **Ojo lento** (la pasiva de la rama Brecha del diseño del juego): mientras apuntas, las balas que entran a 3.5 m de ti se frenan al 25 %. Gasta 6 de energía por segundo solo mientras está frenando alguna.

**C · Remates**
12. **Maniquí temporal** (decisión de Hugo: "no importa si es como un clon mío"): un clon gris del personaje (mismo cuerpo y animación). Está de pie; algunos miran a los lados. La burbuja y Detener lo congelan de verdad.
13. Un **remate** se puede hacer si un maniquí está a menos de 1.8 m y (a) estás a su espalda (> 120° de su frente) o (b) está congelado.
14. El HUD muestra "CLIC: REMATE"; con clic izquierdo: te alineas con el objetivo, cámara lenta breve (0.2 durante 0.4 s reales), golpe (`AM_ComboAttack`), onda de distorsión y el maniquí cae como muñeco de trapo. A los 4 s "el tiempo lo regresa": aparece de pie en su lugar.
15. **Si lo rematas congelado, el golpe se queda guardado**: sale volando cuando su tiempo regresa (se acaba la burbuja o lo sueltas con otro clic).

## 3. No-metas
Parkour (después de estas 3 partes), enemigos con IA (Fase 5), armas del jugador (Fase 4), Enhanced Input (las teclas nuevas siguen con `BindKey`, como las demás), sonido, retroceder personas (idea para después; el maniquí aún no tiene Registro), devolver la vida con F.

## 4. Anti-metas
- Que el dash o los remates se sientan "pegados" o lentos (la animación no debe frenar el movimiento).
- Que la cámara sobre el hombro empeore apuntar a objetos chicos (la mira debe quedar justo donde se apunta).
- Romper lo que ya funciona: R con grupos, Shift+R (ahora Ctrl+R), V/F/Q/E, burbuja, salas 1-4, el tour normal y el tour caos.
- Que los tiempos lentos choquen: Sobremarcha, tiempo de reflejo, remate, burbujas, Detener y Ojo lento **no se multiplican**; gana el más lento (ver §6).

## 5. Restricciones
- C++ en `Source/entropy/` (plano), sin assets hechos a mano (se usan `AM_Dash`, `AM_ComboAttack` y el cuerpo del jugador, que ya existen). Las variantes de la plantilla no se tocan.
- Tiempo real con `TemporalFX::RealDelta()`; distorsión con `UTemporalFXSubsystem`.
- Nada de nombres genéricos en namespaces anónimos.
- Cada parte termina con su prueba en el tour (pasos nuevos) y el tour caos sin errores.

## 6. Decisiones de la IA (Hugo puede cambiarlas)
- Teclas: clic derecho = apuntar; **clic izquierdo = Detener o rematar** (rematar gana si el HUD lo muestra); R = invertir; Shift = dash; Ctrl+R = rebobinar todo; 5 = sala 5.
- **Un solo "árbitro del tiempo"** (`UTemporalDilationSubsystem`): cada poder pide una velocidad para un actor (o para el mundo) y gana la más lenta. Así una bala congelada por una burbuja no se descongela cuando el Ojo lento la suelta. El jugador siempre va a velocidad normal en los tiempos lentos globales (Sobremarcha, reflejo, remate). La burbuja y la Sobremarcha pasan a usarlo.
- Detener reusa la burbuja (`ATimeBubble` con un solo objetivo y sin distorsión de pantalla): 4 s, 15 de energía, máx. 3.
- La mira selecciona lo que tiene Registro y además los maniquíes y las torretas que te siguen (se pueden detener, no invertir).
- Cámara: brazo 300 → 150 al apuntar, hombro a la derecha (55 cm), caminar a 320 al apuntar. No cambia el FOV (el Paso Inverso ya lo usa).
- Dash: 1400 cm/s durante 0.18 s, recarga de 0.6 s, un dash en el aire por salto; no se puede durante el Paso Inverso ni el remate. La animación no mueve al personaje (sin root motion).
- Vida: 100; bala 20; recuperación tras 4 s a 25/s; al morir, 1.5 s de protección.
- Torretas de la sala 5: balas a 1800 cm/s (más lentas que las de la sala 4 para que se puedan esquivar), solo disparan si estás dentro de la sala y te ven; la burbuja y Detener las congelan.
- Ventana de esquiva perfecta 0.35 s; tiempo de reflejo 1.5 s a 0.3.
- Ojo lento: 3.5 m, balas al 25 %, 6/s de energía.
- Remate: 1.8 m, espalda > 120° o congelado; el maniquí se levanta a los 4 s de su tiempo (congelado no cuenta).
- El maniquí ya no es `BP_CombatDummy` (es invencible, es un cilindro sin frente y la burbuja no lo congela).
Todo queda como `UPROPERTY`.

## 7. Cómo sabremos que está bien
- Pruebas automáticas para la lógica pura: árbitro del tiempo (gana el más lento, soltar devuelve el siguiente, compensación del jugador), dash (dirección y recarga), bala contra cápsula y tiempo para pegar (esquiva perfecta), condición de remate (espalda o congelado), vida (daño, recuperación, invulnerable, muerte una vez).
- El tour normal tiene pasos nuevos (apuntar, detener bala, dash, sala 5, esquiva perfecta forzada, ojo lento, remate por la espalda, remate congelado, morir y reaparecer) y sus capturas se ven bien; el tour caos suma cosas raras nuevas y sigue sin errores ni fugas.
- Hugo lo prueba en Play.
