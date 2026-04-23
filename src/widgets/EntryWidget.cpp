#include "widgets/EntryWidget.h"

namespace TwoPLogger {

EntryWidget::EntryWidget(QWidget* parent)
    : QWidget(parent)
{}

EntryWidget::~EntryWidget() = default;

QString EntryWidget::callsign() const { return {}; }

void EntryWidget::clearFields() {}

} // namespace TwoPLogger
