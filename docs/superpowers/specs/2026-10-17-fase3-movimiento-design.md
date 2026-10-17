# Fase 3 · Moverse, reflejos y remates — Diseño

Fecha: 17 oct 2026 · Proyecto: ENTROPÍA (`entropy/`, UE 5.8 C++) · Usuario: Hugo (prueba en Play; no programa)

## 1. Intención
Hugo, sobre lo que hace sentir increíble al personaje: **"el dash y esquivar balas en cámara lenta, sorprender o detener al enemigo y asesinarlo, si te dispara poder ralentizar las balas… la Fase 3 es muy importante"**.
El parkour (subir paredes y bordes, pasar por encima, piruetas) es grande y **va después de estas 3 partes** (decisión de Hugo).

## 2. Metas (3 partes, en este orden; cada una se prueba con el tour antes de seguir)
**A · Moverse**
1. **Cámara sobre el hombro + mira:** la cámara se corre a la derecha del personaje. Con clic derecho mantenido = **apuntar**: la cámara se acerca, el personaje mira hacia donde apuntas y aparece un punto de mira en el centro (ámbar si hay un objeto que se puede invertir).
2. **Poderes con el ratón:** clic izquierdo mantenido = Invertir objeto (lo mismo que R, que sigue funcionando). Si hay un remate disponible, el clic izquierdo remata (parte C).
3. **Dash (Shift):** un impulso corto en la dirección en que te mueves (o hacia adelante si estás quieto), también en el aire (una vez por salto). Usa la animación `AM_Dash` de la plantilla, deja estela y una onda chica de distorsión. No gasta energía; tiene recarga (0.6 s).
4. "Rebobinar todo" pasa de Shift+R a **Ctrl+R**, porque Shift ahora es el dash.

**B · Reflejos**
5. **Vida simple** (hace falta para que esquivar importe): 100 de vida; una bala quita 20; se recupera sola tras 4 s sin daño; en 0 reapareces al inicio de la sala. Barra roja encima de la de energía.
6. **Las balas pueden pegarte:** las de las torretas que te apuntan te hacen daño y te empujan un poco.
7. **Sala 5 · Reflejos** (tecla 5): un cuarto con 3 torretas que te siguen y disparan cada ~1.2 s, y maniquíes de práctica para la parte C.
8. **Esquiva perfecta:** si haces dash cuando una bala te va a pegar en menos de 0.35 s, entras en **"tiempo de reflejo"**: el mundo va lento (0.3) durante 1.5 s reales, tú vas normal y hay una onda fuerte de distorsión. Durante el dash eres invulnerable 0.25 s. Es un premio al buen timing: no gasta energía.
9. **Ojo lento** (la pasiva de la rama Brecha que ya está en el diseño del juego): mientras apuntas, las balas que entran a 3.5 m de ti se frenan al 25 % (y se ven con distorsión). Gasta 6 de energía por segundo solo mientras está frenando alguna.

**C · Remates**
10. Un **remate** se puede hacer si un objetivo está a menos de 1.8 m y (a) estás a su espalda (> 120° de su frente) o (b) está congelado en una burbuja.
11. El HUD muestra "CLIC: REMATE"; con clic izquierdo: te alineas detrás del objetivo, cámara lenta breve (0.2 durante 0.4 s reales), golpe (`AM_ComboAttack`), onda de distorsión y el objetivo cae (daño grande + impulso a través de la interfaz `ICombatDamageable` de la plantilla).
12. Objetivo de práctica: `BP_CombatDummy` de la plantilla (los enemigos con IA son de la Fase 5).

## 3. No-metas
Parkour (después de estas 3 partes), enemigos con IA (Fase 5), armas del jugador (Fase 4), Enhanced Input (las teclas nuevas siguen con `BindKey`, como las demás), sonido.

## 4. Anti-metas
- Que el dash o los remates se sientan "pegados" o lentos (la animación no debe frenar el movimiento).
- Que la cámara sobre el hombro empeore apuntar a objetos chicos (la mira debe quedar justo donde se apunta).
- Romper lo que ya funciona: R/clic con grupos, Shift+R (ahora Ctrl+R), V/F/Q/E, burbuja, salas 1-4, el tour normal y el tour caos.
- Que el "tiempo de reflejo" choque con la Sobremarcha (V): no se acumulan dilataciones.

## 5. Restricciones
- C++ en `Source/entropy/` (plano), sin assets hechos a mano (se usan `AM_Dash`, `AM_ComboAttack` y `BP_CombatDummy` de la plantilla, que ya existen).
- Tiempo real con `TemporalFX::RealDelta()`; distorsión con `UTemporalFXSubsystem`.
- Nada de nombres genéricos en namespaces anónimos.
- Cada parte termina con su prueba en el tour (pasos nuevos) y el tour caos sin errores.

## 6. Decisiones de la IA (Hugo puede cambiarlas)
- Teclas: clic derecho = apuntar; clic izquierdo = invertir o rematar; Shift = dash; Ctrl+R = rebobinar todo.
- Dash: 1400 cm/s durante 0.18 s, recarga de 0.6 s, un dash en el aire por salto.
- Vida: 100; bala 20; recuperación tras 4 s.
- Ventana de esquiva perfecta 0.35 s; tiempo de reflejo 1.5 s a 0.3.
- Ojo lento: 3.5 m, balas al 25 %, 6/s de energía.
- Remate: 1.8 m, espalda > 120° o congelado.
Todo queda como `UPROPERTY`.

## 7. Cómo sabremos que está bien
- Pruebas automáticas para la lógica pura: dash (dirección y recarga), esquiva perfecta (la bala llega en < 0.35 s), condición de remate (espalda o congelado), vida (daño, recuperación, muerte).
- El tour normal tiene pasos nuevos (apuntar, dash, sala 5, esquiva perfecta forzada, ojo lento, remate) y sus capturas se ven bien; el tour caos sigue sin errores ni fugas.
- Hugo lo prueba en Play.
