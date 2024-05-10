#ifndef ICHATITEMMODEL_H
#define ICHATITEMMODEL_H



#include <QStandardItemModel>

class IChatItemModel : public QStandardItemModel
{
public:
    enum CustomRoles {
        ItemHeightRole = Qt::UserRole + 1, // 自定义角色，用于保存项目的高度
    };

    explicit IChatItemModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role) const override;
    void setItemHeight(const QModelIndex &index, int height) const ;
};



#endif // ICHATITEMMODEL_H
