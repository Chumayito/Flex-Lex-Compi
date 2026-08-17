#pragma once
// token.h
//
// Clase Token "preexistente" mínima, provista como contexto para que el
// código generado por la aplicación (Scanner::nextToken) tenga sentido y
// sea compilable de forma independiente. Amplía Token::Type con los tipos
// que el usuario vaya asignando en la interfaz (ID, NUM, etc.), además de
// ERR para lexemas no reconocidos.

#include <string>

class Token {
public:
    enum Type {
        ERR = 0,
        // A partir de aquí, tipos de ejemplo. La UI permite asociar
        // cualquier nombre de tipo a una expresión regular; agregue aquí
        // el enumerador correspondiente antes de compilar el Scanner
        // generado (p. ej. ID, NUM, KEYWORD, etc.).
        ID,
        NUM,
    };

    Type type;
    std::string lexeme;

    // Constructor para tokens delimitados por [first, current) dentro de `input`.
    Token(Type t, const std::string& input, int first, int current)
        : type(t), lexeme(input.substr(static_cast<size_t>(first),
                                        static_cast<size_t>(current - first))) {}

    // Constructor para tokens de error de un solo carácter.
    Token(Type t, char c) : type(t), lexeme(std::string(1, c)) {}
};
