
/// \file pqPipelineListModel.h
/// \brief
///   The pqPipelineListModel class is used to represent the pipeline
///   in the form of a list.
///
/// \date 11/14/2005

#ifndef _pqPipelineListModel_h
#define _pqPipelineListModel_h


#include "QtWidgetsExport.h"
#include <QAbstractItemModel>

class pqPipelineListInternal;
class pqPipelineListItem;
class pqPipelineObject;
class pqPipelineServer;
class pqPipelineWindow;
class QPixmap;
class QVTKWidget;
class QWidget;
class vtkSMProxy;


/// \class pqPipelineListModel
/// \brief
///   The pqPipelineListModel class is used to represent the pipeline
///   in the form of a list.
class QTWIDGETS_EXPORT pqPipelineListModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum ItemType {
    Invalid = -1,
    Server = 0,
    Window,
    Source,
    Filter,
    Bundle,
    LinkBack,
    LinkOut,
    LinkIn,
    Split,
    Merge
  };

public:
  /// \brief
  ///   Creates a pqPipelineListModel instance.
  /// \param parent The parent object.
  pqPipelineListModel(QObject *parent=0);
  virtual ~pqPipelineListModel();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the number of columns for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of columns for the given index.
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets whether or not the given index has child items.
  /// \param parent The parent index.
  /// \return
  ///   True if the given index has child items.
  virtual bool hasChildren(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets a model index for a given location.
  /// \param row The row number.
  /// \param column The column number.
  /// \param parent The parent index.
  /// \return
  ///   A model index for the given location.
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the parent for a given index.
  /// \param index The model index.
  /// \return
  ///   A model index for the parent of the given index.
  virtual QModelIndex parent(const QModelIndex &index) const;

  /// \brief
  ///   Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return
  ///   The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

  /// \brief
  ///   Gets the flags for a given model index.
  ///
  /// The flags for an item indicate if it is enabled, editable, etc.
  ///
  /// \param index The model index.
  /// \return
  ///   The flags for the given model index.
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  //@}

  ItemType getTypeFor(const QModelIndex &index) const;

  vtkSMProxy *getProxyFor(const QModelIndex &index) const;
  QWidget *getWidgetFor(const QModelIndex &index) const;

  QModelIndex getIndexFor(vtkSMProxy *proxy) const;
  QModelIndex getIndexFor(QVTKWidget *window) const;

public slots:
  void clearPipeline();

  void addServer(pqPipelineServer *server);
  void removeServer(pqPipelineServer *server);

  void addWindow(pqPipelineWindow *window);
  void removeWindow(pqPipelineWindow *window);

  void addSource(pqPipelineObject *source);
  void removeSource(pqPipelineObject *source);

  void addFilter(pqPipelineObject *filter);
  void removeFilter(pqPipelineObject *filter);

  void addConnection(pqPipelineObject *source, pqPipelineObject *sink);
  void removeConnection(pqPipelineObject *source, pqPipelineObject *sink);

  void beginCreateAndAppend();
  void finishCreateAndAppend();
  void beginCreateAndInsert();
  void finishCreateAndInsert();

signals:
  void childAdded(const QModelIndex &index);

private:
  void addSubItem(pqPipelineObject *object, ItemType itemType);
  void removeLookupItems(pqPipelineListItem *item);

private:
  pqPipelineListInternal *Internal;
  pqPipelineListItem *Root;
  QPixmap *PixmapList;
};

#endif
