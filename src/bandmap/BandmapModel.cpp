#include "bandmap/BandmapModel.h"

namespace TwoPLogger {

BandmapModel::BandmapModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

BandmapModel::~BandmapModel() = default;

int BandmapModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 0;
}

int BandmapModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return 6; // Freq | Callsign | DXCC | Zone | Age | Comment
}

QVariant BandmapModel::data(const QModelIndex& /*index*/, int /*role*/) const
{
    return {};
}

QVariant BandmapModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section) {
    case 0: return QStringLiteral("Freq");
    case 1: return QStringLiteral("Callsign");
    case 2: return QStringLiteral("DXCC");
    case 3: return QStringLiteral("Zone");
    case 4: return QStringLiteral("Age");
    case 5: return QStringLiteral("Comment");
    }
    return {};
}

} // namespace TwoPLogger
