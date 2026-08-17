#pragma once

#include <QMainWindow>
#include <QString>
#include <vector>

#include "regexengine.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QTextEdit;
class QListWidget;
class QGridLayout;
QT_END_NAMESPACE

// MainWindow: herramienta visual para construir expresiones regulares
// exclusivamente mediante botones (sin campos de texto editables), asociar
// cada ER a un tipo de token, y generar el código C++ del método
// Scanner::nextToken() correspondiente al autómata combinado.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    private slots:
        void onCharButtonClicked();       // botones a-z, 0-9
    void onOperatorButtonClicked();   // botones |, *, (, )
    void onBackspaceClicked();        // borra el último carácter de la ER actual
    void onClearRegexClicked();       // limpia la ER actual (no las definiciones ya asignadas)
    void onAssignTokenType();         // asocia la ER actual a un Token::TIPO
    void onRemoveSelectedDefinition();// quita la definición seleccionada de la lista
    void onClearAllDefinitions();     // quita todas las definiciones
    void onGenerateScanner();         // pipeline completo -> código en el QTextEdit

private:
    void buildUi();
    QGridLayout* buildRegexButtonGrid();
    void appendToRegex(const QString& piece);
    void refreshRegexLabel();
    void refreshDefinitionsList();

    // --- Estado ---
    QString currentRegex_;
    std::vector<RE::TokenDefinition> definitions_;

    // --- Widgets ---
    QLabel* regexLabel_ = nullptr;
    QListWidget* definitionsList_ = nullptr;
    QTextEdit* codeOutput_ = nullptr;
};