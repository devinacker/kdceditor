#ifndef __OBJECT_LIST_WINDOW_H__
#define __OBJECT_LIST_WINDOW_H__

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLabel>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QString>

#include <cstddef>
#include <vector>

namespace Ui {
class ObjectListWindow;
}

struct RenderableAsset {
  QLabel* label;
  int width;
  int height;
  int identifier;

  friend bool operator==(const RenderableAsset& n1, const RenderableAsset& n2) {
    return n1.label == n2.label;
  }

  friend bool operator==(const RenderableAsset& n1, const QLabel* label) {
    return n1.label == label;
  }

  friend bool operator==(const QLabel* label, const RenderableAsset& n1) {
    return n1.label == label;
  }
};

class ObjectListWindow : public QDialog {
  Q_OBJECT

 public:
  explicit ObjectListWindow(QWidget* parent = nullptr);
  ~ObjectListWindow();

 protected slots:
  void mousePressEvent(QMouseEvent* event);
  void resizeEvent(QResizeEvent* event);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragMoveEvent(QDragMoveEvent *event);

 protected:
  void setupObjectIcons();
  void layoutIcons();
  void layoutIconSet(const QString& layoutName, const std::vector<RenderableAsset>& assets);

 private:
  Ui::ObjectListWindow* ui;

  std::vector<RenderableAsset> _character_assets;
  std::vector<RenderableAsset> _obstacle_assets;
};

#endif
