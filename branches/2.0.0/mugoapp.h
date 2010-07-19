/*
    mugo, sgf editor.
    Copyright (C) 2009-2010 nsase.

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
#ifndef __mugoapp_h__
#define __mugoapp_h__


#include <QApplication>
#include <QList>


#define APP_NAME     "mugo"
#define APP_VERSION  "2.0.0"
#define SETTING_NAME "mugo2"
#define AUTHOR       "nsase"
#define COPYRIGHT    "Copyright 2009-2010 nsase."


class QAction;
class QTextCodec;


class MugoApplication;
inline MugoApplication* mugoApp(){
    return (MugoApplication*)qApp;
}


/**
  Mugo Application
*/
class MugoApplication : public QApplication{
    Q_OBJECT
    public:
        MugoApplication(int& argc, char** argv);

        void setDefaultCodec(QTextCodec* codec){ defaultCodec_ = codec; }
        void setEncodingActions( const QList<QAction*>& actions ){ encodingActions_ = actions; }
        void setCodecs( QList<QTextCodec*>& codecs ){ codecs_ = codecs; }

        QTextCodec* defaultCodec() const{ return defaultCodec_; }
        const QList<QAction*>& encodingActions() const{ return encodingActions_; }
        const QList<QTextCodec*>& codecs() const{ return codecs_; }

    private:
        QTextCodec* defaultCodec_;
        QList<QAction*>    encodingActions_;
        QList<QTextCodec*> codecs_;
};


#endif