#include "log/LogBook.h"

namespace TwoPLogger {

LogBook::LogBook(QObject* parent)
    : QObject(parent)
{}

LogBook::~LogBook() = default;

bool LogBook::open(const QString& /*path*/) { return false; }

void LogBook::close() {}

bool LogBook::isOpen() const { return false; }

} // namespace TwoPLogger
