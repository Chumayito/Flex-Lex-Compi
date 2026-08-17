#include "mainwindow.h"

#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// Estilo compartido para los botones de caracteres (a-z, 0-9).
const char* kCharButtonStyle =
    "QPushButton { font-family: 'Consolas', 'Courier New', monospace; "
    "font-size: 13pt; min-width: 34px; min-height: 34px; }";

// Estilo para los botones de operadores, resaltados para distinguirlos.
const char* kOperatorButtonStyle =
    "QPushButton { font-family: 'Consolas', 'Courier New', monospace; "
    "font-size: 14pt; font-weight: bold; min-width: 34px; min-height: 34px; "
    "background-color: #dfe8ff; }";

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    refreshRegexLabel();
    refreshDefinitionsList();
}

void MainWindow::buildUi() {
    setWindowTitle("Generador Visual de Analizadores Léxicos (Regex -> AFND -> AFD -> C++)");

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);

    // --- 1) Visualización de la expresión regular en construcción ---
    auto* regexTitle = new QLabel("Expresión regular en construcción:", central);
    mainLayout->addWidget(regexTitle);

    regexLabel_ = new QLabel(central);
    regexLabel_->setFrameShape(QFrame::Panel);
    regexLabel_->setFrameShadow(QFrame::Sunken);
    regexLabel_->setMinimumHeight(40);
    regexLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    regexLabel_->setStyleSheet(
        "QLabel { font-family: 'Consolas', 'Courier New', monospace; font-size: 14pt; "
        "background-color: white; color: black; padding-left: 8px; }");
    // Refuerzo con QPalette: en algunos temas oscuros de Linux (qt5ct/qt6ct,
    // GTK, etc.) el widget puede heredar un QPalette con texto claro que
    // compite con el stylesheet. Fijar la paleta explícitamente garantiza
    // texto negro sobre fondo blanco sin importar el tema del sistema.
    QPalette regexPalette = regexLabel_->palette();
    regexPalette.setColor(QPalette::Window, Qt::white);
    regexPalette.setColor(QPalette::WindowText, Qt::black);
    regexLabel_->setPalette(regexPalette);
    regexLabel_->setAutoFillBackground(true); // asegura que el fondo blanco se pinte siempre
    regexLabel_->setTextInteractionFlags(Qt::NoTextInteraction); // solo lectura, no editable
    mainLayout->addWidget(regexLabel_);

    // --- 2) Cuadrícula de botones para construir la ER ---
    auto* gridBox = new QGroupBox("Alfabeto y operadores (entrada exclusivamente por botones)", central);
    auto* gridBoxLayout = new QVBoxLayout(gridBox);
    gridBoxLayout->addLayout(buildRegexButtonGrid());
    mainLayout->addWidget(gridBox);

    // --- 3) Acciones sobre la ER actual ---
    auto* regexActionsLayout = new QHBoxLayout();
    auto* backspaceBtn = new QPushButton("⌫ Borrar último", central);
    auto* clearRegexBtn = new QPushButton("Limpiar ER", central);
    auto* assignBtn = new QPushButton("Asociar a Tipo de Token…", central);
    assignBtn->setStyleSheet("QPushButton { font-weight: bold; }");
    connect(backspaceBtn, &QPushButton::clicked, this, &MainWindow::onBackspaceClicked);
    connect(clearRegexBtn, &QPushButton::clicked, this, &MainWindow::onClearRegexClicked);
    connect(assignBtn, &QPushButton::clicked, this, &MainWindow::onAssignTokenType);
    regexActionsLayout->addWidget(backspaceBtn);
    regexActionsLayout->addWidget(clearRegexBtn);
    regexActionsLayout->addStretch();
    regexActionsLayout->addWidget(assignBtn);
    mainLayout->addLayout(regexActionsLayout);

    // --- 4) Lista de definiciones (ER -> Token::TIPO) ya asociadas ---
    auto* defsBox = new QGroupBox("Definiciones de tokens (orden = prioridad, la primera gana empates)", central);
    auto* defsLayout = new QVBoxLayout(defsBox);
    definitionsList_ = new QListWidget(defsBox);
    definitionsList_->setStyleSheet("QListWidget { font-family: 'Consolas', 'Courier New', monospace; }");
    defsLayout->addWidget(definitionsList_);

    auto* defsButtonsLayout = new QHBoxLayout();
    auto* removeSelectedBtn = new QPushButton("Quitar seleccionada", defsBox);
    auto* clearAllBtn = new QPushButton("Quitar todas", defsBox);
    connect(removeSelectedBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelectedDefinition);
    connect(clearAllBtn, &QPushButton::clicked, this, &MainWindow::onClearAllDefinitions);
    defsButtonsLayout->addWidget(removeSelectedBtn);
    defsButtonsLayout->addWidget(clearAllBtn);
    defsButtonsLayout->addStretch();
    defsLayout->addLayout(defsButtonsLayout);
    mainLayout->addWidget(defsBox);

    // --- 5) Botón de acción principal ---
    auto* generateBtn = new QPushButton("⚙ Generar Scanner C++", central);
    generateBtn->setStyleSheet(
        "QPushButton { font-size: 13pt; font-weight: bold; padding: 10px; "
        "background-color: #2e7d32; color: white; }");
    connect(generateBtn, &QPushButton::clicked, this, &MainWindow::onGenerateScanner);
    mainLayout->addWidget(generateBtn);

    // --- 6) Salida: código C++ generado ---
    auto* codeTitle = new QLabel("Código generado — Token* Scanner::nextToken():", central);
    mainLayout->addWidget(codeTitle);

    codeOutput_ = new QTextEdit(central);
    codeOutput_->setReadOnly(true);
    codeOutput_->setLineWrapMode(QTextEdit::NoWrap);
    QFont monoFont("Consolas", 10);
    monoFont.setStyleHint(QFont::Monospace);
    codeOutput_->setFont(monoFont);
    codeOutput_->setMinimumHeight(320);
    mainLayout->addWidget(codeOutput_);

    setCentralWidget(central);
    resize(980, 900);
}

QGridLayout* MainWindow::buildRegexButtonGrid() {
    auto* grid = new QGridLayout();
    const int columns = 10;
    int row = 0;
    int col = 0;

    auto placeNext = [&](QWidget* w) {
        grid->addWidget(w, row, col);
        col++;
        if (col >= columns) {
            col = 0;
            row++;
        }
    };

    // Letras a-z
    for (char c = 'a'; c <= 'z'; ++c) {
        auto* btn = new QPushButton(QString(QChar(c)));
        btn->setStyleSheet(kCharButtonStyle);
        connect(btn, &QPushButton::clicked, this, &MainWindow::onCharButtonClicked);
        placeNext(btn);
    }

    // Nueva fila para dígitos y operadores
    col = 0;
    row++;

    // Dígitos 0-9
    for (char c = '0'; c <= '9'; ++c) {
        auto* btn = new QPushButton(QString(QChar(c)));
        btn->setStyleSheet(kCharButtonStyle);
        connect(btn, &QPushButton::clicked, this, &MainWindow::onCharButtonClicked);
        placeNext(btn);
    }

    // Operadores: |  *  (  )
    col = 0;
    row++;
    const QString operators[] = {"|", "*", "(", ")"};
    for (const QString& op : operators) {
        auto* btn = new QPushButton(op);
        btn->setStyleSheet(kOperatorButtonStyle);
        connect(btn, &QPushButton::clicked, this, &MainWindow::onOperatorButtonClicked);
        placeNext(btn);
    }

    return grid;
}

void MainWindow::appendToRegex(const QString& piece) {
    currentRegex_ += piece;
    refreshRegexLabel();
}

void MainWindow::refreshRegexLabel() {
    regexLabel_->setText(currentRegex_.isEmpty() ? QStringLiteral("(vacío)") : currentRegex_);
}

void MainWindow::refreshDefinitionsList() {
    definitionsList_->clear();
    for (const auto& def : definitions_) {
        QString entry = QString::fromStdString(def.regex) + "   →   Token::" +
                         QString::fromStdString(def.tokenType);
        definitionsList_->addItem(entry);
    }
}

void MainWindow::onCharButtonClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    appendToRegex(btn->text());
}

void MainWindow::onOperatorButtonClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    appendToRegex(btn->text());
}

void MainWindow::onBackspaceClicked() {
    if (!currentRegex_.isEmpty()) {
        currentRegex_.chop(1);
        refreshRegexLabel();
    }
}

void MainWindow::onClearRegexClicked() {
    currentRegex_.clear();
    refreshRegexLabel();
}

void MainWindow::onAssignTokenType() {
    if (currentRegex_.isEmpty()) {
        QMessageBox::warning(this, "Expresión regular vacía",
                              "Construya una expresión regular con los botones antes de "
                              "asociarla a un tipo de token.");
        return;
    }

    // Validar que la ER sea sintácticamente correcta antes de aceptarla.
    try {
        RE::Parser parser(currentRegex_.toStdString());
        parser.parse();
    } catch (const RE::RegexParseException& e) {
        QMessageBox::critical(this, "Expresión regular inválida",
                               QString("La expresión regular no es válida:\n%1").arg(e.what()));
        return;
    }

    bool ok = false;
    QString typeName = QInputDialog::getText(
        this, "Tipo de Token",
        "Nombre del tipo (se usará como Token::NOMBRE, ej. ID, NUM, KEYWORD):",
        QLineEdit::Normal, QString(), &ok);
    if (!ok) return; // usuario canceló

    typeName = typeName.trimmed();
    static const QRegularExpression identifierRe("^[A-Za-z_][A-Za-z0-9_]*$");
    if (!identifierRe.match(typeName).hasMatch()) {
        QMessageBox::critical(this, "Nombre de tipo inválido",
                               "El tipo de token debe ser un identificador C++ válido "
                               "(letras, dígitos y guion bajo, sin empezar por dígito).");
        return;
    }

    definitions_.push_back({currentRegex_.toStdString(), typeName.toStdString()});
    currentRegex_.clear();
    refreshRegexLabel();
    refreshDefinitionsList();
}

void MainWindow::onRemoveSelectedDefinition() {
    int row = definitionsList_->currentRow();
    if (row < 0 || row >= static_cast<int>(definitions_.size())) return;
    definitions_.erase(definitions_.begin() + row);
    refreshDefinitionsList();
}

void MainWindow::onClearAllDefinitions() {
    if (definitions_.empty()) return;
    definitions_.clear();
    refreshDefinitionsList();
}

void MainWindow::onGenerateScanner() {
    if (definitions_.empty()) {
        QMessageBox::warning(this, "Sin definiciones",
                              "Asocie al menos una expresión regular a un tipo de token "
                              "antes de generar el scanner.");
        return;
    }

    try {
        RE::ThompsonBuilder thompson;
        RE::NFA nfa = thompson.combine(definitions_);

        RE::SubsetConstruction subsetConstruction;
        RE::DFA dfa = subsetConstruction.build(nfa);

        RE::CodeGenerator generator;
        std::string code = generator.generate(dfa);

        codeOutput_->setPlainText(QString::fromStdString(code));
    } catch (const RE::RegexParseException& e) {
        QMessageBox::critical(this, "Error al generar el scanner",
                               QString("Ocurrió un error procesando las expresiones "
                                       "regulares:\n%1")
                                   .arg(e.what()));
    }
}