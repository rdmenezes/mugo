/*
    mugo, sgf editor.
    Copyright (C) 2009-2011 nsase.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <math.h>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include "sgfdocument.h"

/**
  GraphicsView of Goban
*/
class BoardWidget : public QGraphicsView
{
    Q_OBJECT
public:
    enum EditMode{
        eAlternateMove, eAddBlack, eAddWhite, eAddEmpty, eAddLabel, eAddLabelManually, eAddCircle, eAddCross, eAddTriangle, eAddSquare, eRemoveMarker,
    };

    struct Data{
        Data() : color(Go::eDame), stone(NULL){}

        Go::Color color;
        QGraphicsItem* stone;
    };

    // constructor
    explicit BoardWidget(SgfDocument* doc, QWidget *parent = 0);

    // document
    const SgfDocument* document() const{ return document_; }
    SgfDocument* document(){ return document_; }

    // get current game, node
    Go::NodePtr currentGame() const{ return currentGame_; }
    Go::NodePtr currentNode() const{ return currentNode_; }
    Go::InformationPtr rootInformation() const{ return rootInformation_; }
    Go::InformationPtr currentInformation() const{ return currentInformation_; }

    // get board size
    int xsize() const { return rootInformation_->xsize(); }
    int ysize() const { return rootInformation_->ysize(); }

signals:
    void gameChanged(const Go::NodePtr& game);
    void informationChanged(const Go::InformationPtr& information);
    void nodeChanged(const Go::NodePtr& node);

public slots:
    bool setGame(const Go::NodePtr& game);
    bool setInformation(const Go::InformationPtr& information);
    bool setNode(const Go::NodePtr& node);

protected:
    // event
    void resizeEvent(QResizeEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void wheelEvent(QWheelEvent* e);
    void onLButtonUp(QMouseEvent* e);
    void onRButtonUp(QMouseEvent* e);

    // set graphics items position
    void setItemsPosition();
    void setItemsPosition(const QSize& size);
    void setVLinesPosition(int x, int y, int gridSize);
    void setHLinesPosition(int x, int y, int gridSize);
    void setStarsPosition();
    void setDataPosition();

    // get items position
    qreal getGridSize() const{ return fabs(vLines[1]->line().x1() - vLines[0]->line().x1()); }

    // create buffer
    void createBoardBuffer();
    void killStone(int x, int y);
    void killStone(int x, int y, Go::Color color);
    bool isDeadStone(int x, int y);
    bool isDeadStone(int x, int y, Go::Color color);
    bool isDeadStone(int x, int y, Go::Color color, QVector< QVector<bool> >& checked);
    bool isKillStone(int x, int y);
    void createNodeList(Go::NodePtr node);

    // create stone item
    QGraphicsItem* createStoneItem(Go::Color color, int sgfX, int sgfY);

    // get star positions
    void getStarPositions(QList<int>& xstarpos, QList<int>& ystarpos) const;

    // transform coordinates
    bool viewToSgfCoordinate(qreal viewX, qreal viewY, int& sgfX, int& sgfY) const;
    bool sgfToViewCoordinate(int sgfX, int sgfY, qreal& viewX, qreal& viewY) const;

    // move stone
    bool alternateMove(int sgfX, int sgfY);
    void back(int step=1);
    void forward(int step=1);

    SgfDocument* document_;
    Go::NodePtr currentGame_;
    Go::InformationPtr rootInformation_;
    Go::InformationPtr currentInformation_;
    Go::NodePtr currentNode_;
    Go::NodeList currentNodeList_;
    int capturedWhite_;
    int capturedBlack_;

    QVector< QVector<Data> > data;
    QVector< QVector<Go::Color> > buffer;

    QGraphicsRectItem* board;
    QGraphicsRectItem* shadow;
    QList<QGraphicsLineItem*> vLines;
    QList<QGraphicsLineItem*> hLines;
    QList<QGraphicsEllipseItem*> stars;

    EditMode editMode_;

private slots:
    void on_document_nodeAdded(const Go::NodePtr& node);
    void on_document_nodeDeleted(const Go::NodePtr& node);
};

#endif // BOARDWIDGET_H
