﻿#include "StartScreenWidget.h"

StartScreenWidget::StartScreenWidget(QWidget * parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	ui.widget->setStyleSheet("border-image: url(:/Tutorial/logo.png) 0 0 0 0 stretch stretch;");
}
