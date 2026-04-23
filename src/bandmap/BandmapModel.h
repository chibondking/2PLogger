#pragma once
#include <QAbstractTableModel>

namespace TwoPLogger {

class BandmapModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit BandmapModel(QObject* parent = nullptr);
    ~BandmapModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
};

} // namespace TwoPLogger
