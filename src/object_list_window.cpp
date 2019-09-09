#include "object_list_window.h"
#include "ui_object_list_window.h"

#include "graphics.h"
#include "kirby.h"

#include <QBitmap>
#include <QDrag>
#include <QLayout>
#include <QMimeData>
#include <QMessageBox>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QSettings>
#include <string>

#include <QtDebug>

using std::string;

ObjectListWindow::ObjectListWindow(QWidget* parent):
    QDialog(parent, Qt::Tool
            | Qt::CustomizeWindowHint
            | Qt::WindowTitleHint
            | Qt::WindowMinimizeButtonHint
            | Qt::WindowCloseButtonHint
            //                | Qt::WindowStaysOnTopHint
            ),
    ui(new Ui::ObjectListWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    this->setupObjectIcons();
}

ObjectListWindow::~ObjectListWindow() {
  delete ui;
}

void ObjectListWindow::setupObjectIcons() {
  // add obstacle types to dropdown (only valid ones)
  for (StringMap::const_iterator i = kirbyObstacles.begin(); i != kirbyObstacles.end(); i++) {
    // Skip obstacles that shouldn't really be used.
    if (i->first == 0x0d  // King Dedede (course 24-1 only)
        || i->first == 0xc3  // Kirby (course 24-1 only)
        ) {
      continue;
    }

    if (i->second.size()) {
      const uint offset = i->first;
      const QString name = i->second;

      // Get the pixmap for the obstacle.
      const QPixmap* pixmap;
      int frame;
      if (Util::Instance()->GetPixmapSettingsForObstacle(offset, &pixmap, &frame)) {
        QPixmap piece = pixmap->copy(frame * TILE_SIZE, 0, TILE_SIZE, pixmap->height());

        // Crop to non-transparent bounds.
        const QImage image = piece.toImage();
        bool should_break = false;
        int ymin = 0;
        int ymax = image.height();

        for (int y = 0; y < image.height(); ++y) {
          const uchar* scanline = image.constScanLine(y);
          for (int x = 0; x < image.width(); ++x) {
            if (scanline[x * 4] != 0) {
              should_break = true;
              ymin = y - 3;
            }
          }

          if (should_break) {
            break;
          }
        }

        should_break = false;
        for (int y = image.height() - 1; y >= 0; --y) {
          const uchar* scanline = image.scanLine(y);
          for (int x = 0; x < image.width(); ++x) {
            if (scanline[x * 4] != 0) {
              should_break = true;
              ymax = y + 3;
            }
          }

          if (should_break) {
            break;
          }
        }

        qDebug() << name << ": (" << ymin << ", " << ymax << ")";

        // Create a new label and assign the pixmap
        QLabel* label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        label->setPixmap(piece.copy(0, ymin, piece.width(), ymax - ymin));
        // label->setText(name);

        RenderableAsset asset;
        asset.label = label;
        asset.width = TILE_SIZE;
        asset.height = ymax - ymin;
        asset.identifier = i->first;

        if (Util::Instance()->IsObstacleCharacterType(offset)) {
          this->_character_assets.push_back(asset);
        } else {
          this->_obstacle_assets.push_back(asset);
        }
      }
    }
  }

  this->layoutIcons();
}

void ObjectListWindow::layoutIconSet(const QString& layoutName, const std::vector<RenderableAsset>& assets) {
  QGridLayout* const layout = this->findChild<QGridLayout*>(layoutName);
  layout->setContentsMargins(0,0,0,0);

  for (int i = 0; i < assets.size(); ++i) {
    layout->removeWidget(assets[i].label);
  }

  const int width = this->width();
  const int padding = 20;
  int consumed = 0;
  int col = 0;
  int row = 0;
  for (int i = 0; i < assets.size(); ++i) {
    if (consumed + assets[i].width + padding > width) {
      consumed = 0;
      col = 0;
      row++;
    }

    layout->addWidget(assets[i].label, row, col);
    consumed += assets[i].width + padding;
    col++;
  }
}

void ObjectListWindow::layoutIcons() {
  layoutIconSet("charactersHolder", _character_assets);
  layoutIconSet("obstaclesHolder", _obstacle_assets);
}

void ObjectListWindow::resizeEvent(QResizeEvent* event) {
  this->layoutIcons();
}

void ObjectListWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void ObjectListWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void ObjectListWindow::mousePressEvent(QMouseEvent* event) {
  qDebug() << "Mouse Press Event: [" << event->button() << ", " << event->x() << ", " << event->y() << "]";

  QLabel *child = static_cast<QLabel*>(childAt(event->pos()));
  auto it = std::find(_character_assets.begin(), _character_assets.end(), child);

  if (it == _character_assets.end()) {
    it = std::find(_obstacle_assets.begin(), _obstacle_assets.end(), child);
    if (it == _obstacle_assets.end()) {
      qDebug() << "No element found at click";
      return;
    }
  }

  QPixmap pixmap = *child->pixmap();
  const int identifier = it->identifier;

  QByteArray itemData;
  QDataStream dataStream(&itemData, QIODevice::WriteOnly);
  dataStream << identifier;

  QMimeData* mimeData = new QMimeData;
  mimeData->setData("application/x-dnditemdata", itemData);

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setPixmap(pixmap);
  drag->setHotSpot(event->pos() - child->pos());

  QPixmap tempPixmap = pixmap;
  QPainter painter;
  painter.begin(&tempPixmap);
  painter.fillRect(pixmap.rect(), QColor(127, 127, 127, 127));
  painter.end();

  child->setPixmap(tempPixmap);

  if (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction) == Qt::MoveAction) {
    child->close();
  } else {
    child->show();
    child->setPixmap(pixmap);
  }
}
