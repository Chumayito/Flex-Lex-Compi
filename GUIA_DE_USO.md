# Guía de uso — Generador Visual de Analizadores Léxicos

## 1. Recorrido de la interfaz

| Zona | Función |
|---|---|
| **Expresión regular en construcción** (rectángulo blanco) | Muestra, de solo lectura, la ER que estás armando con los botones. |
| **Alfabeto y operadores** | Botones `a-z`, `0-9`, `\|`, `*`, `(`, `)`. Cada clic agrega ese carácter al final de la ER actual. |
| **⌫ Borrar último** | Quita el último carácter agregado a la ER actual (por si te equivocaste). |
| **Limpiar ER** | Vacía por completo la ER actual (sin tocar las definiciones ya guardadas). |
| **Asociar a Tipo de Token…** | Valida la ER actual y la guarda en la lista de definiciones, asociada a un `Token::TIPO`. |
| **Definiciones de tokens** | Lista acumulada de todas las reglas `ER → Token::TIPO` ya asociadas, en orden de prioridad. |
| **Quitar seleccionada / Quitar todas** | Edición de la lista de definiciones. |
| **⚙ Generar Scanner C++** | Corre el pipeline completo (AST → AFND → AFD → código) sobre *todas* las definiciones juntas. |
| **Código generado** | `QTextEdit` de solo lectura con el cuerpo de `Token* Scanner::nextToken()`. |

## 2. Flujo básico (un solo token)

Ejemplo: reconocer identificadores con la forma clásica `(a|b)*abb`.

1. Clic en `(` `a` `|` `b` `)` `*` `a` `b` `b` → el rectángulo blanco muestra `(a|b)*abb`.
2. Clic en **Asociar a Tipo de Token…** → aparece un diálogo. Escribí `ID` → Aceptar.
3. La ER actual se vacía automáticamente y aparece en la lista de definiciones:
   `(a|b)*abb   →   Token::ID`
4. Clic en **⚙ Generar Scanner C++** → el `QTextEdit` muestra el `switch` anidado que implementa `nextToken()`.

## 3. Caso de uso: varios tipos de token (lo típico de un lexer real)

Un scanner real casi nunca reconoce un solo patrón. Repetí el paso "armar ER → Asociar a Tipo de Token" una vez por cada categoría léxica que necesites, por ejemplo:

| ER (armada con botones) | Tipo asociado |
|---|---|
| `(a\|b)*abb` | `ID` |
| `0\|1(0\|1)*` | `NUM` |
| `i` `f` (sí, la palabra `if`, letra por letra) | `KEYWORD_IF` |

Al final tenés tres filas en "Definiciones de tokens". Cuando presionás **Generar Scanner C++**, el motor:

- Construye un AFND por Thompson para cada una de las tres ER.
- Las une con un nuevo estado inicial (transiciones épsilon).
- Aplica subconjuntos y obtiene **un único AFD** que reconoce las tres cosas en una sola pasada.
- Genera **un solo** `nextToken()` capaz de devolver `Token::ID`, `Token::NUM` o `Token::KEYWORD_IF` según lo que venga en la entrada.

> Limitación importante del alfabeto de botones: solo hay `a-z` (minúsculas) y `0-9`. Para simular `if`, `while`, etc. armá la ER letra por letra concatenando los botones correspondientes (`i`, luego `f`). No hay botón de mayúsculas ni de otros símbolos (`+`, `;`, espacio, etc.) — si tu lenguaje los necesita, quedan fuera del alcance actual de la UI (ver sección 6, "Limitaciones conocidas").

## 4. Caso de uso: prioridad entre patrones que se solapan

Si dos definiciones podrían aceptar el **mismo** lexema, gana la que se definió **primero** en la lista (igual que la regla de "primera coincidencia" de Lex/Flex). Ejemplo:

1. Asociá `i` `f` → `KEYWORD_IF` (primera definición).
2. Asociá `(a|b)*` → `ID` (segunda definición). Como `if` está formada por `a`/`b`... en este ejemplo no se solaparía porque `i` y `f` no son `a` ni `b`, pero si usaras un alfabeto donde `if` también calzara dentro del patrón de identificador, el resultado sería `Token::KEYWORD_IF` porque se declaró antes.

**Recomendación práctica:** definí siempre las palabras clave *antes* que el patrón general de identificador, tal como harías en un archivo `.l` de Flex.

## 5. Caso de uso: corregir errores sobre la marcha

- **Me equivoqué al tipear la ER:** usá `⌫ Borrar último` (uno a uno) o `Limpiar ER` (todo de una vez). Ninguno de los dos afecta las definiciones ya guardadas.
- **Quiero borrar una definición ya asociada:** hacé clic sobre la fila en la lista "Definiciones de tokens" para seleccionarla, luego `Quitar seleccionada`.
- **Quiero empezar de cero:** `Quitar todas` vacía la lista completa de definiciones.
- **Intento asociar una ER vacía:** la app te avisa con un mensaje y no hace nada — armá algo primero.
- **Intento asociar una ER mal formada** (p. ej. `(a|b` sin cerrar el paréntesis): el parser la detecta antes de guardarla y te muestra el error concreto (posición, motivo). No queda agregada a la lista hasta que la corrijas.
- **Pongo un nombre de tipo inválido** (con espacios, empezando con dígito, etc.): se rechaza con un mensaje, porque el nombre se usa tal cual como `Token::ESE_NOMBRE` en C++, así que tiene que ser un identificador válido.
- **Presiono "Generar Scanner C++" sin haber cargado ninguna definición:** te avisa que necesitás al menos una.

## 6. Qué hacer con el código generado

El `QTextEdit` muestra solo el **cuerpo del método** `Token* Scanner::nextToken()`. No es un programa completo — asume que ya existe en tu proyecto:

- Una clase `Scanner` con los miembros `std::string input`, `int current`, `int first`.
- Una clase `Token` con un constructor `Token(Type, const std::string& input, int first, int current)` para tokens normales y `Token(Type, char)` para errores.
- Un `enum Token::Type` que incluya `ERR` y cada uno de los nombres que le pusiste a tus tokens (`ID`, `NUM`, `KEYWORD_IF`, etc.).

En `token.h` y `scanner.h` del proyecto tenés un ejemplo mínimo de esas dos clases para probar el código generado de forma aislada.

## 7. Limitaciones conocidas (para que no te sorprendan)

- Alfabeto de entrada limitado a `a-z` y `0-9` — no hay mayúsculas, espacios, ni símbolos de puntuación en la cuadrícula de botones.
- No hay operador de rango tipo `[a-z]` ni clases de caracteres `\d`, `\w` — solo unión explícita carácter por carácter, concatenación y `*`.
- No hay `+` (una-o-más) ni `?` (cero-o-uno) como atajos; se pueden expresar con lo que sí hay (`a+` equivale a `aa*`, `a?` equivale a `(a|<vacío>)`, aunque la UI actual no tiene forma de introducir la cadena vacía como átomo suelto).
- No es una gramática libre de contexto: no se pueden definir reglas que se referencien entre sí (ver la explicación de la conversación anterior).
