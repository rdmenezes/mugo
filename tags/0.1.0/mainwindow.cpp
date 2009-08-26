#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QtAlgorithms>
#include "mainwindow.h"
#include "gameinformationdialog.h"
#include "ui_mainwindow.h"

Q_DECLARE_METATYPE(go::node*);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setEncoding(ui->actionEncodingUTF8, "UTF-8");
    setShowMoveNumber(ui->actionNoMoveNumber, 0);
    setEditMode(ui->actionAlternateMove, BoardWidget::eAlternateMove);

    fileNew();
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* e){
    if (fileClose())
        e->accept();
    else
        e->ignore();
}

void MainWindow::keyPressEvent(QKeyEvent* event){
    if (event->key() == Qt::Key_Delete)
        deleteTreeWidget();
}

/**
* Slot
* File -> New
*/
void MainWindow::on_actionNew_triggered(){
    fileNew();
}

/**
* Slot
* File -> Open
*/
void MainWindow::on_actionOpen_triggered(){
    fileOpen();
}

/**
* Slot
* File -> Save
*/
void MainWindow::on_actionSave_triggered(){
    fileSave();
}

/**
* Slot
* File -> Save As
*/
void MainWindow::on_actionSaveAs_triggered(){
    fileSaveAs();
}

/**
* Slot
* File -> Save Board As Picture
*/
void MainWindow::on_actionSaveBoardAsPicture_triggered()
{
    QString selectedFilter;
    QString fname = QFileDialog::getSaveFileName(this, QString(), QString(),
        tr("PNG image(*.png);;Bitmap image(*.bmp);;JPEG image(*.jpeg *.jpg);;TIFF image(*.tiff *.tif)"),
        &selectedFilter);
    if (fname.isEmpty())
        return;

    const char* format[] = { "PNG", "BMP", "JPG", "TIFF" };
    int n;

    if (selectedFilter.indexOf("*.png") >= 0)
        n = 0;
    else if (selectedFilter.indexOf("*.bmp") >= 0)
        n = 1;
    else if (selectedFilter.indexOf("*.jpg") >= 0)
        n = 2;
    else if (selectedFilter.indexOf("*.tiff") >= 0)
        n = 3;
    else
        return;

    int w = ui->boardWidget->width();
    int h = ui->boardWidget->height();
    w = h = std::min(w, h);
    QImage image(w, h, QImage::Format_RGB32);
    ui->boardWidget->paint(&image);
    image.save(fname, format[n]);
}

/**
* Slot
* File -> Exit
*/
void MainWindow::on_actionExit_triggered(){
    close();
}

/**
* Slot
* Edit -> Game Information
*/
void MainWindow::on_actionGameInformation_triggered(){
    GameInformationDialog dlg(this, &ui->boardWidget->getData().root);
    if (dlg.exec() != QDialog::Accepted)
        return;

    ui->boardWidget->setDirty(true);
    setCaption();
}

/**
* Slot
* Edit -> Pass
*/
void MainWindow::on_actionPass_triggered(){
    go::node* currentNode = ui->boardWidget->getCurrentNode();
    go::node* node;
    if (currentNode->isBlack())
        node = new go::whiteNode(currentNode);
    else
        node = new go::blackNode(currentNode);
    ui->boardWidget->addNode(currentNode, node);
    ui->boardWidget->setCurrentNode(node);
}

/**
* Slot
* Edit -> Delete
*/
void MainWindow::on_actionDelete_triggered(){
    deleteTreeWidget();
}

/**
* Slot
* Edit -> Stone & Marker -> Alternate Move
*/
void MainWindow::on_actionAlternateMove_triggered(){
    setEditMode(ui->actionAlternateMove, BoardWidget::eAlternateMove);
}

/**
* Slot
* Edit -> Stone & Marker -> Add Black Stone
*/
void MainWindow::on_actionAddBlackStone_triggered(){
    setEditMode(ui->actionAddBlackStone, BoardWidget::eAddBlack);
}

/**
* Slot
* Edit -> Stone & Marker -> Add White Stone
*/
void MainWindow::on_actionAddWhiteStone_triggered(){
    setEditMode(ui->actionAddWhiteStone, BoardWidget::eAddWhite);
}

/**
* Slot
* Edit -> Stone & Marker -> Add Empty
*/
void MainWindow::on_actionAddEmpty_triggered(){
    setEditMode(ui->actionAddEmpty, BoardWidget::eAddEmpty);
}

/**
* Slot
* Edit -> Stone & Marker -> Add Label
*/
void MainWindow::on_actionAddLabel_triggered(){
    setEditMode(ui->actionAddLabel, BoardWidget::eLabelMark);
}

/**
* Slot
* Edit -> Stone & Marker -> Add Circle
*/
void MainWindow::on_actionAddCircle_triggered(){
    setEditMode(ui->actionAddCircle, BoardWidget::eCircleMark);
}

/**
* Slot
* Edit -> Stone & Marker -> Add Cross
*/
void MainWindow::on_actionAddCross_triggered(){
    setEditMode(ui->actionAddCross, BoardWidget::eCrossMark);
}

/**
* Slot
* Edit -> Stone & Marker -> Add Square
*/
void MainWindow::on_actionAddSquare_triggered(){
    setEditMode(ui->actionAddSquare, BoardWidget::eSquareMark);
}

/**
* Slot
* Edit -> Stone & Marker -> Add Triangle
*/
void MainWindow::on_actionAddTriangle_triggered(){
    setEditMode(ui->actionAddTriangle, BoardWidget::eTriangleMark);
}

/**
* Slot
* Edit -> Stone & Marker -> Delete Marker
*/
void MainWindow::on_actionDeleteMarker_triggered(){
    setEditMode(ui->actionDeleteMarker, BoardWidget::eDeleteMarker);
}

/**
* Slot
* Advance -> Encoding -> UTF-8
*/
void MainWindow::on_actionEncodingUTF8_triggered(){
    setEncoding(ui->actionEncodingUTF8, "UTF-8");
}

/**
* Slot
* Advance -> Encoding -> ISO8859_1
*/
void MainWindow::on_actionISO8859_1_triggered(){
    setEncoding(ui->actionISO8859_1, "ISO-8859-1");
}

/**
* Slot
* Advance -> Encoding -> Windows_1252
*/
void MainWindow::on_actionWindows_1252_triggered(){
    setEncoding(ui->actionWindows_1252, "Windows-1252");
}

/**
* Slot
* Advance -> Encoding -> Chinese Simplified(GB2312)
*/
void MainWindow::on_actionEncodingGB2312_triggered(){
    setEncoding(ui->actionEncodingGB2312, "GB2312");
}

/**
* Slot
* Advance -> Encoding -> Chinese Traditional(Big5)
*/
void MainWindow::on_actionEncodingBig5_triggered(){
    setEncoding(ui->actionEncodingBig5, "Big5");
}

/**
* Slot
* Advance -> Encoding -> Shift_JIS
*/
void MainWindow::on_actionEncodingShiftJIS_triggered(){
    setEncoding(ui->actionEncodingShiftJIS, "Shift_JIS");
}

/**
* Slot
* Advance -> Encoding -> JIS
*/
void MainWindow::on_actionEncodingJIS_triggered(){
    setEncoding(ui->actionEncodingJIS, "ISO 2022-JP");
}

/**
* Slot
* Advance -> Encoding -> EUC-JP
*/
void MainWindow::on_actionEncodingEucJP_triggered(){
    setEncoding(ui->actionEncodingEucJP, "EUC-JP");
}

/**
* Slot
* Advance -> Encoding -> EUC-KR
*/
void MainWindow::on_actionEncodingKorean_triggered(){
    setEncoding(ui->actionEncodingKorean, "EUC-KR");
}

/**
* Slot
* View -> Move Number-> No Move Number
*/
void MainWindow::on_actionNoMoveNumber_triggered(){
    setShowMoveNumber(ui->actionNoMoveNumber, 0);
}

/**
* Slot
* View -> Move Number-> Last 1 move
*/
void MainWindow::on_actionLast1Move_triggered(){
    setShowMoveNumber(ui->actionLast1Move, 1);
}

/**
* Slot
* View -> Move Number-> Last 2 moves
*/
void MainWindow::on_actionLast2Moves_triggered(){
    setShowMoveNumber(ui->actionLast2Moves, 2);
}

/**
* Slot
* View -> Move Number-> Last 5 moves
*/
void MainWindow::on_actionLast5Moves_triggered(){
    setShowMoveNumber(ui->actionLast5Moves, 5);
}

/**
* Slot
* View -> Move Number-> Last 10 moves
*/
void MainWindow::on_actionLast10Moves_triggered(){
    setShowMoveNumber(ui->actionLast10Moves, 10);
}

/**
* Slot
* View -> Move Number-> Last 20 moves
*/
void MainWindow::on_actionLast20Moves_triggered(){
    setShowMoveNumber(ui->actionLast20Moves, 20);
}

/**
* Slot
* View -> Move Number-> Last 50 moves
*/
void MainWindow::on_actionLast50Moves_triggered(){
    setShowMoveNumber(ui->actionLast50Moves, 50);
}

/**
* Slot
* View -> Move Number-> All Moves
*/
void MainWindow::on_actionAllMoves_triggered(){
    setShowMoveNumber(ui->actionAllMoves, -1);
}

/**
* Slot
* View -> Comment Window
*/
void MainWindow::on_actionCommentWindow_triggered(){
    if (ui->commentDockWidget->isVisible())
        ui->commentDockWidget->hide();
    else
        ui->commentDockWidget->show();
}

/**
* Slot
* View -> Branch Window
*/
void MainWindow::on_actionBranchWindow_triggered(){
    if (ui->branchDockWidget->isVisible())
        ui->branchDockWidget->hide();
    else
        ui->branchDockWidget->show();
}

/**
* Slot
* Help -> About
*/
void MainWindow::on_actionAbout_triggered(){
    QMessageBox::about(this, tr(APP_NAME), tr(APP_NAME " version " VERSION "\n\nCopyright 2009 Naoya Sase. All rights reserved."));
}

/**
* Slot
* Help -> About qt
*/
void MainWindow::on_actionAboutQT_triggered(){
    qApp->aboutQt();
}

/**
* Slot
* new node was created by BoardWidget.
*/
void MainWindow::on_boardWidget_nodeAdded(go::node* /*parent*/, go::node* node){
    addTreeWidget(*node);
    setCaption();
}

/**
* Slot
* node was deleted by BoardWidget.
*/
void MainWindow::on_boardWidget_nodeDeleted(go::node* node){
    deleteTreeWidget(node);
    deleteTreeWidgetForMap(node);

    ui->boardWidget->setCurrentNode(node->parent);

    setCaption();
}

/**
* Slot
*/
void MainWindow::on_boardWidget_nodeModified(go::node* node){
    setCaption();

    QTreeWidgetItem* treeWidget = nodeToTreeWidget[node];
    if (treeWidget == NULL)
        return;
    treeWidget->setText(0, createTreeText(node));
}

/**
* Slot
* current node was changed by BoardWidget.
*/
void MainWindow::on_boardWidget_currentNodeChanged(go::node* node){
    setTreeWidget(node);
    ui->commentWidget->setPlainText(node->comment);
}

/**
* Slot
* branch dock widget was showed or hid.
*/
void MainWindow::on_branchDockWidget_visibilityChanged(bool visible){
    ui->actionBranchWindow->setChecked(visible);
}

/**
* Slot
* node was changed on branch tree view.
*/
void MainWindow::on_branchWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/){
    if (current == NULL){
        ui->boardWidget->setCurrentNode();
        return;
    }

    QVariant v = current->data(0, Qt::UserRole);
    go::node* n = v.value<go::node*>();

    ui->boardWidget->setCurrentNode(n);
}

/**
* Slot
* comment dock widget was showed or hid.
*/
void MainWindow::on_commentDockWidget_visibilityChanged(bool visible){
    ui->actionCommentWindow->setChecked(visible);
}

/**
* Slot
* comment was modified.
*/
void MainWindow::on_commentWidget_textChanged()
{
    go::node* currentNode = ui->boardWidget->getCurrentNode();
    if (currentNode->comment == ui->commentWidget->toPlainText())
        return;
    currentNode->comment = ui->commentWidget->toPlainText();
    ui->boardWidget->modifyNode(currentNode);
}

/**
* set text to MainWindow's title bar
*
* if game information has player info, set player name to window text.
*/
void MainWindow::setCaption(){
    QString caption;
    if(fileName.isEmpty())
        caption = "Untitled";
    else{
        QFileInfo info(fileName);
        caption = info.fileName();
    }

    if (ui->boardWidget->isDirty())
        caption.append(" *");

    caption.append(" - ");

    go::informationNode& gameInfo = ui->boardWidget->getData().root;
    bool hasPlayerInfo = !gameInfo.whitePlayer.isEmpty() || !gameInfo.whiteRank.isEmpty() ||
                            !gameInfo.blackPlayer.isEmpty() || !gameInfo.blackRank.isEmpty();
    if (hasPlayerInfo){
        caption.append(ui->boardWidget->getData().root.whitePlayer);
        caption.push_back(' ');
        caption.append(ui->boardWidget->getData().root.whiteRank);
        caption.append(" (W) vs ");
        caption.append(ui->boardWidget->getData().root.blackPlayer);
        caption.push_back(' ');
        caption.append(ui->boardWidget->getData().root.blackRank);
        caption.append(" (B) (Result:");
        caption.append(ui->boardWidget->getData().root.result);
        caption.append(") - ");
    }
    caption.append(APP_NAME);

    setWindowTitle(caption);
}

/**
* a new document is created if current document can be closed.
*/
bool MainWindow::fileNew(){
    return fileClose();
}

/**
* file open.
*/
bool MainWindow::fileOpen(){
    if (maybeSave() == false)
        return false;

    QString fname = QFileDialog::getOpenFileName(this, QString(), QString(), tr("sgf(*.sgf)"));
    if (fname.isEmpty())
        return  false;

    return fileOpen(fname);
}

/**
* file open.
*/
bool MainWindow::fileOpen(const QString& fname){
    fileName = fname;

    go::sgf sgf;
    sgf.read(fname, codec);
    ui->boardWidget->setData(sgf);
    ui->boardWidget->setDirty(false);

    setTreeData();
    setCaption();

    return true;
}

/**
* file save.
*/
bool MainWindow::fileSave(){
    if (fileName.isEmpty())
        return fileSaveAs();
    else
        return fileSaveAs(fileName);
}

/**
* file saveas.
*/
bool MainWindow::fileSaveAs(){
    QString fname = QFileDialog::getSaveFileName(this, QString(), QString(), tr("sgf(*.sgf)"));
    if (fname.isEmpty())
        return false;

    return fileSaveAs(fname);
}

/**
* file saveas.
*/
bool MainWindow::fileSaveAs(const QString& fname){
    fileName = fname;
    go::sgf sgf;
    ui->boardWidget->getData(sgf);
    sgf.save(fileName, codec);
    ui->boardWidget->setDirty(false);

    setCaption();

    return true;
}

/**
* file close.
*/
bool MainWindow::fileClose(){
    if (maybeSave() == false)
        return false;

    fileName.clear();
    ui->boardWidget->clear();

    setTreeData();
    setCaption();

    return true;
}

/**
*/
bool MainWindow::maybeSave(){
    if (!ui->boardWidget->isDirty())
        return true;
    QMessageBox::StandardButton ret =
    QMessageBox::warning(this, tr(APP_NAME),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

/**
*/
void MainWindow::setTreeData(){
    ui->branchWidget->clear();

    addTreeWidget(NULL, ui->boardWidget->getData().root);
    ui->boardWidget->setCurrentNode();
}

QTreeWidgetItem* MainWindow::addTreeWidget(go::node& node){
    QTreeWidgetItem* currentWidget = ui->branchWidget->currentItem();
    if (currentWidget == NULL)
        currentWidget = ui->branchWidget->topLevelItem(0);

    remakeTreeWidget(currentWidget);
    return addTreeWidget(currentWidget, node);
}

QTreeWidgetItem* MainWindow::addTreeWidget(QTreeWidgetItem* parentWidget, go::node& node){
    QTreeWidgetItem* parentWidget2 = parentWidget ? parentWidget->parent() : NULL;
    go::node* parentNode  = getNode(parentWidget);
    go::node* parentNode2 = getNode(parentWidget2);

    QTreeWidgetItem* newWidget = NULL;
    if ((parentNode && parentNode->childNodes.front() != &node) || (parentNode2 && parentNode2->childNodes.size() > 1)){
        newWidget = createTreeWidget(parentWidget, node);
    }
    else
        newWidget = createTreeWidget(parentWidget2, node);

    go::nodeList::const_iterator iter = node.childNodes.begin();
    while (iter != node.childNodes.end()){
        addTreeWidget(newWidget, **iter);
        ++iter;
    }

    return newWidget;
}

QTreeWidgetItem* MainWindow::remakeTreeWidget(QTreeWidgetItem* currentWidget){
    QTreeWidgetItem* child1 = NULL;
    go::node* node1 = NULL;
    for (int i=0; i<currentWidget->childCount();){
        QTreeWidgetItem* child2 = currentWidget->child(i);
        if (child1 == NULL){
            child1 = child2;
            node1  = getNode(child1);
            ++i;
        }
        else{
            go::node* node2 = getNode(child2);
            if (node1 == node2->parent){
                child1->addChild( child2->clone() );
                delete child2;
                node1 = node2;
            }
            else
                ++i;
        }
    }
    return NULL;
}

QTreeWidgetItem* MainWindow::createTreeWidget(QTreeWidgetItem* parentWidget, go::node& node){
    // TreeItemを作成
    QTreeWidgetItem* nodeWidget = new QTreeWidgetItem( QStringList(createTreeText(&node)) );
    QVariant v;
    v.setValue(&node);
    nodeWidget->setData(0, Qt::UserRole, v);
    nodeToTreeWidget[&node] = nodeWidget;

    // TreeItemにIconを設定
    const go::stoneNode* stoneNode = dynamic_cast<const go::stoneNode*>(&node);
    if (stoneNode){
        if (stoneNode->isBlack())
            nodeWidget->setIcon(0, QIcon(":/res/black_64.png"));
        else
            nodeWidget->setIcon(0, QIcon(":/res/white_64.png"));
    }
    else
        nodeWidget->setIcon(0, QIcon(":/res/green_64.png"));

    // TreeにItemを追加
    if (parentWidget)
        parentWidget->addChild(nodeWidget);
    else
        ui->branchWidget->addTopLevelItem(nodeWidget);

    return nodeWidget;
}

void MainWindow::deleteTreeWidget(){
    QTreeWidgetItem* currentWidget = ui->branchWidget->currentItem();
    if (currentWidget == NULL)
        return;

    deleteTreeWidget(currentWidget);
}

void MainWindow::deleteTreeWidget(QTreeWidgetItem* treeWidget){
    go::node* node = getNode(treeWidget);
    ui->boardWidget->deleteNode(node);
}

void MainWindow::deleteTreeWidget(go::node* node){
    QTreeWidgetItem* treeWidget = nodeToTreeWidget[node];
    go::nodeList::iterator iter = node->childNodes.begin();
    while (iter != node->childNodes.end()){
        QTreeWidgetItem* treeWidget2 = nodeToTreeWidget[*iter];
        if (treeWidget2->parent() != treeWidget)
            deleteTreeWidget(*iter);
        ++iter;
    }
    delete treeWidget;
}

void MainWindow::deleteTreeWidgetForMap(go::node* node){
    NodeToTreeWidgetType::iterator iter = nodeToTreeWidget.find(node);
    if (iter != nodeToTreeWidget.end())
        nodeToTreeWidget.erase(iter);

    go::nodeList::iterator iter2 = node->childNodes.begin();
    while (iter2 != node->childNodes.end()){
        deleteTreeWidgetForMap(*iter2);
        ++iter2;
    }
}

void MainWindow::setTreeWidget(go::node* n){
    NodeToTreeWidgetType::iterator iter = nodeToTreeWidget.find(n);
    if (iter != nodeToTreeWidget.end())
        ui->branchWidget->setCurrentItem( iter.value() );
}

QString MainWindow::createTreeText(const go::node* node){
    const go::stoneNode* stoneNode = dynamic_cast<const go::stoneNode*>(node);
    QString s;
    if (stoneNode){
        if (stoneNode->isPass())
            s.append("Pass");
        else
            s.append( ui->boardWidget->getXYString(node->getX(), node->getY()) );
    }

    if (!s.isEmpty())
        s.push_back(' ');
    s.append( node->toString() );

    return s;
}

go::node* MainWindow::getNode(QTreeWidgetItem* treeWidget){
    if (treeWidget == NULL)
        return NULL;

    QVariant v = treeWidget->data(0, Qt::UserRole);
    return v.value<go::node*>();
}

void MainWindow::setEncoding(QAction* action, const char* codecName){
    static QAction* actions[] = {
        ui->actionEncodingUTF8,
        ui->actionISO8859_1,
        ui->actionWindows_1252,
        ui->actionEncodingGB2312,
        ui->actionEncodingBig5,
        ui->actionEncodingKorean,
        ui->actionEncodingEucJP,
        ui->actionEncodingJIS,
        ui->actionEncodingShiftJIS,
    };
    static const int N = sizeof(actions) / sizeof(actions[0]);

    for (int i=0; i<N; ++i)
        actions[i]->setChecked(actions[i] == action);

    codec = QTextCodec::codecForName(codecName);
}

void MainWindow::setShowMoveNumber(QAction* action, int moveNumber){
    static QAction* actions[] = {
        ui->actionNoMoveNumber,
        ui->actionLast1Move,
        ui->actionLast2Moves,
        ui->actionLast5Moves,
        ui->actionLast10Moves,
        ui->actionLast20Moves,
        ui->actionLast50Moves,
        ui->actionAllMoves
    };
    static const int N = sizeof(actions) / sizeof(actions[0]);

    for (int i=0; i<N; ++i)
        actions[i]->setChecked(actions[i] == action);

    ui->boardWidget->setShowMoveNumber(moveNumber);
    ui->boardWidget->repaint();
}

void MainWindow::setEditMode(QAction* action, BoardWidget::eEditMode editMode){
   static QAction* actions[] = {
        ui->actionAlternateMove,
        ui->actionAddBlackStone,
        ui->actionAddWhiteStone,
        ui->actionAddEmpty,
        ui->actionAddLabel,
        ui->actionAddCircle,
        ui->actionAddCross,
        ui->actionAddSquare,
        ui->actionAddTriangle,
        ui->actionDeleteMarker,
    };
    static const int N = sizeof(actions) / sizeof(actions[0]);

    for (int i=0; i<N; ++i)
        actions[i]->setChecked(actions[i] == action);

    ui->boardWidget->setEditMode(editMode);
}