#include "regexengine.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <queue>
#include <sstream>

namespace RE {

// ============================================================================
// Parser
// ============================================================================

Parser::Parser(const std::string& pattern) : src_(pattern), pos_(0) {}

bool Parser::atEnd() const { return pos_ >= src_.size(); }

char Parser::peek() const { return atEnd() ? '\0' : src_[pos_]; }

char Parser::advance() {
    if (atEnd()) {
        throw RegexParseException("Fin inesperado de la expresión regular.");
    }
    return src_[pos_++];
}

bool Parser::isAtomStart(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) != 0;
}

ASTNodePtr Parser::parse() {
    if (src_.empty()) {
        throw RegexParseException("La expresión regular está vacía.");
    }
    ASTNodePtr result = parseUnion();
    if (!atEnd()) {
        throw RegexParseException(
            std::string("Carácter inesperado en la posición ") + std::to_string(pos_) +
            ": '" + peek() + "' (¿paréntesis sin cerrar/abrir?).");
    }
    return result;
}

// union := concat ('|' concat)*
ASTNodePtr Parser::parseUnion() {
    ASTNodePtr node = parseConcat();
    while (!atEnd() && peek() == '|') {
        advance(); // consume '|'
        ASTNodePtr rhs = parseConcat();
        auto unionNode = std::make_shared<ASTNode>(NodeType::UNION);
        unionNode->left = node;
        unionNode->right = rhs;
        node = unionNode;
    }
    return node;
}

// concat := star*
ASTNodePtr Parser::parseConcat() {
    ASTNodePtr node = nullptr;
    while (!atEnd() && peek() != '|' && peek() != ')') {
        ASTNodePtr term = parseStar();
        if (!node) {
            node = term;
        } else {
            auto concatNode = std::make_shared<ASTNode>(NodeType::CONCAT);
            concatNode->left = node;
            concatNode->right = term;
            node = concatNode;
        }
    }
    if (!node) {
        // Concatenación vacía (p.ej. "()" o "a||b"): representa la cadena vacía.
        node = std::make_shared<ASTNode>(NodeType::EMPTY);
    }
    return node;
}

// star := atom '*'*
ASTNodePtr Parser::parseStar() {
    ASTNodePtr node = parseAtom();
    while (!atEnd() && peek() == '*') {
        advance(); // consume '*'
        auto starNode = std::make_shared<ASTNode>(NodeType::STAR);
        starNode->left = node;
        node = starNode;
    }
    return node;
}

// atom := [a-zA-Z0-9] | '(' union ')'
ASTNodePtr Parser::parseAtom() {
    if (atEnd()) {
        throw RegexParseException("Se esperaba un átomo (carácter o '(') y se llegó al final.");
    }
    char c = peek();
    if (c == '(') {
        advance(); // consume '('
        ASTNodePtr inner = parseUnion();
        if (atEnd() || peek() != ')') {
            throw RegexParseException("Falta paréntesis de cierre ')'.");
        }
        advance(); // consume ')'
        return inner;
    }
    if (isAtomStart(c)) {
        advance();
        auto charNode = std::make_shared<ASTNode>(NodeType::CHAR);
        charNode->ch = c;
        return charNode;
    }
    throw RegexParseException(std::string("Carácter no soportado en la expresión regular: '") + c + "'.");
}

// ============================================================================
// Thompson construction
// ============================================================================

Fragment ThompsonBuilder::buildFragment(NFA& nfa, const ASTNodePtr& node) {
    switch (node->type) {
        case NodeType::CHAR: {
            int s1 = nfa.newState();
            int s2 = nfa.newState();
            nfa.states[s1].trans[node->ch].push_back(s2);
            return {s1, s2};
        }
        case NodeType::EMPTY: {
            int s1 = nfa.newState();
            int s2 = nfa.newState();
            nfa.states[s1].epsilon.push_back(s2);
            return {s1, s2};
        }
        case NodeType::CONCAT: {
            Fragment f1 = buildFragment(nfa, node->left);
            Fragment f2 = buildFragment(nfa, node->right);
            nfa.states[f1.accept].epsilon.push_back(f2.start);
            return {f1.start, f2.accept};
        }
        case NodeType::UNION: {
            int s1 = nfa.newState();
            int s2 = nfa.newState();
            Fragment f1 = buildFragment(nfa, node->left);
            Fragment f2 = buildFragment(nfa, node->right);
            nfa.states[s1].epsilon.push_back(f1.start);
            nfa.states[s1].epsilon.push_back(f2.start);
            nfa.states[f1.accept].epsilon.push_back(s2);
            nfa.states[f2.accept].epsilon.push_back(s2);
            return {s1, s2};
        }
        case NodeType::STAR: {
            int s1 = nfa.newState();
            int s2 = nfa.newState();
            Fragment f = buildFragment(nfa, node->left);
            nfa.states[s1].epsilon.push_back(f.start);
            nfa.states[s1].epsilon.push_back(s2);
            nfa.states[f.accept].epsilon.push_back(f.start);
            nfa.states[f.accept].epsilon.push_back(s2);
            return {s1, s2};
        }
    }
    throw std::logic_error("Tipo de nodo AST desconocido.");
}

NFA ThompsonBuilder::combine(const std::vector<TokenDefinition>& definitions) {
    NFA nfa;
    int newStart = nfa.newState();
    nfa.start = newStart;

    for (size_t i = 0; i < definitions.size(); ++i) {
        Parser parser(definitions[i].regex);
        ASTNodePtr ast = parser.parse();
        Fragment frag = buildFragment(nfa, ast);
        nfa.states[newStart].epsilon.push_back(frag.start);
        nfa.states[frag.accept].accept = true;
        nfa.states[frag.accept].tokenType = definitions[i].tokenType;
        nfa.states[frag.accept].priority = static_cast<int>(i);
    }
    return nfa;
}

// ============================================================================
// Subset construction
// ============================================================================

std::set<int> SubsetConstruction::epsilonClosure(const NFA& nfa, const std::set<int>& states) const {
    std::set<int> closure = states;
    std::queue<int> pending;
    for (int s : states) pending.push(s);

    while (!pending.empty()) {
        int s = pending.front();
        pending.pop();
        for (int t : nfa.states[s].epsilon) {
            if (closure.insert(t).second) {
                pending.push(t);
            }
        }
    }
    return closure;
}

std::set<int> SubsetConstruction::move(const NFA& nfa, const std::set<int>& states, char symbol) const {
    std::set<int> result;
    for (int s : states) {
        auto it = nfa.states[s].trans.find(symbol);
        if (it != nfa.states[s].trans.end()) {
            for (int t : it->second) result.insert(t);
        }
    }
    return result;
}

DFA SubsetConstruction::build(const NFA& nfa) {
    DFA dfa;

    // Alfabeto: todos los símbolos que aparecen en alguna transición del NFA.
    for (const auto& st : nfa.states) {
        for (const auto& kv : st.trans) dfa.alphabet.insert(kv.first);
    }

    auto bestAccept = [&](const std::set<int>& nfaSet) -> std::pair<bool, std::string> {
        bool accepting = false;
        std::string tokenType;
        int bestPriority = INT_MAX;
        for (int s : nfaSet) {
            const NFAState& st = nfa.states[s];
            if (st.accept && st.priority < bestPriority) {
                bestPriority = st.priority;
                tokenType = st.tokenType;
                accepting = true;
            }
        }
        return {accepting, tokenType};
    };

    std::map<std::set<int>, int> setToId;
    std::set<int> startClosure = epsilonClosure(nfa, {nfa.start});

    DFAState startState;
    startState.id = 0;
    startState.nfaStates = startClosure;
    auto [accepting0, type0] = bestAccept(startClosure);
    startState.accepting = accepting0;
    startState.tokenType = type0;
    dfa.states.push_back(startState);
    setToId[startClosure] = 0;
    dfa.start = 0;

    std::queue<int> worklist;
    worklist.push(0);

    while (!worklist.empty()) {
        int currentId = worklist.front();
        worklist.pop();
        // Copia porque dfa.states puede reasignarse al añadir estados nuevos.
        std::set<int> currentSet = dfa.states[currentId].nfaStates;

        for (char symbol : dfa.alphabet) {
            std::set<int> moved = move(nfa, currentSet, symbol);
            if (moved.empty()) continue;
            std::set<int> closure = epsilonClosure(nfa, moved);

            int targetId;
            auto it = setToId.find(closure);
            if (it != setToId.end()) {
                targetId = it->second;
            } else {
                DFAState newState;
                newState.id = static_cast<int>(dfa.states.size());
                newState.nfaStates = closure;
                auto [acc, type] = bestAccept(closure);
                newState.accepting = acc;
                newState.tokenType = type;
                targetId = newState.id;
                dfa.states.push_back(newState);
                setToId[closure] = targetId;
                worklist.push(targetId);
            }
            dfa.states[currentId].trans[symbol] = targetId;
        }
    }

    return dfa;
}

// ============================================================================
// Code generation
// ============================================================================

namespace {

std::string escapeChar(char c) {
    // Genera un literal de carácter C++ válido para el switch(input[current]).
    switch (c) {
        case '\'': return "'\\''";
        case '\\': return "'\\\\'";
        default: {
            std::ostringstream oss;
            oss << "'" << c << "'";
            return oss.str();
        }
    }
}

std::string indent(int level) { return std::string(static_cast<size_t>(level) * 4, ' '); }

} // namespace

std::string CodeGenerator::generate(const DFA& dfa) {
    std::ostringstream out;

    out << "Token* Scanner::nextToken() {\n";
    out << indent(1) << "while (true) {\n";
    out << indent(2) << "// Omitir espacios en blanco entre lexemas\n";
    out << indent(2) << "while (current < (int)input.length() && std::isspace((unsigned char)input[current])) {\n";
    out << indent(3) << "current++;\n";
    out << indent(2) << "}\n";
    out << indent(2) << "if (current >= (int)input.length()) {\n";
    out << indent(3) << "return nullptr; // fin de la entrada\n";
    out << indent(2) << "}\n\n";
    out << indent(2) << "first = current;\n";
    out << indent(2) << "int state = " << dfa.start << ";\n\n";
    out << indent(2) << "while (true) {\n";
    out << indent(3) << "switch (state) {\n";

    for (const auto& st : dfa.states) {
        out << indent(3) << "case " << st.id << ": {\n";
        out << indent(4) << "if (current >= (int)input.length()) {\n";
        if (st.accepting) {
            out << indent(5) << "return new Token(Token::" << st.tokenType
                << ", input, first, current);\n";
        } else {
            out << indent(5) << "return new Token(Token::ERR, input[first]);\n";
        }
        out << indent(4) << "}\n";
        out << indent(4) << "switch (input[current]) {\n";

        for (const auto& tr : st.trans) {
            out << indent(5) << "case " << escapeChar(tr.first) << ":\n";
            out << indent(6) << "current++;\n";
            out << indent(6) << "state = " << tr.second << ";\n";
            out << indent(6) << "break;\n";
        }

        out << indent(5) << "default:\n";
        if (st.accepting) {
            out << indent(6) << "return new Token(Token::" << st.tokenType
                << ", input, first, current);\n";
        } else {
            out << indent(6) << "return new Token(Token::ERR, input[current++]);\n";
        }
        out << indent(4) << "}\n";
        out << indent(4) << "break;\n";
        out << indent(3) << "}\n";
    }

    out << indent(3) << "default:\n";
    out << indent(4) << "return new Token(Token::ERR, input[current++]);\n";
    out << indent(3) << "}\n";
    out << indent(2) << "}\n";
    out << indent(1) << "}\n";
    out << "}\n";

    return out.str();
}

} // namespace RE
