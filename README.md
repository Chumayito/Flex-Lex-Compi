# Generador Visual de Analizadores Léxicos (Regex → AFND → AFD → C++)

Aplicación de escritorio en C++/Qt Widgets que permite construir expresiones
regulares únicamente con botones, asociarlas a tipos de token, y generar el
código C++ del método `Token* Scanner::nextToken()` correspondiente al
autómata combinado de todas las definiciones.

## Estructura del proyecto

```
qtscanner/
├── CMakeLists.txt
└── src/
    ├── main.cpp            # Punto de entrada (QApplication)
    ├── mainwindow.h/.cpp   # UI: botones, QLabel, lista de definiciones, QTextEdit
    ├── regexengine.h/.cpp  # Backend: Parser -> AST -> NFA (Thompson) -> DFA (subconjuntos) -> codegen
    ├── token.h             # Clase Token de ejemplo (contexto para el código generado)
    ├── scanner.h           # Clase Scanner de ejemplo (contexto para el código generado)
    └── test_engine.cpp     # Prueba standalone del motor SIN Qt (útil para depurar el backend)
```

`regexengine.h/.cpp` no depende de Qt en absoluto: es C++ estándar puro, lo
que permite probarlo y reutilizarlo de forma aislada (ver `test_engine.cpp`).

## Compilación

Requiere Qt 5 (≥5.12) o Qt 6, con el módulo `Widgets` y sus headers de
desarrollo (`qtbase5-dev` / `qt6-base-dev` según la distro), además de CMake.

```bash
mkdir build && cd build
cmake ..
cmake --build . -j
./RegexScannerGenerator
```

Para compilar y correr solo la prueba del motor (sin Qt, útil para verificar
el backend de forma independiente):

```bash
cmake --build . --target test_engine
./test_engine
```

> Nota de desarrollo: en el entorno donde se escribió este código no había
> headers de desarrollo de Qt disponibles, así que `regexengine.h/.cpp` se
> compiló y probó exhaustivamente con `g++` puro (parser, Thompson,
> subconjuntos y el código generado se ejecutaron end-to-end contra varios
> casos, incluyendo el ejemplo clásico `(a|b)*abb` del libro de Aho). La
> capa Qt (`mainwindow.*`, `main.cpp`) se escribió revisando cuidadosamente
> cada firma de API contra Qt5/Qt6, pero no pudo compilarse en este entorno
> por falta de los paquetes `-dev`; revísala al compilar por primera vez.

## Uso

1. Arme una expresión regular presionando los botones de letras (`a-z`),
   dígitos (`0-9`) y operadores (`|`, `*`, `(`, `)`). El resultado se ve en
   el `QLabel` de solo lectura de arriba. `⌫ Borrar último` quita el último
   carácter; `Limpiar ER` la vacía por completo.
2. Presione **"Asociar a Tipo de Token…"**. La ER se valida sintácticamente
   (paréntesis balanceados, no vacía, etc.) y se le pide un nombre de tipo
   (p. ej. `ID`, `NUM`). Queda agregada a la lista de definiciones, en orden
   de prioridad (la primera definida gana los empates entre patrones que
   acepten el mismo lexema — igual que en Lex/Flex).
3. Repita el proceso para todos los tokens del lenguaje que quiera reconocer.
4. Presione **"⚙ Generar Scanner C++"**. El backend:
   - Convierte cada ER a un AST respetando la precedencia `|` < concat < `*`.
   - Construye un AFND combinado (una rama de Thompson por cada patrón,
     unidas por un nuevo estado inicial con transiciones épsilon).
   - Aplica construcción de subconjuntos para obtener el AFD.
   - Emite el cuerpo de `Token* Scanner::nextToken()` en el `QTextEdit`,
     como un `switch(estado)` anidado con `switch(input[current])`, sin
     tablas de transición.
5. Copie el código generado dentro de la implementación real de la clase
   `Scanner` de su proyecto (que debe declarar los miembros `input`
   (`std::string`), `current` (`int`) y `first` (`int`), y un `Token::Type`
   con los nombres que haya usado como tipos de token, más `Token::ERR`).
   `token.h` / `scanner.h` en este repo muestran un ejemplo mínimo de esas
   clases preexistentes.

## Decisiones de diseño relevantes

- **Prioridad de patrones**: si dos definiciones aceptan el mismo lexema en
  un estado del AFD, gana la definida primero (orden de la lista), igual
  que el criterio estándar "primera regla" de los generadores de lexers.
- **Fin de cadena dentro del autómata**: si `current` llega al final de
  `input` mientras el autómata está en un estado no final, se retorna un
  `Token::ERR` sin reintentar, evitando bucles infinitos; si el estado es
  de aceptación, se retorna el token normalmente (esto es lo que permite
  reconocer, por ejemplo, un identificador que ocupa el resto de la
  cadena).
- **Carácter no reconocido**: en cualquier estado, si el carácter actual no
  tiene transición definida, se retorna `new Token(Token::ERR,
  input[current]);` y se incrementa `current` (tal como pide el enunciado),
  salvo que el estado ya sea de aceptación, en cuyo caso se prioriza cerrar
  el lexema válido (maximal munch simple, sin backtracking).
- **Espacios en blanco**: se saltan automáticamente entre lexemas antes de
  fijar `first`, como en cualquier scanner típico.
