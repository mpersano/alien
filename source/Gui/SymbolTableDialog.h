#pragma once
#include <QDialog>

#include "EngineInterface/Definitions.h"
#include "EngineInterface/SymbolTable.h"

namespace Ui {
class SymbolTableDialog;
}

class SymbolTableDialog : public QDialog
{
    Q_OBJECT
    
public:
	SymbolTableDialog(SymbolTable const* symbolTable, Serializer* serializer, QWidget *parent = nullptr);
	virtual ~SymbolTableDialog();

    SymbolTable* getSymbolTable ();

private:
	Q_SLOT void updateWidgetsFromSymbolTable ();
	Q_SLOT void itemSelectionChanged ();
	Q_SLOT void addButtonClicked ();
	Q_SLOT void delButtonClicked ();
	Q_SLOT void defaultButtonClicked ();
	Q_SLOT void loadButtonClicked ();
	Q_SLOT void saveButtonClicked ();
	Q_SLOT void mergeWithButtonClicked ();

private:
	void updateSymbolTableFromWidgets();

    Ui::SymbolTableDialog *ui;
	SymbolTable* _symbolTable = nullptr;
	Serializer* _serializer = nullptr;
};

