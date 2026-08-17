#pragma once
// scanner.h
//
// Clase Scanner "preexistente" mínima. La aplicación genera EXCLUSIVAMENTE
// el cuerpo del método Scanner::nextToken(); el resto de la clase (miembros
// `input`, `current`, `first`, constructor) se asume ya definido en el
// proyecto destino, tal como pide el enunciado. Este archivo sirve de
// referencia/contexto y para poder compilar el código generado de forma
// independiente durante el desarrollo.

#include <string>
#include "token.h"

class Scanner {
public:
    explicit Scanner(std::string source) : input(std::move(source)), current(0), first(0) {}

    // Implementado por el código generado (ver generated_scanner_body.txt
    // o lo que se copie desde el QTextEdit de la aplicación).
    Token* nextToken();

private:
    std::string input;
    int current;
    int first;
};
