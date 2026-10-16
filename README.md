# TotalLoot (RE_Kenshi plugin)

Plugin de RE_Kenshi (C++, MSVC x64) para Kenshi 1.0.65 que agrega botones **Loot**, **Loot all**, **Pick pocket** y **Auto loot** al menú contextual nativo.

## Características
- **Loot / Loot all**: Solo aparecen cuando el objetivo tiene ítems saqueables (contenedores/cajas con inventario, o personajes y animales derribados, inconscientes o muertos). No se muestran sobre aliados conscientes ni sobre objetivos vacíos. Loot transfiere los ítems del objetivo al personaje seleccionado; Loot all además recorre los cuerpos cercanos dentro de `LootRadius` (20 m por defecto, configurable). Si el inventario se llena, los sobrantes permanecen intactos en el objetivo. Con `CheatLoot=0` (por defecto) el personaje camina hasta cada cuerpo; ver [CheatLoot](#cheatloot).
- **Looteo con animales**: Loot / Loot all también funcionan con un animal del jugador seleccionado, y el botón **Auto loot** aparece al hacer clic derecho sobre tu propio animal vivo. Ver [Looteo con animales](#looteo-con-animales).
- **Pick pocket**: Con el personaje seleccionado en **sigilo**, el clic derecho sobre un NPC **consciente** muestra el botón **"Pick pocket"**. Un clic izquierdo, o mantener el clic derecho para abrir el menú, deslizarse sobre el botón y soltar (como las opciones nativas), hace que el personaje camine hasta el objetivo y abra la ventana de saqueo nativa con su inventario. Las probabilidades de robo, la detección, el crimen y el XP de Thievery son los del propio motor. No se activa el sigilo automáticamente.
  - Solo aparece si: el personaje es del jugador y está en sigilo, no está en combate ni en diálogo, y el objetivo es un NPC válido, consciente, de pie, no animal, no del jugador y no cargado.
  - La aproximación se cancela (con un mensaje breve) si se pierde el sigilo, el jugador da otra orden (se detecta enganchando `Character::addOrder`, no consultando la cola de órdenes), empieza un combate, el objetivo desaparece, pasan 60 s o no hay progreso durante 15 s.
  - Con la ventana abierta, se cierra sola si se pierde el sigilo, el objetivo sale del rango de comercio del motor (60 unidades, mismo piso y edificio), hay combate o el objetivo deja de ser válido.
- **Entrada y aspecto de los botones**: Loot, Loot all, Pick pocket y Auto loot comparten un único módulo de entrada (`context_button_input`) que escucha presionar *y* soltar el ratón, igual que TotalSparring: tanto el clic normal como el gesto nativo de soltar sobre el botón funcionan, y cada gesto actúa una sola vez. Tienen el mismo aspecto que los botones de TotalSparring (`button_style`): piel `Kenshi_Button1` de Kenshi, 160x44 px, separación de 2 px, texto ámbar de 18 pt con sombra y tinte latón (hover ámbar con texto oscuro). No se necesita ningún archivo de recursos adicional.
- **Registro**: Pick pocket escribe en el log de RE_Kenshi el inicio de sesión (en rango o no), cada orden de aproximación (limitado), la apertura/cierre de la ventana y el motivo de cada cancelación.
- **Posición sin solapamientos**: Los botones de TotalLoot forman un bloque que se coloca en el primer hueco libre junto al menú nativo, en este orden: izquierda alineado arriba, derecha alineado arriba, izquierda bajo los botones ajenos, derecha bajo los botones ajenos y, como último recurso, izquierda ajustado a la pantalla (el bloque incluye Auto loot cuando corresponde). Se evitan los botones visibles de otros mods (TotalSparring, TotalRecruit, TotalPredator, etc.). Se mantiene el lado elegido mientras siga libre para evitar parpadeos, y se respetan los bordes izquierdo y derecho de la pantalla.
- **Seguridad**: Todo acceso a punteros del motor y de MyGUI está protegido con SEH (`TL_TRY` / `TL_CATCH`).

## Configuración
`TotalLoot.ini` vive junto a `TotalLoot.dll`, en la carpeta del mod (`[Kenshi_Dir]\mods\TotalLoot\`). Se lee una sola vez al iniciar el juego: **reinicia Kenshi tras editarlo**. Las secciones y claves no distinguen mayúsculas; las líneas que empiezan por `;` o `#` son comentarios. Si el archivo falta, o una clave es desconocida o tiene un valor inválido, se usa el valor por defecto (el radio y la distancia de llegada deben ser mayores que cero; los valores no positivos vuelven al defecto).

`package_deploy.py` **nunca sobrescribe** un `TotalLoot.ini` ya existente en la carpeta del juego: tus ediciones sobreviven a cada despliegue. Solo copia la plantilla si el archivo no existe. Para recuperar la plantilla por defecto, borra tu INI y vuelve a desplegar.

| Sección | Clave | Defecto | Significado |
|---|---|---|---|
| `[General]` | `CheatLoot` | `0` | `0` = el personaje camina hasta cada cuerpo; `1` = looteo remoto instantáneo. Ver [CheatLoot](#cheatloot). |
| `[General]` | `LootRadius` | `20` | Radio de búsqueda (m) de Loot all y Auto loot. Máximo 500; valores mayores se recortan. |
| `[General]` | `ArriveDistance` | `5` | Distancia (m) a la que el looteador cuenta como llegado a un cuerpo. Máximo 100. |
| `[Looting]` | `clear_stolen_flag` | `0` | `1` = quita la marca de robado a los ítems saqueados para poder venderlos libremente. |
| `[Looting]` | `cancel_on_combat` | `1` | `1` = el looteo caminando se detiene si el looteador entra en combate. |
| `[Looting]` | `cancel_when_nearby_ally_attacked` | `1` | `1` = se detiene si atacan a un aliado cercano. |

Los valores booleanos aceptan `1/0`, `true/false`, `yes/no` y `on/off`.

## CheatLoot
- **`CheatLoot=0` (por defecto)**: el personaje camina hasta cada cuerpo y lo saquea al llegar (a `ArriveDistance`).
  - **Loot**: va al objetivo y lo saquea.
  - **Loot all**: primero el cuerpo sobre el que hiciste clic y luego el resto de los cuerpos saqueables dentro de `LootRadius`, del más cercano al más lejano. Los cuerpos de personajes del jugador se omiten.
  - **Se detiene** cuando: das cualquier orden al personaje (la cancelación se detecta enganchando `Character::addOrder`), empieza un combate (`cancel_on_combat`), atacan a un aliado cercano (`cancel_when_nearby_ally_attacked`), el inventario se llena (tras 3 cuerpos seguidos sin ningún ítem transferido), o se agotan los tiempos: 60 s por cuerpo y 15 s sin progreso. La orden de movimiento se reemite cada 2,5 s.
  - Los contenedores también se saquean caminando. Los contenedores de la facción del jugador no son saqueables.
- **`CheatLoot=1`**: Loot y Loot all son instantáneos y remotos, como en versiones anteriores; Loot all usa `LootRadius` en lugar de los 20 m fijos de antes.

## Looteo con animales
Loot y Loot all funcionan con un animal del jugador seleccionado como looteador. Además, al hacer clic derecho sobre **tu propio animal vivo** aparece el botón **Auto loot**:
- Al pulsarlo, el animal recorre los cuerpos saqueables dentro de `LootRadius` alrededor de sí mismo y se detiene cuando su mochila está llena, en combate o si atacan a un aliado cercano.
- El botón cambia a **Stop auto loot** mientras está activo, y detiene el proceso.
- Siempre camina, sin importar `CheatLoot`.
- No aparece sobre animales ajenos ni sobre animales muertos o derribados.
- Esta función se movió desde TotalPredator, que ahora solo se ocupa de alimentar. Con ambos mods instalados no hay botones LOOT / AUTO LOOT duplicados.

## Transferencia de ítems
- **Mochila primero**: los ítems van a la mochila del looteador y, si no caben, al inventario principal; lo que no cabe vuelve al objetivo sin bloquear al resto.
- **Apilado**: los ítems se combinan con pilas existentes cuando es posible.
- **Dinero**: el dinero se acredita directamente al jugador.
- **Marca de robado**: por defecto se respeta el comportamiento nativo (saquear a un vivo es robo; a un muerto, no). Con `clear_stolen_flag=1` se limpia la marca.

## Pruebas
La lógica pura (colocación de botones, reglas y máquina de estados de Pick pocket, parser de `TotalLoot.ini`, conjunto de órdenes vigiladas, máquina de estados de la cola de looteo caminando, reglas del despachador y reglas del botón Auto loot) tiene pruebas unitarias sin dependencias de Kenshi:
```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' tests\totalloot_tests.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo
.\tests\x64\Release\totalloot_tests.exe
```
Las verificaciones dentro del juego están en [`docs/QA-CHECKLIST.md`](docs/QA-CHECKLIST.md).

## Compilación
Requisitos:
- Visual Studio 2019 / 2022 con herramientas C++ v100/v140 o MSBuild.
- KenshiLib y dependencias de Ogre/MyGUI/Boost.

Comando:
```powershell
$env:KENSHILIB_DIR='F:\Proyectos\Kenshi-mods\KenshiLib_Examples_deps\KenshiLib'
$env:BOOST_INCLUDE_PATH='F:\Proyectos\Kenshi-mods\KenshiLib_Examples_deps\boost_1_60_0'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' TotalLoot.vcxproj -p:Configuration=Release -p:Platform=x64 -v:minimal -nologo
```

## Instalación
Con `package_deploy.py` (tras compilar, ver arriba): `python package_deploy.py` empaqueta `ModPackage/TotalLoot` e instala en `D:\SteamLibrary\steamapps\common\Kenshi\mods\TotalLoot` (ajusta la ruta en el script si tu Kenshi está en otro lugar). Copia siempre la DLL y deja intacto un `TotalLoot.ini` existente.

Manualmente, copiar la carpeta `ModPackage/TotalLoot` a la carpeta `mods` de Kenshi:
`[Kenshi_Dir]\mods\TotalLoot\` conteniendo:
- `TotalLoot.dll`
- `RE_Kenshi.json`
- `TotalLoot.ini` (configuración, ver [Configuración](#configuración))
