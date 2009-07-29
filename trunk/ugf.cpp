#include <QDebug>
#include "appdef.h"
#include "ugf.h"

namespace go{



bool ugf::readStream(QString::iterator& first, QString::iterator last){
    dataList_.push_back( data() );
    while (first != last){
        QString s = readLine(first, last);
        if (s == "[Header]")
            readHeader(first, last);
        else if (s == "[Remote]")
            readRemote(first, last);
        else if (s == "[Files]")
            readFiles(first, last);
        else if (s == "[Data]")
            readData(first, last);
        else if (s == "[Figure]")
            readFigure(first, last);
    }

    return false;
}

bool ugf::saveStream(QTextStream& stream){
    return false;
}

bool ugf::get(go::data& data) const{
    data.clear();

    data.root.gameName = title;
    data.root.place = place;
    data.root.date = date;
    data.root.rule = rule;
    data.root.xsize = size;
    data.root.ysize = size;
    data.root.handicap = handicap;
    data.root.komi = komi;
    data.root.result = winner;
    data.root.user = writer;
    data.root.copyright = copyright;
    data.root.whitePlayer = whitePlayer;
    data.root.whiteRank = whiteRank;
    data.root.blackPlayer = blackPlayer;
    data.root.blackRank = blackRank;

    dataList::const_iterator first = dataList_.begin();
    data.root.comment = first->comment;
    markerList::const_iterator marker = first->markers.begin();
    while (marker != first->markers.end()){
        data.root.characters.push_back( go::mark(marker->x, marker->y, marker->str) );
        ++marker;
    }

    return get(++first, dataList_.end(), &data.root);
}

bool ugf::set(const go::data& data){
    return false;
}

QString ugf::readLine(QString::iterator& first, QString::iterator& last){
    QString str;
    while (first != last){
        QChar c = *first++;
        if (c == '\r')
            continue;
        else if (c == '\n')
            break;
        str.push_back(c);
    }
    return str;
}

bool ugf::readHeader(QString::iterator& first, QString::iterator& last){
    while (first != last && *first != '['){
        QString str = readLine(first, last);
        if (!str.isEmpty() && str[0] == '[')
            break;

        int pos = str.indexOf('=');
        if (pos == -1)
            continue;

        QString key   = str.left(pos);
        QString value = str.mid(pos + 1);

        if (key == "Title")
            title = value;
        else if (key == "Place")
            place = value;
        else if (key == "Date")
            date = value;
        else if (key == "Rule")
            rule = value;
        else if (key == "Size")
            size = value.toInt();
        else if (key == "Hdcp"){
            QStringList list = value.split(',');
            handicap = list[0].toInt();
            if (list.size() > 1)
                komi = list[1].toDouble();
        }
        else if (key == "Winner"){
            QStringList list = value.split(',');
            winner = list[0];
            if (list.size() > 1){
                if (list[1] == "C")
                    winner.append("+R");
                else
                    winner += "+" + list[1];
            }
        }
        else if (key == "Writer")
            writer = value;
        else if (key == "Copyright")
            copyright = value;
        else if (key == "WMemb1" || key == "PlayerW"){
            QStringList list = value.split(',');
            whitePlayer = list[0];
            if (list.size() > 1)
                whiteRank = list[1];
        }
        else if (key == "BMemb1" || key == "PlayerB"){
            QStringList list = value.split(',');
            blackPlayer = list[0];
            if (list.size() > 1)
                blackRank = list[1];
        }
    }

    return true;
}

bool ugf::readRemote(QString::iterator& first, QString::iterator& last){
    while (first != last && *first != '['){
        QString str = readLine(first, last);
        if (!str.isEmpty() && str[0] == '[')
            break;
    }

    return false;
}

bool ugf::readFiles(QString::iterator& first, QString::iterator& last){
    while (first != last && *first != '['){
        QString str = readLine(first, last);
        if (!str.isEmpty() && str[0] == '[')
            break;
    }

    return false;
}

bool ugf::readData(QString::iterator& first, QString::iterator& last){
    while (first != last && *first != '['){
        QString str = readLine(first, last);
        QStringList list = str.split(',');
        if (list.size() != 4)
            continue;

        if (list[2].toInt() < 1)
            continue;

        if (list[0].size() < 2)
            continue;
        data d;
        d.x = list[0][0].toAscii() - 'A';
        d.y = list[0][1].toAscii() - 'A';

        if (list[1].size() < 2)
            continue;
        d.color = list[1][0] == 'B' ? go::stone::eBlack : go::stone::eWhite;

        dataList_.push_back(d);
    }

    return true;
}

bool ugf::readFigure(QString::iterator& first, QString::iterator& last){
    while (first != last && *first != '['){
        QString str = readLine(first, last);
        QStringList list =  str.split(',');
        if (list.size() < 4)
            continue;

        if (list[0] == ".Text")
            readFigureText(first, last, list[1].toInt());
    }

    return false;
}

bool ugf::readFigureText(QString::iterator& first, QString::iterator& last, int index){
    if (dataList_.size() <= index)
        return false;

    data& d = dataList_[index];

    while (first != last && *first != '['){
        QString str = readLine(first, last);
        if (!str.isEmpty() && str[0] == '.'){
            if (str == ".EndText")
                break;

            QStringList list =  str.split(',');
            if (list[0] == ".#" && list.size() >= 4)
                d.markers.push_back( marker(list[1].toInt()-1, list[2].toInt()-1, list[3]) );
        }
        else{
            if (!d.comment.isEmpty())
                d.comment.push_back('\n');
            d.comment.append(str);
        }
    }

    return true;
}

bool ugf::get(dataList::const_iterator first, dataList::const_iterator last, go::node* parent) const{
    if (first == last)
        return true;

    node* node;
    if (first->color == go::stone::eBlack)
        node = new go::blackNode(parent, first->x, first->y);
    else
        node = new go::whiteNode(parent, first->x, first->y);

    node->comment = first->comment;

    markerList::const_iterator marker = first->markers.begin();
    while (marker != first->markers.end()){
        node->characters.push_back( go::mark(marker->x, marker->y, marker->str) );
        ++marker;
    }

    parent->childNodes.push_back(node);

    return get(++first, last, node);
}




};