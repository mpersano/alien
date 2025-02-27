﻿#pragma once

#include <QWidget>

#include "EngineInterface/Definitions.h"
#include "Gui/Definitions.h"

class DataEditView
	: public QObject
{
	Q_OBJECT
public:
	DataEditView(QWidget * parent = nullptr);
	virtual ~DataEditView() = default;

	void init(IntVector2D const& upperLeftPosition, DataEditModel* model, DataEditController* controller
		, CellComputerCompiler* compiler);

	void switchToNoEditor();
	void switchToCellEditorWithComputerWithToken(UpdateDescription update);
	void switchToCellEditorWithoutComputerWithToken(UpdateDescription update);
	void switchToCellEditorWithComputerWithoutToken();
	void switchToCellEditorWithoutComputerWithoutToken();
	void switchToParticleEditor();
	void switchToSelectionEditor();

	void show(bool visible);
	void updateDisplay(UpdateDescription update = UpdateDescription::All) const;

private:
	enum class EditorSelector {
		No, 
		CellWithComputerWithToken, 
		CellWithoutComputerWithToken, 
		CellWithComputerWithoutToken, 
		CellWithoutComputerWithoutToken, 
		Particle, 
		Selection
	};
	void saveTabPositionForCellEditor();
	int getTabPositionForCellEditor();

	bool _visible = false;
	EditorSelector _editorSelector = EditorSelector::No;

	DataEditModel* _model = nullptr;

	QTabWidget* _mainTabWidget = nullptr;
	ClusterEditTab* _clusterTab = nullptr;
	CellEditTab* _cellTab = nullptr;
	MetadataEditTab* _metadataTab = nullptr;
	ParticleEditTab* _particleTab = nullptr;
	SelectionEditTab* _selectionTab = nullptr;

	QTabWidget* _computerTabWidget = nullptr;
	CellComputerEditTab* _computerTab = nullptr;

	QTabWidget* _symbolTabWidget = nullptr;
	SymbolEditTab* _symbolTab = nullptr;

	TokenEditTabWidget* _tokenTabWidget = nullptr;

	int _savedTabPosition = 0;
	IntVector2D _upperLeftPosition;
};
