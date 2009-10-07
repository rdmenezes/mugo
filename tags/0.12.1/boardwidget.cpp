#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>
#include <QPainter>
#include <QPrinter>
#include <QMouseEvent>
#include <QSound>
#include <QInputDialog>
#include <QList>
#include <QDateTime>
#include <math.h>
#include "appdef.h"
#include "boardwidget.h"
#include "mainwindow.h"
#include "command.h"
#include "ui_boardwidget.h"

#ifdef Q_WS_WIN
#   define usleep(X) Sleep(X/1000)
#else
#   include <unistd.h>
#endif

Sound::Sound(QWidget* parent_) : parent(parent_){
#if defined(Q_WS_WIN)
    lastClock = 0;
    memset(&mop, 0, sizeof(mop));
#else
    media = Phonon::createPlayer(Phonon::NotificationCategory);
#endif
}

Sound::~Sound(){
#if defined(Q_WS_WIN)
#else
    delete media;
#endif
}

void Sound::setCurrentSource(const QString& source){
#if defined(Q_WS_WIN)
    if (mop.wDeviceID != 0)
        mciSendCommand(mop.wDeviceID, MCI_CLOSE, 0, 0);

    memset(&mop, 0, sizeof(mop));
    mop.lpstrDeviceType  = L"WaveAudio";
    mop.lpstrElementName = (WCHAR*)source.utf16();
    mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE|MCI_OPEN_ELEMENT, (DWORD)&mop);
#else
    media->setCurrentSource(source);
#endif
}

void Sound::play(){
#if defined(Q_WS_WIN)
    qreal currentClock = (qreal)clock() / CLOCKS_PER_SEC;
    if (mop.wDeviceID && currentClock - lastClock > 0.2){
        mciSendCommand(mop.wDeviceID, MCI_STOP, 0, 0);
        mciSendCommand(mop.wDeviceID, MCI_SEEK, MCI_SEEK_TO_START, 0);
        mciSendCommand(mop.wDeviceID, MCI_PLAY, 0, 0);
        lastClock = currentClock;
    }
#else
    if (media->currentTime() == media->totalTime()){
        media->stop();
        media->seek(0);
    }
    media->play();
#endif
}



namespace{
    const char* hiragana[] = {"あ","い","う","え","お","か","き","く","け","こ","さ","し","す","せ","そ","た","ち","つ","て","と","な","に","ぬ","ね","の","は","ひ","ふ","へ","ほ","ま","み","む","め","も","や","ゆ","よ","ら","り","る","れ","ろ","わ","を","ん"};
    const char* katakana[] = {"ア","イ","ウ","エ","オ","カ","キ","ク","ケ","コ","サ","シ","ス","セ","ソ","タ","チ","ツ","テ","ト","ナ","ニ","ヌ","ネ","ノ","ハ","ヒ","フ","ヘ","ホ","マ","ミ","ム","メ","モ","ヤ","ユ","ヨ","ラ","リ","ル","レ","ロ","ワ","ヲ","ン"};
    const char* hira_iroha[] = {"い","ろ","は","に","ほ","へ","と","ち","り","ぬ","る","を","わ","か","よ","た","れ","そ","つ","ね","な","ら","む","う","ゐ","の","お","く","や","ま","け","ふ","こ","え","て","あ","さ","き","ゆ","め","み","し","ゑ","ひ","も","せ","す","ん"};
    const char* kana_iroha[] = {"イ","ロ","ハ","ニ","ホ","ヘ","ト","チ","リ","ヌ","ル","ヲ","ワ","カ","ヨ","タ","レ","ソ","ツ","ネ","ナ","ラ","ム","ウ","ヰ","ノ","オ","ク","ヤ","マ","ケ","フ","コ","エ","テ","ア","サ","キ","ユ","メ","ミ","シ","ヱ","ヒ","モ","セ","ス","ン"};
    const int hiragana_size = sizeof(hiragana) / sizeof(hiragana[0]);
    const int katakana_size = sizeof(katakana) / sizeof(katakana[0]);
    const int hira_iroha_size = sizeof(hira_iroha) / sizeof(hira_iroha[0]);
    const int kana_iroha_size = sizeof(kana_iroha) / sizeof(kana_iroha[0]);
}

BoardWidget::BoardWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::BoardWidget),
    dirty(false),
    capturedBlack(0),
    capturedWhite(0),
    color(go::black),
    currentMoveNumber(0),
    showMoveNumber(true),
    showMoveNumberCount(0),
    showCoordinates(true),
    showCoordinatesI(false),
    showMarker(true),
    showBranchMoves(true),
    editMode(eAlternateMove),
    tutorMode(eNoTutor),
    moveToClicked(false),
    rotateBoard_(0),
    flipBoardHorizontally_(false),
    flipBoardVertically_(false),
    playSound(false),
    stoneSound(this),
    playGame(NULL)
{
    m_ui->setupUi(this);

//    stoneSound = Phonon::createPlayer(Phonon::NotificationCategory);

    readSettings();

    setCurrentNode(goData.root);
}

BoardWidget::~BoardWidget()
{
    delete m_ui;
}

/**
*/
void BoardWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

/**
*/
void BoardWidget::paintEvent(QPaintEvent* e){
    QWidget::paintEvent(e);

    QPainter p(this);
    p.drawPixmap(0, 0, width(), height(), offscreenBuffer3);
}

/**
*/
void BoardWidget::mouseReleaseEvent(QMouseEvent* e){
    QWidget::mouseReleaseEvent(e);

    if(e->button() & Qt::LeftButton)
        onLButtonDown(e);
    else if (e->button() & Qt::RightButton)
        onRButtonDown(e);
}

/**
*/
void BoardWidget::mouseMoveEvent(QMouseEvent* e){
    QWidget::mouseMoveEvent(e);

    if (editMode == ePlayGame && color != playGame->yourColor())
        return;

    bool black;
    if (editMode == eAlternateMove || editMode == ePlayGame || tutorMode == eTutorBossSides || tutorMode == eTutorOneSide)
        black = color == go::black;
    else if (editMode == eAddBlack || editMode == eAddWhite)
        black = editMode == eAddBlack;
    else
        return;

    int bx = (int)floor( qreal(e->x() - xlines[0] + boxSize / 2) / boxSize );
    int by = (int)floor( qreal(e->y() - ylines[0] + boxSize / 2) / boxSize );

    offscreenBuffer3 = offscreenBuffer2.copy();
    QPainter p(&offscreenBuffer3);

    if (! (bx < 0 || bx >= xlines.size() || by < 0 || by >= ylines.size() || board[by][bx].color != go::empty) )
        drawStone(p, bx, by, black ? go::black : go::white, 0.5);

    repaint();
}

/**
*/
void BoardWidget::wheelEvent(QWheelEvent* e){
    QWidget::wheelEvent(e);

    if (tutorMode != eNoTutor)
        return;

    go::nodeList::iterator iter = qFind(nodeList.begin(), nodeList.end(), currentNode);
    if (e->delta() > 0){
        if (iter != nodeList.begin() && iter != nodeList.end())
            setCurrentNode( *--iter );
    }
    else{
        if (iter != nodeList.end() && ++iter != nodeList.end())
            setCurrentNode( *iter );
    }
}

void BoardWidget::resizeEvent(QResizeEvent* e){
    QWidget::resizeEvent(e);

    offscreenBuffer1 = QPixmap(e->size());
    repaintBoard();
}

/**
*/
void BoardWidget::onLButtonDown(QMouseEvent* e){
    int boardX = (int)floor( qreal(e->x() - xlines[0] + boxSize / 2) / boxSize );
    int boardY = (int)floor( qreal(e->y() - ylines[0] + boxSize / 2) / boxSize );

    if (boardX < 0 || boardX >= xsize || boardY < 0 || boardY >= ysize)
        return;

    int sgfX, sgfY;
    boardToSgfCoordinate(boardX, boardY, sgfX, sgfY);

    if (editMode == ePlayGame)
        playGameLButtonDown(sgfX, sgfY);
    else if (tutorMode == eTutorBossSides)
        moveNextStone(sgfX, sgfY);
    else if (tutorMode == eTutorOneSide){
        if (moveNextStone(sgfX, sgfY)){
            go::nodeList::iterator iter = qFind(nodeList.begin(), nodeList.end(), currentNode);
            if (iter != nodeList.end() && ++iter != nodeList.end()){
                usleep(500000);
                setCurrentNode(*iter);
            }
        }
    }
    else if (editMode == eAlternateMove){
        if (moveToClicked && board[boardY][boardX].node)
            setCurrentNode( board[boardY][boardX].node );
        else
            addStoneNodeCommand(sgfX, sgfY, boardX, boardY);
    }
    else if (editMode == eCountTerritory){
        addTerritory(boardX, boardY);
        int alive_b=0, alive_w=0, dead_b=0, dead_w=0, bt=0, wt=0;
        getCountTerritory(alive_b, alive_w, dead_b, dead_w, bt, wt);
        emit updateTerritory(alive_b, alive_w, dead_b, dead_w, capturedBlack, capturedWhite, bt, wt, goData.root->komi);
    }
    else
        addMark(sgfX, sgfY, boardX, boardY, e->modifiers() & Qt::ControlModifier);
}

/**
*/
void BoardWidget::onRButtonDown(QMouseEvent*){
    undoStack.undo();
}

/**
* read settings
*/
void BoardWidget::readSettings(){
    QSettings settings;

    // board
    boardType  = settings.value("board/boardType").toInt();
    boardColor = settings.value("board/boardColor", BOARD_COLOR).value<QColor>();
    bgColor    = settings.value("board/bgColor", BG_COLOR).value<QColor>();
    tutorColor = settings.value("board/bgTutorColor", BG_TUTOR_COLOR).value<QColor>();
    if (boardType == 0){
        if (boardImage1.load(":/res/bg.png") == false)
            boardType = 2;
    }
    else if (boardType == 1){
        if (boardImage1.load( settings.value("board/boardPath").toString() ) == false)
            boardType = 2;
    }

    // white stone
    whiteType  = settings.value("board/whiteType").toInt();
    whiteColor = settings.value("board/whiteColor", WHITE_COLOR).value<QColor>();
    if (whiteType == 0){
        if (white1.load(":/res/white_128_ds.png") == false)
            whiteType = 2;
    }
    else{
        if (white1.load( settings.value("board/whitePath").toString() ) == false)
            whiteType = 2;
    }

    // black stone
    blackColor = settings.value("board/blackColor", BLACK_COLOR).value<QColor>();
    blackType  = settings.value("board/blackType").toInt();
    if (blackType == 0){
        if (black1.load(":/res/black_128_ds.png") == false)
            blackType = 2;
    }
    else{
        if (black1.load( settings.value("board/blackPath").toString() ) == false)
            blackType = 2;
    }

    // focus/markers
    focusType = settings.value("marker/focusType").toInt();
    focusWhiteColor = settings.value("marker/focusWhiteColor", FOCUS_WHITE_COLOR).value<QColor>();
    focusBlackColor = settings.value("marker/focusBlackColor", FOCUS_BLACK_COLOR).value<QColor>();
    branchColor = settings.value("marker/branchColor", BRANCH_COLOR).value<QColor>();
    labelType = settings.value("marker/labelType").toInt();

    // sound
    if (settings.value("sound/type").toInt() == 0){
        QStringList soundPathList;
#if defined(Q_WS_MAC)
        soundPathList.push_back( QFileInfo(qApp->applicationDirPath() + "/../Resources/sounds/").absolutePath() );
#endif
        soundPathList.push_back(qApp->applicationDirPath() + "/sounds");
        soundPathList.push_back("/usr/share/" APPNAME "/sounds");
        soundPathList.push_back("/usr/local/share/" APPNAME "/sounds");
        soundPathList.push_back("./sounds");
        QStringList::iterator iter = soundPathList.begin();
        while (iter != soundPathList.end()){
            QFileInfo finfo( *iter + "/stone.wav" );
            if (finfo.exists()){
                setStoneSoundPath(finfo.filePath());
                break;
            }
            ++iter;
        }
    }
    else{
        QFileInfo finfo( settings.value("sound/path").toString() );
        if (finfo.exists())
            setStoneSoundPath(finfo.filePath());
    }
}

/**
*/
void BoardWidget::repaintBoard(bool board, bool stones){
    if (offscreenBuffer1.isNull())
        return;

    if (board)
        paintBoard(&offscreenBuffer1);

    if (stones){
        offscreenBuffer2 = offscreenBuffer1.copy();
        paintStones(&offscreenBuffer2);
        paintTerritories(&offscreenBuffer2);
    }

    offscreenBuffer3 = offscreenBuffer2.copy();

    repaint();
}

/**
*/
void BoardWidget::paintBoard(QPaintDevice* pd){
    QPainter p(pd);
    p.setRenderHints(QPainter::Antialiasing/*|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform*/);

    width_  = pd->width();
    height_ = pd->height();

    paintBoard(p);
}

void BoardWidget::paintBoard(QPainter& p, qreal pointSize){

    QFont font;
    font.setPointSizeF(pointSize);
    font.setStyleHint(QFont::SansSerif);
    font.setWeight(QFont::Normal);
    font.setStyleStrategy(QFont::PreferAntialias);

    p.setFont(font);
    p.setPen(Qt::black);

    if (boardType >= 0){
        if (tutorMode != eNoTutor)
            p.fillRect(0, 0, width_, height_, tutorColor);
        else
            p.fillRect(0, 0, width_, height_, bgColor);
    }

    drawBoard(p);
    drawCoordinates(p);
}

/**
*/
void BoardWidget::paintStones(QPaintDevice* pd){
    QPainter p(pd);
    p.setRenderHints(QPainter::Antialiasing/*|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform*/);

    width_  = pd->width();
    height_ = pd->height();

    paintStones(p);
}

void BoardWidget::paintStones(QPainter& p){
    QFont font;
    font.setStyleHint(QFont::SansSerif);
    font.setWeight(QFont::Normal);
    font.setStyleStrategy(QFont::PreferAntialias);

    p.setFont(font);
    p.setPen(Qt::black);

    drawStonesAndMarker(p);
}

/**
*/
void BoardWidget::paintTerritories(QPaintDevice* pd){
    QPainter p(pd);
    p.setRenderHints(QPainter::Antialiasing/*|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform*/);

    width_  = pd->width();
    height_ = pd->height();

    paintTerritories(p);
}

void BoardWidget::paintTerritories(QPainter& p){
    drawTerritories(p);
}

/**
* print
*/
void BoardWidget::print(QPrinter& printer, int option, int movesPerPage){
    QPainter p;
    p.begin(&printer);

    int  boardType_  = -1;
    int  whiteType_  = -1;
    int  blackType_  = -1;
    bool showNumber_ = true;
    int  moveNumber_ = -1;

    qSwap(boardType, boardType_);
    qSwap(whiteType, whiteType_);
    qSwap(blackType, blackType_);
    qSwap(showMoveNumber, showNumber_);
    qSwap(showMoveNumberCount, moveNumber_);

    width_  = printer.width();
    height_ = printer.height();

    for (int i=0; i<printer.numCopies(); ++i){
        board.clear();
        board.resize(ysize);
        for(int i=0; i<ysize; ++i)
            board[i].resize(xsize);
        BoardBuffer buf = board;

        print( printer, p, option, movesPerPage, buf );
    }

    p.end();

    qSwap(boardType, boardType_);
    qSwap(whiteType, whiteType_);
    qSwap(blackType, blackType_);
    qSwap(showMoveNumber, showNumber_);
    qSwap(showMoveNumberCount, moveNumber_);

    createBoardBuffer();
    repaintBoard();
}

void BoardWidget::print(QPrinter& printer, QPainter& p, int option, int movePerPage, BoardBuffer& buf){
    p.save();

    int moveNumber = 0;
    int moveNumberInPage = 0;
    QString rangai;

    int page = 0;
    int fig = 0;
    newPage(printer, p, moveNumberInPage, buf, page, fig);

    if (option < 3){
        foreach (go::nodePtr node, nodeList){
            print(printer, p, node, page, moveNumber, moveNumberInPage, buf, rangai);

            if (option == 0 && node == currentNode)
                break;
            else if (option == 2 && moveNumberInPage == movePerPage){
                printRangai(printer, p, rangai, page);
                newPage(printer, p, moveNumberInPage, buf, page, fig);
            }
        }
    }
    printRangai(printer, p, rangai, page);

    p.restore();
}

void BoardWidget::print(QPrinter& printer, QPainter& p, go::nodePtr node, int page, int& moveNumber, int& moveNumberInPage, BoardBuffer& buf, QString& rangai){
    p.save();

    QFont font( p.font() );
    if (node->isStone() && !node->isPass()){
        ++moveNumber;
        ++moveNumberInPage;

        if ( !( printer.printRange() == QPrinter::PageRange && (page < printer.fromPage() || page > printer.toPage()) ) ){
            int bx, by;
            sgfToBoardCoordinate(node->position.x, node->position.y, bx, by);
            board[by][bx].color  = node->color;
            board[by][bx].number = moveNumber;
            removeDeadStones(bx, by);

            if (buf[by][bx].empty()){
                drawStone(p, bx, by, node->color);
                buf[by][bx].color  = node->color;
                buf[by][bx].number = moveNumber;

                if (moveNumber < 10)
                    font.setPointSizeF(boxSize * 0.41);
                else if (moveNumber < 99)
                    font.setPointSizeF(boxSize * 0.38);
                else
                    font.setPointSizeF(boxSize * 0.35);

                font.setWeight(QFont::Black);
                p.setFont(font);

                QString s = QString("%1").arg(moveNumber);
                p.setPen( node->isBlack() ? Qt::white : Qt::black );
                p.drawText(xlines[bx] - boxSize, ylines[by] - boxSize, boxSize * 2, boxSize * 2, Qt::AlignCenter, s);
            }
            else{
                QString s = QString( tr("%1(%2)") ).arg(moveNumber).arg( getXYString(bx, by) );
                if (!rangai.isEmpty())
                    rangai.append(", ");
                rangai.append(s);
            }
        }
    }

    p.restore();
}

void BoardWidget::printHeader(QPrinter& printer, QPainter& p, int& /*page*/){
    p.save();

    QFont f(p.font());
    f.setPointSizeF(8.5);
    p.setFont(f);

    QString header = QString("%5 %1 %2 (W) vs %3 %4 (B)").arg(goData.root->whitePlayer).arg(goData.root->whiteRank).arg(goData.root->blackPlayer).arg(goData.root->blackRank).arg(goData.root->gameName);
    QRect r = p.boundingRect(0, 0, printer.width(), 0, Qt::AlignTop|Qt::AlignHCenter, header);
    headerRect.setRect(0, 0, printer.width(), r.height()+5);

    p.drawText(headerRect, Qt::AlignTop|Qt::AlignHCenter, header);

    p.setPen( QPen(Qt::gray, 2) );
    p.drawLine( 0, headerRect.bottom(), printer.width(), headerRect.bottom() );

    p.restore();
}

void BoardWidget::printFooter(QPrinter& printer, QPainter& p, int& page){
    p.save();

    QFont f(p.font());
    f.setPointSizeF(8.5);
    p.setFont(f);

    QDateTime time = QDateTime::currentDateTime();
    QString footer = time.toString(Qt::DefaultLocaleLongDate);
    QRect r = p.boundingRect(0, 0, printer.width(), 0, Qt::AlignBottom|Qt::AlignRight, footer);
    footerRect.setRect(0, printer.height()-r.height()-5, printer.width(), r.height()+5);

    p.drawText(footerRect, Qt::AlignBottom|Qt::AlignRight, footer);

    footer.sprintf("%d", page);
    p.drawText(footerRect, Qt::AlignBottom|Qt::AlignLeft, footer);

    p.setPen( QPen(Qt::gray, 2) );
    p.drawLine(0, footerRect.top(), printer.width(), footerRect.top());

    p.restore();
}

void BoardWidget::printCaption(QPrinter& printer, QPainter& p, int fig){
    p.save();

    QFont f(p.font());
    f.setPointSizeF(14);
    p.setFont(f);

    QString text = QString( tr("Figure %1") ).arg(fig);
    QRect r;
    p.drawText(0, headerRect.bottom() + 20, printer.width(), printer.height(), Qt::AlignHCenter, text, &r);

    p.restore();

    QTransform transform;
    transform.translate(0, r.bottom() + 5);
    p.setTransform(transform);

    height_ -= 25 + r.height();
    if (height_ > width_)
        height_ = width_;
    else
        width_ = height_;
}

void BoardWidget::printRangai(QPrinter& printer, QPainter& p, QString& rangai, int page){
    if (printer.printRange() == QPrinter::PageRange && (page < printer.fromPage() || page > printer.toPage()))
        return;

    p.save();

    QFont f(p.font());
    f.setPointSizeF(12);
    p.setFont(f);

    if (printer.height() > printer.width())
        p.drawText(0, coordinatesRect.bottom() + 10, printer.width(), printer.height(), Qt::TextWordWrap|Qt::AlignHCenter, rangai);
    else
        p.drawText(coordinatesRect.right() + 10, 0, printer.width() - coordinatesRect.right() - 10, coordinatesRect.height(), Qt::TextWordWrap|Qt::AlignHCenter, rangai);

    p.restore();

    rangai.clear();
}

void BoardWidget::newPage(QPrinter& printer, QPainter& p, int& moveNumberInPage, BoardBuffer& buf, int& page, int& fig){
    ++page;
    ++fig;

    if (printer.printRange() == QPrinter::PageRange && (page < printer.fromPage() || page > printer.toPage()))
        return;

    if (page > 1 && page > printer.fromPage())
        printer.newPage();

    p.setTransform( QTransform() );

    printHeader(printer, p, page);
    printFooter(printer, p, page);

    height_ = footerRect.top() - headerRect.bottom();

    printCaption(printer, p, fig);
    paintBoard(p, 15.0);

    moveNumberInPage = 0;
    buf = board;
    for (int y=0; y<board.size(); ++y){
        for (int x=0; x<board[y].size(); ++x){
            if (board[y][x].black())
                drawStone(p, x, y, go::black);
            else if (board[y][x].white())
                drawStone(p, x, y, go::white);
        }
    }
}

/**
*/
void BoardWidget::clear(){
    int xsize = goData.root->xsize;
    int ysize = goData.root->ysize;

    setDirty(false);
    goData.clear();
    goData.root->xsize = xsize;
    goData.root->ysize = ysize;
    nodeList.clear();
    capturedBlack = 0;
    capturedWhite = 0;
    setCurrentNode();
    repaintBoard();
    undoStack.clear();
}

/**
*/
void BoardWidget::getData(go::fileBase& data){
    data.set(goData);
}

/**
*/
void BoardWidget::setData(const go::fileBase& data){
    clear();
    data.get(goData);
    nodeList.clear();
    setCurrentNode();
    repaintBoard();
}

void BoardWidget::insertData(const go::nodePtr node, const go::fileBase& data){
    go::data d;
    data.get(d);

    node->childNodes.push_back( d.root );
    d.root->parent = node;

    createNodeList();
    setDirty(true);
}

/**
*/
void BoardWidget::addStoneNodeCommand(int sgfX, int sgfY){
    if (sgfX == -1 && sgfY == -1){
        go::nodePtr node;
        if (color == go::black)
            node = go::createBlackNode(currentNode);
        else
            node = go::createWhiteNode(currentNode);

        addNodeCommand(currentNode, node);
    }
    else{
        int boardX, boardY;
        sgfToBoardCoordinate(sgfX, sgfY, boardX, boardY);
        addStoneNodeCommand(sgfX, sgfY, boardX, boardY);
    }
}

/**
*/
void BoardWidget::addStoneNodeCommand(int sgfX, int sgfY, int boardX, int boardY){
    if (board[boardY][boardX].empty() == false)
        return;

    if (moveNextStone(sgfX, sgfY))
        return;

    board[boardY][boardX].color = color;
    if (isKill(boardX, boardY) == false && isDead(boardX, boardY) == true){
        board[boardY][boardX].color = go::empty;
        return;
    }

    go::nodePtr n;
    if (color == go::black)
        n = go::createBlackNode(currentNode, sgfX, sgfY);
    else
        n = go::createWhiteNode(currentNode, sgfX, sgfY);

    addNodeCommand(currentNode, n);
}

/**
*/
void BoardWidget::addNodeCommand(go::nodePtr parentNode, go::nodePtr childNode, bool select){
    if (tutorMode != eNoTutor)
        return;

    undoStack.push( new AddNodeCommand(this, parentNode, childNode, select) );
}

/**
*/
void BoardWidget::insertNodeCommand(go::nodePtr parentNode, go::nodePtr childNode, bool select){
    if (tutorMode != eNoTutor)
        return;

    undoStack.push( new InsertNodeCommand(this, parentNode, childNode, select) );
}

/**
*/
void BoardWidget::deleteNodeCommand(go::nodePtr node, bool deleteChildren){
    if (tutorMode != eNoTutor)
        return;
    else if( node == goData.root )
        return;

    undoStack.push( new DeleteNodeCommand(this, node, deleteChildren) );
}

/**
*/
void BoardWidget::setMoveNumberCommand(go::nodePtr node, int moveNumber){
    if( node->moveNumber == moveNumber )
        return;

    undoStack.push( new SetMoveNumberCommand(this, node, moveNumber) );
}

/**
*/
void BoardWidget::unsetMoveNumberCommand(go::nodePtr node){
    if( node->moveNumber == -1 )
        return;

    undoStack.push( new UnsetMoveNumberCommand(this, node) );
}

/**
*/
void BoardWidget::setNodeNameCommand(go::nodePtr node, const QString& nodeName){
    if( node->name == nodeName)
        return;

    undoStack.push( new SetNodeNameCommand(this, node, nodeName) );
}

/**
*/
void BoardWidget::setCommentCommand(go::nodePtr node, const QString& comment){
    if( node->comment == comment)
        return;

//    undoStack.push( new SetCommentCommand(this, node, comment) );
    node->comment = comment;
    modifyNode(node);
}

void BoardWidget::rotateSgfCommand(){
    RotateSgfCommand* command = new RotateSgfCommand(this, tr("Rotate SGF"));
    rotateSgf(goData.root, command);
    undoStack.push(command);

    setDirty(true);
}

void BoardWidget::flipSgfHorizontallyCommand(){
    RotateSgfCommand* command = new RotateSgfCommand(this, tr("Flip SGF Horizontally"));
    flipSgf(goData.root, goData.root->xsize, 0, command);
    undoStack.push(command);

    setDirty(true);
}

void BoardWidget::flipSgfVerticallyCommand(){
    RotateSgfCommand* command = new RotateSgfCommand(this, tr("Flip SGF Vertically"));
    flipSgf(goData.root, 0, goData.root->ysize, command);
    undoStack.push(command);

    setDirty(true);
}

/**
*/
void BoardWidget::addNode(go::nodePtr parent, go::nodePtr node, bool select){
    parent->childNodes.push_back(node);
    node->parent = parent;

    setDirty(true);
    emit nodeAdded(parent, node, select);

    if (select)
        setCurrentNode(node);
}

/**
*/
void BoardWidget::deleteNode(go::nodePtr node, bool deleteChildren){
    if( node == goData.root )
        return;

    go::nodePtr parent = node->parent;
    if (parent){
        go::nodeList::iterator iter = qFind(parent->childNodes.begin(), parent->childNodes.end(), node);
        if (iter != parent->childNodes.end())
            parent->childNodes.erase(iter);

        if (deleteChildren == false){
            parent->childNodes += node->childNodes;
            go::nodeList::iterator iter = node->childNodes.begin();
            while (iter != node->childNodes.end()){
                (*iter)->parent = parent;
                ++iter;
            }
        }
    }

    setCurrentNode(parent);

    setDirty(true);
    emit nodeDeleted(node, deleteChildren);

    createNodeList();
}

/**
*/
void BoardWidget::modifyNode(go::nodePtr node, bool recreateBoardBuffer){
    if (recreateBoardBuffer)
        createBoardBuffer();
    repaintBoard(false);
    setDirty(true);
    emit nodeModified(node);
}

/**
*/
void BoardWidget::pass(){
    if (editMode == ePlayGame){
        if (color == playGame->yourColor())
            playGame->move(-1, -1);
    }
    else
        addStoneNodeCommand(-1, -1);
}

/**
* public slot
*/
void BoardWidget::setCurrentNode(go::nodePtr node){
    if (editMode == eCountTerritory)
        return;

    if (node == NULL)
        node = goData.root;

    if (currentNode == node && !nodeList.empty())
        return;

    currentNode  = node;
    go::nodeList::iterator iter = qFind(nodeList.begin(), nodeList.end(), node);
    if (iter == nodeList.end())
        createNodeList();

    createBoardBuffer();

    if (playSound && node->isStone())
        stoneSound.play();

    repaintBoard(false);
    emit currentNodeChanged(currentNode);
}

/**
*/
void BoardWidget::playGameLButtonDown(int sgfX, int sgfY){
    if (color == playGame->yourColor())
        playGame->move(sgfX, sgfY);
}

void BoardWidget::setBoardSize(int xsize, int ysize){
    clear();
    goData.root->xsize = xsize;
    goData.root->ysize = ysize;
    createBoardBuffer();
    setCurrentNode();

    repaintBoard();
}

void BoardWidget::rotateSgf(go::nodePtr node, QUndoCommand* command){
    go::point p = node->position;
    p.x = goData.root->ysize - node->position.y - 1;
    p.y = node->position.x;
    new MovePositionCommand(this, node, p, command);

    go::nodeList::iterator iter = node->childNodes.begin();
    while (iter != node->childNodes.end()){
        rotateSgf(*iter, command);
        ++iter;
    }

    rotateStoneSgf(node, node->emptyStones, command);
    rotateStoneSgf(node, node->blackStones, command);
    rotateStoneSgf(node, node->whiteStones, command);
    rotateMarkSgf(node, node->crosses, command);
    rotateMarkSgf(node, node->squares, command);
    rotateMarkSgf(node, node->triangles, command);
    rotateMarkSgf(node, node->circles, command);
    rotateMarkSgf(node, node->characters, command);
    rotateMarkSgf(node, node->blackTerritories, command);
    rotateMarkSgf(node, node->whiteTerritories, command);
}

void BoardWidget::rotateStoneSgf(go::nodePtr node, go::stoneList& stoneList, QUndoCommand* command){
    go::stoneList::iterator iter = stoneList.begin();
    while (iter != stoneList.end()){
        go::point p = iter->p;
        p.x = goData.root->ysize - iter->p.y - 1;
        p.y = iter->p.x;

        new MoveStoneCommand(this, node, &(*iter), p, command);

        ++iter;
    }
}

void BoardWidget::rotateMarkSgf(go::nodePtr node, go::markList& markList, QUndoCommand* command){
    go::markList::iterator iter = markList.begin();
    while (iter != markList.end()){
        go::point p = iter->p;
        p.x = goData.root->ysize - iter->p.y - 1;
        p.y = iter->p.x;

        new MoveMarkCommand(this, node, &(*iter), p, command);

        ++iter;
    }
}

void BoardWidget::flipSgf(go::nodePtr node, int xsize, int ysize, QUndoCommand* command){
    go::point p = node->position;
    if (xsize)
        p.x = xsize - p.x - 1;
    if (ysize)
        p.y = ysize - p.y - 1;
    new MovePositionCommand(this, node, p, command);

    go::nodeList::iterator iter = node->childNodes.begin();
    while (iter != node->childNodes.end()){
        flipSgf(*iter, xsize, ysize, command);
        ++iter;
    }

    flipStoneSgf(node, node->emptyStones, xsize, ysize, command);
    flipStoneSgf(node, node->blackStones, xsize, ysize, command);
    flipStoneSgf(node, node->whiteStones, xsize, ysize, command);
    flipMarkSgf(node, node->crosses, xsize, ysize, command);
    flipMarkSgf(node, node->squares, xsize, ysize, command);
    flipMarkSgf(node, node->triangles, xsize, ysize, command);
    flipMarkSgf(node, node->circles, xsize, ysize, command);
    flipMarkSgf(node, node->characters, xsize, ysize, command);
    flipMarkSgf(node, node->blackTerritories, xsize, ysize, command);
    flipMarkSgf(node, node->whiteTerritories, xsize, ysize, command);
}

void BoardWidget::flipStoneSgf(go::nodePtr node, go::stoneList& stoneList, int xsize, int ysize, QUndoCommand* command){
    go::stoneList::iterator iter = stoneList.begin();
    while (iter != stoneList.end()){
        go::point pos = iter->p;
        if (xsize)
            pos.x = xsize - pos.x - 1;
        if (ysize)
            pos.y = ysize - pos.y - 1;

        new MoveStoneCommand(this, node, &(*iter), pos, command);

        ++iter;
    }
}

void BoardWidget::flipMarkSgf(go::nodePtr node, go::markList& markList, int xsize, int ysize, QUndoCommand* command){
    go::markList::iterator iter = markList.begin();
    while (iter != markList.end()){
        go::point pos = iter->p;
        if (xsize)
            pos.x = xsize - pos.x - 1;
        if (ysize)
            pos.y = ysize - pos.y - 1;

        new MoveMarkCommand(this, node, &(*iter), pos, command);

        ++iter;
    }
}

int  BoardWidget::rotateBoard(){
    if (++rotateBoard_ > 3)
        rotateBoard_ = 0;

    createBoardBuffer();
    repaintBoard();

    return rotateBoard_;
}

void BoardWidget::flipBoardHorizontally(bool flip){
    flipBoardHorizontally_ = flip;

    createBoardBuffer();
    repaintBoard();
}

void BoardWidget::flipBoardVertically(bool flip){
    flipBoardVertically_ = flip;

    createBoardBuffer();
    repaintBoard();
}

void BoardWidget::resetBoard(){
    rotateBoard_ = 0;
    flipBoardHorizontally_ = false;
    flipBoardVertically_ = false;

    createBoardBuffer();
    repaintBoard();
}

/**
*/
go::nodePtr BoardWidget::findNodeFromMoveNumber(int moveNumber){
    go::nodeList::iterator iter = nodeList.begin();

    int number = 0;
    while (iter != nodeList.end()){
        if ((*iter)->isStone() && ++number == moveNumber)
            return *iter;
        ++iter;
    }

    return go::nodePtr();
}

/**
*/
void BoardWidget::createNodeList(){
    nodeList.clear();

    go::nodePtr node = currentNode;
    while ((node = node->parent) != NULL)
        nodeList.push_front(node);

    nodeList.push_back( node = currentNode );
    while (!node->childNodes.empty()){
        node = node->childNodes.front();
        nodeList.push_back(node);
    }
}

/**
*/
void BoardWidget::createBoardBuffer(){
    xsize = (rotateBoard_ == 0 || rotateBoard_ == 2) ? goData.root->xsize : goData.root->ysize;
    ysize = (rotateBoard_ == 0 || rotateBoard_ == 2) ? goData.root->ysize : goData.root->xsize;

    capturedBlack = 0;
    capturedWhite = 0;

    board.clear();
    board.resize(ysize);
    for (int i=0; i<ysize; ++i)
        board[i].resize(xsize);

    currentMoveNumber = 0;
    go::nodeList::iterator iter = nodeList.begin();
    while (iter != nodeList.end()){
        if ((*iter)->moveNumber > 0)
            currentMoveNumber = (*iter)->moveNumber;
        else if ((*iter)->isStone())
            ++currentMoveNumber;
        putStone(*iter, currentMoveNumber);

        if (*iter == currentNode)
            break;

        ++iter;
    }

    go::markList::iterator iter2 = currentNode->blackTerritories.begin();
    while (iter2 != currentNode->blackTerritories.end()){
        int boardX, boardY;
        sgfToBoardCoordinate(iter2->p.x, iter2->p.y, boardX, boardY);
        if (boardX >= 0 && boardX < xsize && boardY >= 0 && boardY < ysize){
            board[boardY][boardX].color |= go::blackTerritory;
        }
        ++iter2;
    }

    iter2 = currentNode->whiteTerritories.begin();
    while (iter2 != currentNode->whiteTerritories.end()){
        int boardX, boardY;
        sgfToBoardCoordinate(iter2->p.x, iter2->p.y, boardX, boardY);
        if (boardX >= 0 && boardX < xsize && boardY >= 0 && boardY < ysize){
            board[boardY][boardX].color |= go::whiteTerritory;
        }
        ++iter2;
    }
}

/**
*/
void BoardWidget::drawBoard(QPainter& p){
    p.save();

    QRectF coordRect = p.boundingRect(QRectF(0.0, 0.0, 1.0, 1.0), Qt::AlignCenter, "999999");
    int w = static_cast<int>( (width_  - (showCoordinates ? coordRect.width() : 0.0)) / xsize );
    int h = static_cast<int>( (height_ - (showCoordinates ? coordRect.width() : 0.0)) / ysize );
    boxSize = qMin(w, h);
    w = boxSize * (xsize - 1);
    h = boxSize * (ysize - 1);
    int margin = int(boxSize * 0.5);

    int l = (width_ - w) / 2;
    int r = l + boxSize * (xsize - 1);
    int t = (height_ - h) / 2;
    int b = t + boxSize * (ysize - 1);

    // create board and stone image
    boardRect.setRect(l - margin, t - margin, w + margin * 2, h + margin * 2);
    coordinatesRect = boardRect;
    if (boardType >= 0){
        boardImage2 = QImage(boardRect.size(), QImage::Format_RGB32);
        QPainter board(&boardImage2);
        if (boardType == 0 || boardType == 1)
            board.fillRect(0, 0, boardRect.width(), boardRect.height(), QBrush(boardImage1));
        else
            board.fillRect(0, 0, boardRect.width(), boardRect.height(), boardColor);
    }

    if (blackType == 0 || blackType == 1)
        black2 = black1.scaled(boxSize, boxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else{
        black2 = QImage(boxSize, boxSize, QImage::Format_ARGB32);
        black2.fill(0);
        QPainter p2(&black2);
        p2.setRenderHints(QPainter::Antialiasing/*|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform*/);
        p2.setPen(Qt::black);
        p2.setBrush(blackColor);
        p2.drawEllipse(1, 1, boxSize-2, boxSize-2);
    }

    if (whiteType == 0 || whiteType == 1)
        white2 = white1.scaled(boxSize, boxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else{
        white2 = QImage(boxSize, boxSize, QImage::Format_ARGB32);
        white2.fill(0);
        QPainter p2(&white2);
        p2.setRenderHints(QPainter::Antialiasing/*|QPainter::TextAntialiasing|QPainter::SmoothPixmapTransform*/);
        p2.setPen(Qt::black);
        p2.setBrush(whiteColor);
        p2.drawEllipse(1, 1, boxSize-2, boxSize-2);
    }

    if (boardType >= 0){
        p.fillRect(boardRect.left()+3, boardRect.top()+3, boardRect.width(), boardRect.height(), QColor(10, 10, 10, 120));
        p.drawImage(boardRect.topLeft(), boardImage2);
    }

    // horizontal line
    ylines.clear();
    for (int i=0; i<ysize; ++i){
//        pen.setWidth( i == 0 || i == ysize-1 ? 2 : 1 );
//        p.setPen(pen);

        int y = t + i * boxSize;
        p.drawLine(l, y, r, y);
        ylines.push_back(y);
    }

    // vertical line
    xlines.clear();
    for (int i=0; i<xsize; ++i){
//        pen.setWidth( i == 0 || i == xsize-1 ? 2 : 1 );
//        p.setPen(pen);

        int x = l + i * boxSize;
        p.drawLine(x, t, x, b);
        xlines.push_back(x);
    }

    // draw stars
    QList<int> xstar, ystar;
    getStartPosition(xstar, xsize);
    getStartPosition(ystar, ysize);
    for (int y=0; y<ystar.size(); ++y){
        for (int x=0; x<xstar.size(); ++x){
            int cx = xlines[ xstar[x] ];
            int cy = ylines[ ystar[y] ];

            QPainterPath path;
            path.addEllipse(cx-3, cy-3, 6, 6);
            p.fillPath(path, QBrush(Qt::black));
        }
    }

    p.restore();
}

void BoardWidget::getStartPosition(QList<int>& star, int size){
    if (size >= 7 && size <= 9){
        star.push_back(2);
        star.push_back(size-3);
    }
    else if (size > 9){
        star.push_back(3);
        star.push_back(size-4);
        if (size % 2)
            star.push_back(size / 2);
    }
}

/**
*/
void BoardWidget::drawCoordinates(QPainter& p){
    if (showCoordinates == false)
        return;

    p.save();

    QRect r = p.boundingRect(0, 0, 1, 1, Qt::AlignCenter, "999");

    for (int i=0; i<xsize; ++i){
        QString s;
        if (rotateBoard_ == 0)
            s = getXString(flipBoardHorizontally_ ? xsize-i-1 : i);
        else if (rotateBoard_ == 1)
            s = getYString(flipBoardHorizontally_ ? i : xsize-i-1);
        else if (rotateBoard_ == 2)
            s = getXString(flipBoardHorizontally_ ? i : xsize-i-1);
        else if (rotateBoard_ == 3)
            s = getYString(flipBoardHorizontally_ ? xsize-i-1 : i);

        p.drawText(QRectF(xlines[i]-r.width()/2.0, boardRect.top()-r.height(), r.width(), r.height()), Qt::AlignCenter, s);
        p.drawText(QRectF(xlines[i]-r.width()/2.0, boardRect.bottom()+5, r.width(), r.height()), Qt::AlignCenter, s);
    }
    coordinatesRect.setTop( boardRect.top() - r.height() );
    coordinatesRect.setBottom( boardRect.bottom() + 5 + r.height() );

    for (int i=0; i<ysize; ++i){
        QString s;
        if (rotateBoard_ == 0)
            s = getYString(flipBoardVertically_ ? ysize-i-1 : i);
        else if (rotateBoard_ == 1)
            s = getXString(flipBoardVertically_ ? ysize-i-1 : i);
        else if (rotateBoard_ == 2)
            s = getYString(flipBoardVertically_ ? i : ysize-i-1);
        else if (rotateBoard_ == 3)
            s = getXString(flipBoardVertically_ ? i : ysize-i-1);

        p.drawText(QRectF(boardRect.left()-r.width(), ylines[i]-r.height()/2.0, r.width(), r.height()), Qt::AlignCenter, s);
        p.drawText(QRectF(boardRect.right()+3, ylines[i]-r.height()/2.0, r.width(), r.height()), Qt::AlignCenter, s);
    }
    coordinatesRect.setLeft( boardRect.left() - r.width() );
    coordinatesRect.setRight( boardRect.right() + 3 + r.width() );

    p.restore();
}

/**
*/
void BoardWidget::drawStonesAndMarker(QPainter& p){
    p.save();

    QFont font(p.font());
    font.setPointSizeF(boxSize * 0.5);
    p.setFont(font);

    drawStones(p);

    if (currentNode->childNodes.size() > 1)
        drawBranchMoves(p, currentNode->childNodes.begin(), currentNode->childNodes.end());

    drawCross(p, currentNode->crosses.begin(), currentNode->crosses.end());
    drawTriangle(p, currentNode->triangles.begin(), currentNode->triangles.end());
    drawCircle(p, currentNode->circles.begin(), currentNode->circles.end());
    drawSquare(p, currentNode->squares.begin(), currentNode->squares.end());
    drawCharacter(p, currentNode->characters.begin(), currentNode->characters.end());
//    drawTerritories(p, currentNode->blackTerritories.begin(), currentNode->blackTerritories.end());
//    drawTerritories(p, currentNode->whiteTerritories.begin(), currentNode->whiteTerritories.end());

    if (showMoveNumber && showMoveNumberCount == 0)
        if (currentNode->isStone())
            drawCurrentMark(p, currentNode);

    p.restore();
}

void BoardWidget::drawStones(QPainter& p){
    p.save();

    QFont font( p.font() );

    for (int y=0; y<board.size(); ++y){
        for (int x=0; x<board[y].size(); ++x){
            if (board[y][x].empty())
                continue;

            // draw stone
            if (board[y][x].black())
                drawStone(p, x, y, go::black, board[y][x].whiteTerritory() ? 0.6 : 1.0);
            else if (board[y][x].white())
                drawStone(p, x, y, go::white, board[y][x].blackTerritory() ? 0.6 : 1.0);

            // draw move number
            if (showMoveNumber == false || showMoveNumberCount == 0 || board[y][x].number == 0 || (showMoveNumberCount != -1 && currentMoveNumber - showMoveNumberCount + 1 > board[y][x].number))
                continue;

            if (board[y][x].number < 10)
                font.setPointSizeF(boxSize * 0.41);
            else if (board[y][x].number < 99)
                font.setPointSizeF(boxSize * 0.38);
            else
                font.setPointSizeF(boxSize * 0.35);

            font.setWeight(board[y][x].number == currentMoveNumber ? QFont::Black: QFont::Normal);
            p.setFont(font);

            QString s = QString("%1").arg(board[y][x].number);
            p.setPen( board[y][x].number == currentMoveNumber ? Qt::red : board[y][x].black() ? Qt::white : Qt::black );
            p.drawText(QRectF(xlines[x] - boxSize * 0.5, ylines[y] - boxSize * 0.5, boxSize, boxSize), Qt::AlignCenter, s);
        }
    }

    p.restore();
}

/**
*/
void BoardWidget::drawBranchMoves(QPainter& p, go::nodeList::iterator first, go::nodeList::iterator last){
    if (showBranchMoves == false)
        return;

    p.save();
    p.setPen( branchColor );

    char s[] = "A";
    while (first != last){
        int boardX, boardY;
        sgfToBoardCoordinate((*first)->getX(), (*first)->getY(), boardX, boardY);
        if (boardX >= 0 && boardX < xsize && boardY >= 0 && boardY < ysize){
            eraseBackground(p, boardX, boardY);
            p.drawText(xlines[boardX] - boxSize, ylines[boardY] - boxSize, boxSize * 2, boxSize * 2, Qt::AlignCenter, s);
            ++s[0];
        }
        ++first;
    }

    p.restore();
}

/**
*/
void BoardWidget::drawCross(QPainter& p, go::markList::iterator first, go::markList::iterator last){
    QPainterPath path = createCrossPath();
    drawMark(p, path, first, last);
}

/**
*/
void BoardWidget::drawTriangle(QPainter& p, go::markList::iterator first, go::markList::iterator last){
    QPainterPath path = createTrianglePath();
    drawMark(p, path, first, last);
}

/**
*/
void BoardWidget::drawCircle(QPainter& p, go::markList::iterator first, go::markList::iterator last){
    QPainterPath path = createCirclePath();
    drawMark(p, path, first, last);
}

/**
*/
void BoardWidget::drawSquare(QPainter& p, go::markList::iterator first, go::markList::iterator last){
    QPainterPath path = createSquarePath();
    drawMark(p, path, first, last);
}

void BoardWidget::drawCharacter(QPainter& p, go::markList::iterator first, go::markList::iterator last){
    if (showMarker == false)
        return;

    p.save();

    QFont font( p.font() );
    font.setWeight(QFont::Black);
    p.setFont(font);

    while (first != last){
        int boardX, boardY;
        sgfToBoardCoordinate(first->p.x, first->p.y, boardX, boardY);

        if (boardX >= 0 && boardX < xsize && boardY >= 0 && boardY < ysize){
            int x = xlines[boardX];
            int y = ylines[boardY];

            stoneInfo& info = board[boardY][boardX];
            if (info.empty()){
                p.setPen( Qt::black );
                eraseBackground(p, boardX, boardY);
            }
            else
                p.setPen( info.black() ? Qt::white : Qt::black );

            p.drawText(x-boxSize, y-boxSize, boxSize*2, boxSize*2, Qt::AlignCenter, first->s);
        }
        ++first;
    }

    p.restore();
}

/**
*/
void BoardWidget::drawMark(QPainter& p, const QPainterPath& path, go::markList::iterator first, go::markList::iterator last){
    if (showMarker == false)
        return;

    while (first != last){
        int boardX, boardY;
        sgfToBoardCoordinate(first->p.x, first->p.y, boardX, boardY);

        if (boardX >= 0 && boardX < xsize && boardY >= 0 && boardY < ysize)
            drawPath(p, path, boardX, boardY);
        ++first;
    }
}

/**
*/
void BoardWidget::drawPath(QPainter& p, const QPainterPath& path, int boardX, int boardY){
    p.save();

    QColor color;
    stoneInfo& info = board[boardY][boardX];
    if (info.empty()){
        p.setPen( QPen(Qt::black, 2) );
        eraseBackground(p, boardX, boardY);
    }
    else
        p.setPen( info.black() ? QPen(Qt::white, 2) : QPen(Qt::black, 2) );

    int x = xlines[boardX];
    int y = ylines[boardY];
    p.translate(x, y);

    p.drawPath(path);

    p.restore();
}

/**
*/
void BoardWidget::drawTerritories(QPainter& p){
    p.save();

    qreal w = boxSize * 0.3;

    for (int by=0; by<ysize; ++by){
        for (int bx=0; bx<xsize; ++bx){
            if (!board[by][bx].blackTerritory() && !board[by][bx].whiteTerritory())
                continue;

            int x = xlines[bx];
            int y = ylines[by];

            QColor color = board[by][bx].whiteTerritory() ? QColor(255, 255, 255, 105) : QColor(0, 0, 0, 45);
            p.fillRect(x-boxSize/2, y-boxSize/2, boxSize, boxSize, color);

            color.setAlpha(255);
            p.fillRect( QRectF(x-w/2, y-w/2, w, w), color);
        }
    }

    p.restore();
}

/**
*/
void BoardWidget::drawCurrentMark(QPainter& p, go::nodePtr node){
    if (node->isPass())
        return;

    p.save();

    QBrush brush;
    if (node->isBlack()){
        p.setPen( QPen(focusBlackColor, 2) );
        brush = QBrush(focusBlackColor);
    }
    else{
        p.setPen( QPen(focusWhiteColor, 2) );
        brush = QBrush(focusWhiteColor);
    }

    int boardX, boardY;
    sgfToBoardCoordinate(node->getX(), node->getY(), boardX, boardY);
    int x = xlines[boardX];
    int y = ylines[boardY];
    p.translate(x, y);

    if (focusType == 0){
        QPainterPath path = createFocusTrianglePath();
        p.fillPath(path, brush);
    }
    else if (focusType == 1){
        QPainterPath path = createCirclePath();
        p.drawPath(path);
    }
    else if (focusType == 2){
        QPainterPath path = createCrossPath();
        p.drawPath(path);
    }
    else if (focusType == 3){
        QPainterPath path = createSquarePath();
        p.drawPath(path);
    }
    else if (focusType == 4){
        QPainterPath path = createTrianglePath();
        p.drawPath(path);
    }

    p.restore();
}

QPainterPath BoardWidget::createFocusTrianglePath() const{
    QPainterPath path;
    qreal w = boxSize * 0.25;
    qreal h = boxSize * 0.15;
    QPolygonF polygon(3);
    polygon[0] = QPointF(0, -h);
    polygon[1] = QPointF(-w, h);
    polygon[2] = QPointF(w, h);
    path.addPolygon(polygon);
    path.closeSubpath();
    return path;
}

QPainterPath BoardWidget::createCirclePath() const{
    QPainterPath path;
    qreal w = boxSize * 0.42;
    path.addEllipse(-w/2, -w/2, w, w);
    return path;
}

QPainterPath BoardWidget::createCrossPath() const{
    QPainterPath path;
    qreal w = boxSize * 0.18;

    path.moveTo(-w, -w);
    path.lineTo(w, w);

    path.moveTo(w, -w);
    path.lineTo(-w, w);

    return path;
}

QPainterPath BoardWidget::createSquarePath() const{
    QPainterPath path;
    qreal w = boxSize * 0.4;
    path.addRect(-w/2, -w/2, w, w);
    return path;
}

QPainterPath BoardWidget::createTrianglePath() const{
    QPainterPath path;
    qreal w = boxSize * 0.22;
    qreal h = boxSize * 0.18;
    QPolygonF polygon(3);
    polygon[0] = QPointF(0, -h);
    polygon[1] = QPointF(-w, h);
    polygon[2] = QPointF(w, h);
    path.addPolygon(polygon);
    path.closeSubpath();
    return path;
}

/**
*/
void BoardWidget::drawStone(QPainter& p, int bx, int by, go::color color, qreal opacity){
    p.save();

    p.translate(xlines[bx], ylines[by]);
    if (color == go::black){
        if (blackType >= 0){
            p.setOpacity(opacity);
            p.drawImage(-boxSize/2, -boxSize/2, black2);
        }
        else{
            p.setPen(Qt::black);
            p.setBrush(blackColor);
            p.drawEllipse(-boxSize/2, -boxSize/2, boxSize-2, boxSize-2);
        }
    }
    else{
        if (whiteType >= 0){
            p.setOpacity(opacity);
            p.drawImage(-boxSize/2, -boxSize/2, white2);
        }
        else{
            p.setPen(Qt::black);
            p.setBrush(whiteColor);
            p.drawEllipse(-boxSize/2, -boxSize/2, boxSize-2, boxSize-2);
        }
    }

    p.restore();
}

/**
*/
void BoardWidget::eraseBackground(QPainter& p, int x, int y){
    int dx = xlines[x] - boxSize / 2;
    int dy = ylines[y] - boxSize / 2;
    p.drawImage(dx, dy, boardImage2, dx - boardRect.left(), dy - boardRect.top(), boxSize, boxSize);
}

/**
*/
void BoardWidget::putStone(go::nodePtr node, int moveNumber){
    color = node->isWhite() ? go::black : go::white;

    if (node->isStone()){
        int boardX, boardY;
        sgfToBoardCoordinate(node->getX(), node->getY(), boardX, boardY);

        if (boardX >= 0 && boardX < xsize && boardY >= 0 && boardY < ysize){
            board[boardY][boardX].color  = node->isBlack() ? go::black : go::white;
            board[boardY][boardX].number = moveNumber;
            board[boardY][boardX].node   = node;
            removeDeadStones(boardX, boardY);
        }
    }

    go::stoneList stones;
    stones << node->emptyStones << node->blackStones << node->whiteStones;
    go::stoneList::iterator iter = stones.begin();
    while (iter != stones.end()){
        int boardX, boardY;
        sgfToBoardCoordinate(iter->p.x, iter->p.y, boardX, boardY);
        if (boardX >= 0 && boardX < xsize && boardY >= 0 && boardY < ysize){
            board[boardY][boardX].color  = iter->c;
            board[boardY][boardX].number = 0;
            board[boardY][boardX].node   = node;
            removeDeadStones(boardX, boardY);
        }
        ++iter;
    }
}

/**
*/
void BoardWidget::removeDeadStones(int x, int y){
    int c = board[y][x].color == go::black ? go::white : go::black;

    int* tmp = new int[xsize * ysize];

    memset(tmp, 0, sizeof(int) * xsize * ysize);
    if (y > 0 && board[y-1][x].color == c && isDead(tmp, c, x, y - 1))
        dead(tmp);

    memset(tmp, 0, sizeof(int) * xsize * ysize);
    if (y < ysize-1 && board[y+1][x].color == c && isDead(tmp, c, x, y + 1))
        dead(tmp);

    memset(tmp, 0, sizeof(int) * xsize * ysize);
    if (x > 0 && board[y][x-1].color == c && isDead(tmp, c, x - 1, y))
        dead(tmp);

    memset(tmp, 0, sizeof(int) * xsize * ysize);
    if (x < xsize-1 && board[y][x+1].color == c && isDead(tmp, c, x + 1, y))
        dead(tmp);

    delete[] tmp;
}

/**
*/
bool BoardWidget::isDead(int* tmp, int c, int x, int y){
    if (tmp[y*xsize+x])
        return true;
    else if (board[y][x].empty())
        return false;
    else if ((board[y][x].color & c) == 0)
        return true;
    tmp[y*xsize+x] = board[y][x].color;

    if (y > 0 && !isDead(tmp, c, x, y - 1))
        return false;

    if (y < ysize-1 && !isDead(tmp, c, x, y + 1))
        return false;

    if (x > 0 && !isDead(tmp, c, x - 1, y))
        return false;

    if (x < xsize-1 && !isDead(tmp, c, x + 1, y))
        return false;

    return true;
}

/**
*/
bool BoardWidget::isDead(int x, int y){
    int size = xsize * ysize;
    int* tmp = new int[size];
    memset(tmp, 0, sizeof(int)*size);

    int c = board[y][x].color;
    bool dead = isDead(tmp, c, x, y);

    delete[] tmp;

    return dead;
}

/**
*/
bool BoardWidget::isKill(int x, int y){
    int* tmp = new int[xsize * ysize];

    go::color c = board[y][x].color == go::black ? go::white : go::black;

    memset(tmp, 0, sizeof(int)* xsize * ysize);
    bool dead = (y > 0 && board[y-1][x].color == c && isDead(tmp, c, x, y - 1));

    memset(tmp, 0, sizeof(int)* xsize * ysize);
    dead = !dead ? (y < ysize-1 && board[y+1][x].color == c && isDead(tmp, c, x, y + 1)) : true;

    memset(tmp, 0, sizeof(int)* xsize * ysize);
    dead = !dead ? (x > 0 && board[y][x-1].color == c && isDead(tmp, c, x - 1, y)) : true;

    memset(tmp, 0, sizeof(int)* xsize * ysize);
    dead = !dead ? (x < xsize-1 && board[y][x+1].color == c && isDead(tmp, c, x + 1, y)) : true;

    delete[] tmp;

    return dead;
}

/**
*/
void BoardWidget::dead(int* tmp){
    for (int y=0; y<ysize; ++y){
        for (int x=0; x<xsize; ++x){
            if (tmp[y*xsize+x]){
                if (board[y][x].color == go::black)
                    ++capturedBlack;

                if (board[y][x].color == go::white)
                    ++capturedWhite;

                board[y][x].color = go::empty;
                board[y][x].node.reset();
            }
        }
    }
}

/**
*/
bool BoardWidget::moveNextStone(int sgfX, int sgfY){
    go::nodeList::iterator iter = currentNode->childNodes.begin();
    while (iter != currentNode->childNodes.end()){
        if ((*iter)->getX() == sgfX && (*iter)->getY() == sgfY){
            setCurrentNode(*iter);
            return true;
        }
        ++iter;
    }

    return false;
}

/**
*/
void BoardWidget::addMark(int sgfX, int sgfY, int boardX, int boardY, bool ctrl){
    switch (editMode){
        case eAlternateMove:
            return;

        case eAddBlack:
            addStone(currentNode, go::point(sgfX, sgfY), go::point(boardX, boardY), go::black);
            break;

        case eAddWhite:
            addStone(currentNode, go::point(sgfX, sgfY), go::point(boardX, boardY), go::white);
            break;

        case eAddEmpty:
            addEmpty(currentNode, go::point(sgfX, sgfY), go::point(boardX, boardY));
            break;

        case eLabelMark:
        case eManualMark:{
            go::point p(sgfX, sgfY);
            removeMark(currentNode->circles, p);
            removeMark(currentNode->crosses, p);
            removeMark(currentNode->squares, p);
            removeMark(currentNode->triangles, p);
            if (editMode == eLabelMark && !ctrl)
                addCharacter(currentNode->characters, p);
            else
                addManualEntry(currentNode->characters, p);
            modifyNode(currentNode);
            break;
        }

        case eCircleMark:{
            go::mark mark(sgfX, sgfY, go::mark::eCircle);
            addMark(currentNode->circles, mark);
            removeMark(currentNode->crosses, mark.p);
            removeMark(currentNode->squares, mark.p);
            removeMark(currentNode->triangles, mark.p);
            removeMark(currentNode->characters, mark.p);
            modifyNode(currentNode);
            break;
        }

        case eCrossMark:{
            go::mark mark(sgfX, sgfY, go::mark::eCross);
            removeMark(currentNode->circles, mark.p);
            addMark(currentNode->crosses, mark);
            removeMark(currentNode->squares, mark.p);
            removeMark(currentNode->triangles, mark.p);
            removeMark(currentNode->characters, mark.p);
            modifyNode(currentNode);
            break;
        }

        case eSquareMark:{
            go::mark mark(sgfX, sgfY, go::mark::eSquare);
            removeMark(currentNode->circles, mark.p);
            removeMark(currentNode->crosses, mark.p);
            addMark(currentNode->squares, mark);
            removeMark(currentNode->triangles, mark.p);
            removeMark(currentNode->characters, mark.p);
            modifyNode(currentNode);
            break;
        }

        case eTriangleMark:{
            go::mark mark(sgfX, sgfY, go::mark::eTriangle);
            removeMark(currentNode->circles, mark.p);
            removeMark(currentNode->crosses, mark.p);
            removeMark(currentNode->squares, mark.p);
            addMark(currentNode->triangles, mark);
            removeMark(currentNode->characters, mark.p);
            modifyNode(currentNode);
            break;
        }

        case eDeleteMarker:{
            go::point p(sgfX, sgfY);
            removeMark(currentNode->circles, p);
            removeMark(currentNode->crosses, p);
            removeMark(currentNode->squares, p);
            removeMark(currentNode->triangles, p);
            removeMark(currentNode->characters, p);
            removeStone(currentNode->emptyStones, p, go::point(boardX, boardY));
            removeStone(currentNode->blackStones, p, go::point(boardX, boardY));
            removeStone(currentNode->whiteStones, p, go::point(boardX, boardY));
            modifyNode(currentNode);
            break;
        }

        default:
            return;
    };
}

void BoardWidget::addMark(go::markList& markList, const go::mark& mark){
    go::markList::iterator iter = markList.begin();
    while (iter != markList.end()){
        if (iter->p == mark.p){
            markList.erase(iter);
            return;
        }
        ++iter;
    }

    markList.push_back(mark);
}

void BoardWidget::addCharacter(go::markList& markList, const go::point& p){
    go::markList::iterator iter1 = markList.begin();
    while (iter1 != markList.end()){
        if (iter1->p == p){
            markList.erase(iter1);
            return;
        }
        ++iter1;
    }

    QStringList marks;
    foreach(go::mark m, markList)
        marks.push_back(m.s);
    qSort(marks);

    int c = 'A';
    if (labelType == 1)
        c = 'a';
    else if (labelType == 2)
        c = 1;
    else if (labelType == 3 || labelType == 4)
        c = 0;

    QString s;
    while (true){
        if (labelType == 2)
            s.sprintf("%d", c);
        else if (labelType == 3)
            s = QString::fromUtf8(katakana[c]);
        else if (labelType == 4)
            s = QString::fromUtf8(kana_iroha[c]);
        else
            s = QChar(c);

        QStringList::iterator iter2 = qFind(marks.begin(), marks.end(), s);
        if (iter2 == marks.end())
            break;
        ++iter2;
        ++c;

        if ((labelType == 3 && c == katakana_size) || (labelType == 4 && c == kana_iroha_size))
            break;
    }
    markList.push_back(go::mark(p, s));
}

void BoardWidget::addManualEntry(go::markList& markList, const go::point& p){
    QString label = QInputDialog::getText(this, QString(), tr("Input Label"));
    if (label.isEmpty())
        return;

    go::markList::iterator iter = markList.begin();
    while (iter != markList.end()){
        if (iter->p == p){
            markList.erase(iter);
            return;
        }
        ++iter;
    }

    markList.push_back( go::mark(p, label) );
}

bool BoardWidget::removeMark(go::markList& markList, const go::point& p){
    go::markList::iterator iter = markList.begin();
    while (iter != markList.end()){
        if (iter->p == p){
            markList.erase(iter);
            return true;
        }
        ++iter;
    }
    return false;
}

void BoardWidget::addStone(go::nodePtr node, const go::point& sgfPoint, go::color color){
    go::point boardPoint;
    sgfToBoardCoordinate(sgfPoint.x, sgfPoint.y, boardPoint.x, boardPoint.y);
    addStone(node, sgfPoint, boardPoint, color);
}

void BoardWidget::addStone(go::nodePtr node, const go::point& sp, const go::point& bp, go::color c){
    if (removeStone(node->emptyStones, sp, bp) || removeStone(node->blackStones, sp, bp) || removeStone(node->whiteStones, sp, bp)){
        modifyNode(node);
        return;
    }

    if (!board[bp.y][bp.x].empty())
        return;

    go::nodePtr stoneNode( node->isStone() ? go::nodePtr(new go::node(node)) : node );

    if (c == go::empty)
        stoneNode->emptyStones.push_back( go::stone(sp, c) );
    else if (c == go::black)
        stoneNode->blackStones.push_back( go::stone(sp, c) );
    else
        stoneNode->whiteStones.push_back( go::stone(sp, c) );

    board[bp.y][bp.x].color = c;
    board[bp.y][bp.x].number = 0;

    if (stoneNode == node)
        modifyNode(node);
    else
        insertNodeCommand(node, stoneNode);
}

void BoardWidget::addEmpty(go::nodePtr node, const go::point& sgfPoint){
    go::point boardPoint;
    sgfToBoardCoordinate(sgfPoint.x, sgfPoint.y, boardPoint.x, boardPoint.y);
    addEmpty(node, sgfPoint, boardPoint);
}

void BoardWidget::addEmpty(go::nodePtr node, const go::point& sp, const go::point& bp){
    if (removeStone(node->emptyStones, sp, bp) || removeStone(node->blackStones, sp, bp) || removeStone(node->whiteStones, sp, bp)){
        modifyNode(node);
        return;
    }

    if (board[bp.y][bp.x].empty())
        return;

    go::nodePtr stoneNode( node->isStone() ? go::nodePtr(new go::node(node)) : node );

    stoneNode->emptyStones.push_back( go::stone(sp, go::empty) );
    board[bp.y][bp.x].color = go::empty;
    board[bp.y][bp.x].number = 0;

    if (stoneNode == node)
        modifyNode(node);
    else
        insertNodeCommand(node, stoneNode);
}

bool BoardWidget::removeStone(go::stoneList& stoneList, const go::point& sp, const go::point& bp){
    go::stoneList::iterator iter = stoneList.begin();
    while (iter != stoneList.end()){
        if (iter->p == sp) {
            stoneList.erase(iter);
            board[bp.y][bp.x].color = go::empty;
            board[bp.y][bp.x].number = 0;
            return true;
        }
        ++iter;
    }
    return false;
}

QString BoardWidget::toString(go::nodePtr node) const{
    if (node->isPass())
        return "Pass";
    else
        return getXYString(node->position.x, node->position.y);
}

QString BoardWidget::getXString(int x, bool showI) const{
    int a = x % 25;
    if (showI == false && a > 7)
        ++a;

    return QString("%1").arg(QChar('A' + a));
}

QString BoardWidget::getXString(int x) const{
    return getXString(x, showCoordinatesI);
}

QString  BoardWidget::getYString(int y) const{
    return QString("%1").arg(goData.root->ysize - y);
}

QString  BoardWidget::getXYString(int x, int y, bool showI) const{
    if (x < 0 || x >= goData.root->xsize || y < 0 || y >= goData.root->ysize)
        return "";

    QString s = getXString(x, showI);
    s.append( getYString(y) );
    return s;
}

QString BoardWidget::getXYString(int x, int y) const{
    return getXYString(x, y, showCoordinatesI);
}

void BoardWidget::boardToSgfCoordinate(int boardX, int boardY, int& sgfX, int& sgfY){
    if (rotateBoard_ == 0){
        sgfX = boardX;
        sgfY = boardY;
    }
    else if (rotateBoard_ == 1){
        sgfX = boardY;
        sgfY = xsize - boardX - 1;
    }
    else if (rotateBoard_ == 2){
        sgfX = xsize - boardX - 1;
        sgfY = ysize - boardY - 1;
    }
    else{
        sgfX = ysize - boardY - 1;
        sgfY = boardX;
    }

    if (flipBoardHorizontally_){
        if (rotateBoard_ == 0 || rotateBoard_ == 2)
            sgfX = goData.root->xsize - sgfX - 1;
        else
            sgfY = goData.root->ysize - sgfY - 1;
    }

    if (flipBoardVertically_){
        if (rotateBoard_ == 0 || rotateBoard_ == 2)
            sgfY = goData.root->ysize - sgfY - 1;
        else
            sgfX = goData.root->xsize - sgfX - 1;
    }
}

void BoardWidget::sgfToBoardCoordinate(int sgfX, int sgfY, int& boardX, int& boardY){
    if (rotateBoard_ == 0){
        boardX = sgfX;
        boardY = sgfY;
    }
    else if (rotateBoard_ == 1){
        boardX = xsize - sgfY - 1;
        boardY = sgfX;
    }
    else if (rotateBoard_ == 2){
        boardX = xsize - sgfX - 1;
        boardY = ysize - sgfY - 1;
    }
    else{
        boardX = sgfY;
        boardY = ysize - sgfX - 1;
    }

    if (flipBoardHorizontally_)
        boardX = xsize - boardX - 1;

    if (flipBoardVertically_)
        boardY = ysize - boardY - 1;
}

void BoardWidget::setCountTerritoryMode(bool countMode){
    if (countMode){
        editMode = eCountTerritory;
        countTerritory();
        int alive_b=0, alive_w=0, dead_b=0, dead_w=0, bt=0, wt=0;
        getCountTerritory(alive_b, alive_w, dead_b, dead_w, bt, wt);
        emit updateTerritory(alive_b, alive_w, dead_b, dead_w, capturedBlack, capturedWhite, bt, wt, goData.root->komi);
    }
    else{
        editMode = eAlternateMove;
        createBoardBuffer();
    }

    repaintBoard();
}

void BoardWidget::whiteFirst(bool whiteFirst){
    if (whiteFirst)
        goData.root->setColor(go::black);
    else
        goData.root->setColor(go::white);
    createBoardBuffer();
}

void BoardWidget::countTerritory(){
    char* tmp = new char[xsize * ysize];

    for (int y=0; y<ysize; ++y){
        for (int x=0; x<xsize; ++x){
            memset(tmp, 0, xsize*ysize);
            int c = go::empty;
            whichTerritory(x, y, tmp, c);
            if (board[y][x].empty() && (c & go::blackTerritory || c & go::whiteTerritory))
                board[y][x].color = c;
        }
    }

    delete[] tmp;
}

void BoardWidget::whichTerritory(int x, int y, char* tmp, int& c){
    if (tmp[y*xsize+x] != 0)
        return;
    tmp[y*xsize+x] = 1;

    if (c == go::dame)
        return;
    else if (board[y][x].whiteTerritory()){
        c = go::whiteTerritory;
        return;
    }
    else if (board[y][x].blackTerritory()){
        c = go::blackTerritory;
        return;
    }
    else if ((board[y][x].black() && c == go::whiteTerritory) || (board[y][x].white() && c == go::blackTerritory)){
        c = go::dame;
        return;
    }
    else if (board[y][x].black()){
        c = go::blackTerritory;
        return;
    }
    else if (board[y][x].white()){
        c = go::whiteTerritory;
        return;
    }

    if (y > 0)
        whichTerritory(x, y-1, tmp, c);
    if (y < ysize-1)
        whichTerritory(x, y+1, tmp, c);
    if (x > 0)
        whichTerritory(x-1, y, tmp, c);
    if (x < xsize-1)
        whichTerritory(x+1, y, tmp, c);
}

void BoardWidget::addTerritory(int x, int y){
    if (board[y][x].white() && !board[y][x].blackTerritory())
        setTerritory(x, y, go::blackTerritory);
    else if (board[y][x].black() && !board[y][x].whiteTerritory())
        setTerritory(x, y, go::whiteTerritory);
    else if ( (board[y][x].white() || board[y][x].black()) && (board[y][x].blackTerritory() || board[y][x].whiteTerritory()) )
        unsetTerritory(x, y);

    countTerritory();
    repaintBoard(false);
}

void BoardWidget::setTerritory(int x, int y, int c){
    if ( c & go::blackTerritory && (board[y][x].black() || board[y][x].blackTerritory()) )
        return;
    else if ( c & go::whiteTerritory && (board[y][x].white() || board[y][x].whiteTerritory()) )
        return;

    board[y][x].color |= c;
    if (c == go::blackTerritory)
        board[y][x].color &= ~go::whiteTerritory;
    else
        board[y][x].color &= ~go::blackTerritory;

    if (y > 0)
        setTerritory(x, y-1, c);
    if (y < ysize-1)
        setTerritory(x, y+1, c);
    if (x > 0)
        setTerritory(x-1, y, c);
    if (x < xsize-1)
        setTerritory(x+1, y, c);
}

void BoardWidget::unsetTerritory(int x, int y){
    if ( !board[y][x].blackTerritory() && !board[y][x].whiteTerritory() )
        return;

    board[y][x].color &= go::black | go::white;

    if (y > 0)
        unsetTerritory(x, y-1);
    if (y < ysize-1)
        unsetTerritory(x, y+1);
    if (x > 0)
        unsetTerritory(x-1, y);
    if (x < xsize-1)
        unsetTerritory(x+1, y);
}

void BoardWidget::getCountTerritory(int& alive_b, int& alive_w, int& dead_b, int& dead_w, int& bt, int& wt){
    for (int y=0; y<ysize; ++y){
        for (int x=0; x<xsize; ++x){
            if (board[y][x].blackTerritory()){
                ++bt;
                if (board[y][x].white())
                    ++dead_w;
            }
            else if (board[y][x].whiteTerritory()){
                ++wt;
                if (board[y][x].black())
                    ++dead_b;
            }
            else if (board[y][x].black())
                ++alive_b;
            else if (board[y][x].white())
                ++alive_w;
        }
    }
}

void BoardWidget::playWithComputer(PlayGame* game){
    playGame = game;
    if (playGame){
        editMode = ePlayGame;
        if (playGame->yourColor() == go::white && goData.root->handicap == 0)
            playGame->wait();
        else if (goData.root->handicap > 0){
            whiteFirst(true);
            playGame->setHandicap();
            if (playGame->yourColor() == go::black)
                playGame->wait();
        }
    }
    else{
        editMode = eAlternateMove;
    }
}