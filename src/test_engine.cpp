// Prueba standalone (sin Qt) del motor: valida parser -> NFA -> DFA -> codegen
#include "regexengine.h"
#include <iostream>

int main() {
    using namespace RE;

    std::vector<TokenDefinition> defs = {
        {"(a|b)*abb", "ID"},   // clásico ejemplo de libro (Aho)
        {"0|1(0|1)*", "NUM"},
    };

    try {
        ThompsonBuilder tb;
        NFA nfa = tb.combine(defs);
        std::cout << "NFA construido con " << nfa.states.size() << " estados.\n";

        SubsetConstruction sc;
        DFA dfa = sc.build(nfa);
        std::cout << "DFA construido con " << dfa.states.size() << " estados.\n";

        int acceptCount = 0;
        for (auto& s : dfa.states) if (s.accepting) acceptCount++;
        std::cout << "Estados de aceptacion: " << acceptCount << "\n\n";

        CodeGenerator gen;
        std::string code = gen.generate(dfa);
        std::cout << "----- CODIGO GENERADO -----\n" << code << "\n";

    } catch (const RegexParseException& e) {
        std::cerr << "Error de parseo: " << e.what() << "\n";
        return 1;
    }

    // Caso de error esperado: ER vacía
    try {
        Parser p("");
        p.parse();
        std::cerr << "FALLO: se esperaba excepcion por ER vacia\n";
        return 1;
    } catch (const RegexParseException&) {
        std::cout << "OK: ER vacia lanza excepcion como se esperaba.\n";
    }

    // Caso de error esperado: parentesis desbalanceado
    try {
        Parser p("(a|b");
        p.parse();
        std::cerr << "FALLO: se esperaba excepcion por parentesis sin cerrar\n";
        return 1;
    } catch (const RegexParseException&) {
        std::cout << "OK: parentesis sin cerrar lanza excepcion como se esperaba.\n";
    }

    std::cout << "\nTodas las pruebas pasaron.\n";
    return 0;
}
