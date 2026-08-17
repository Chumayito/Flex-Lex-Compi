#pragma once
// regexengine.h
//
// Motor de expresiones regulares para el generador visual de scanners.
// Pipeline: Regex (string) -> AST -> NFA (Thompson) -> DFA (subconjuntos) -> código C++.
//
// Gramática de expresiones regulares soportada (precedencia de menor a mayor):
//
//   union   := concat ('|' concat)*
//   concat  := star*
//   star    := atom '*'*
//   atom    := [a-zA-Z0-9] | '(' union ')'
//
// '|' tiene la menor precedencia, luego la concatenación (implícita), y '*' la mayor.

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace RE {

// ============================================================================
// 1) AST
// ============================================================================

enum class NodeType { CHAR, CONCAT, UNION, STAR, EMPTY };

struct ASTNode {
    NodeType type;
    char ch = 0;                    // válido solo si type == CHAR
    std::shared_ptr<ASTNode> left;  // hijo izquierdo (o único, en STAR)
    std::shared_ptr<ASTNode> right; // hijo derecho (CONCAT / UNION)

    explicit ASTNode(NodeType t) : type(t) {}
};
using ASTNodePtr = std::shared_ptr<ASTNode>;

class RegexParseException : public std::runtime_error {
public:
    explicit RegexParseException(const std::string& msg) : std::runtime_error(msg) {}
};

// Parser recursivo descendente para la gramática de arriba.
class Parser {
public:
    explicit Parser(const std::string& pattern);
    ASTNodePtr parse(); // lanza RegexParseException si la ER es inválida o está vacía

private:
    std::string src_;
    size_t pos_ = 0;

    bool atEnd() const;
    char peek() const;
    char advance();
    bool isAtomStart(char c) const;

    ASTNodePtr parseUnion();
    ASTNodePtr parseConcat();
    ASTNodePtr parseStar();
    ASTNodePtr parseAtom();
};

// ============================================================================
// 2) NFA (Construcción de Thompson)
// ============================================================================

struct NFAState {
    int id = -1;
    std::map<char, std::vector<int>> trans; // símbolo -> estados destino
    std::vector<int> epsilon;                // transiciones epsilon
    bool accept = false;                     // ¿es estado final de ALGÚN patrón?
    std::string tokenType;                   // tipo de token asociado (si accept == true)
    int priority = -1;                       // orden de definición; menor = mayor prioridad
};

struct NFA {
    std::vector<NFAState> states;
    int start = -1;

    int newState() {
        NFAState s;
        s.id = static_cast<int>(states.size());
        states.push_back(s);
        return s.id;
    }
};

struct Fragment {
    int start;
    int accept;
};

// Par (expresión regular ya parseada, tipo de token) en orden de prioridad
// (el primero definido gana en caso de empate entre varios patrones aceptando
// el mismo lexema).
struct TokenDefinition {
    std::string regex;
    std::string tokenType;
};

class ThompsonBuilder {
public:
    // Construye el fragmento (sin marcar aceptación) correspondiente a un AST.
    Fragment buildFragment(NFA& nfa, const ASTNodePtr& node);

    // Combina varias definiciones (regex, tipoDeToken) en un único NFA:
    // crea un nuevo estado inicial con transiciones epsilon hacia el inicio
    // de cada patrón, y marca el estado de aceptación de cada patrón con su
    // tipo de token y su prioridad (índice en el vector `definitions`).
    NFA combine(const std::vector<TokenDefinition>& definitions);
};

// ============================================================================
// 3) DFA (Construcción de subconjuntos)
// ============================================================================

struct DFAState {
    int id = -1;
    std::set<int> nfaStates;
    std::map<char, int> trans; // símbolo -> id de estado DFA destino
    bool accepting = false;
    std::string tokenType;     // válido solo si accepting == true
};

struct DFA {
    std::vector<DFAState> states;
    int start = 0;
    std::set<char> alphabet;
};

class SubsetConstruction {
public:
    DFA build(const NFA& nfa);

private:
    std::set<int> epsilonClosure(const NFA& nfa, const std::set<int>& states) const;
    std::set<int> move(const NFA& nfa, const std::set<int>& states, char symbol) const;
};

// ============================================================================
// 4) Generador de código C++ (Scanner::nextToken)
// ============================================================================

class CodeGenerator {
public:
    // Genera el cuerpo completo de Token* Scanner::nextToken() a partir del DFA.
    // nombreVariables asumidos preexistentes en la clase Scanner: `input`
    // (std::string), `current` (int) y `first` (int).
    std::string generate(const DFA& dfa);
};

} // namespace RE
