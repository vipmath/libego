#include <cassert>
#include <QDir>
#include <QProcess>
#include <QString>
#include <QVariant>
#include <QDebug>

#include "gtp_process.hpp"

GtpProcess::GtpProcess () {
}

GtpProcess::~GtpProcess () {
  process.close();
}

bool GtpProcess::Start (QString name_,
                        QString command_line_,
                        int timeout_ms)
{
  name = name_;
  command_line = command_line_.trimmed();

  process.start (command_line);
  if (!process.waitForStarted (timeout_ms)) {
    qDebug () << command_line << " failed to start";
    return false;
  }

  if (!BasicCommandsOk ()) {
    return false;
  }

  return true;
}


bool GtpProcess::BasicCommandsOk () {
  QString response;
  return
    Command ("known_command genmove",     &response) && response == "true" &&
    Command ("known_command play",        &response) && response == "true" &&
    // because MoGo fails here
    //Command ("known_command boardsize",   &response) && response == "true" &&
    Command ("known_command clear_board", &response) && response == "true" &&
    Command ("known_command komi",        &response) && response == "true";
}


bool GtpProcess::IsRunning () {
  return process.state () == QProcess::Running;
}

QString GtpProcess::Name () const {
  return name;
}

QString GtpProcess::CommandLine () const {
  return command_line;
}


bool GtpProcess::Command (QString command, QString* response, int timeout_ms) {
  if (command.trimmed() == "") return true;
  process.write (command.toAscii());
  process.write ("\n");

  // read chunk
  const QString sep = "\n\n";

  QString chunk;
  while (!chunk.contains (sep)) {
    if (!process.waitForReadyRead (timeout_ms)) {
      qDebug() << "GtpProcess Command timeout on command: " << command;
      qDebug() << "Read so far: " << chunk;
      return false;
    }
    chunk += process.readAll();
  }

  if (chunk.count (sep) != 1 || !chunk.endsWith (sep)  || chunk[0] != '=') {
    CHECK (chunk[0] == '?');
    qDebug() << "Command: " << command << " failed";
    return false;
  }

  chunk[0] = ' ';
  if (response != NULL) *response = chunk.trimmed();
  return true;
}

QVariant GtpProcess::TryCommand (QString command) {
  QString ret = "";
  if (!Command (command, &ret)) {
    return QVariant();
  }
  return ret;
}

bool GtpProcess::ClearBoard (uint board_size, float komi) {
  return 
    Command ("boardsize " + QString::number (board_size), NULL) &&
    Command ("komi " + QString::number (komi), NULL) &&
    Command ("clear_board", NULL);
}

bool GtpProcess::Play (Move m) {
  return Command ("play " + QString::fromStdString(m.ToGtpString()),  NULL);
}

bool GtpProcess::Genmove (Player pl, Vertex* v, int timeout_ms) {
  QString response;
  QString pl_str = pl.ToGtpString().c_str();
  if (!Command ("genmove " + pl_str, &response, timeout_ms)) return false;
  *v = Vertex::OfGtpString (response.toStdString());
  return true;
}

bool GtpProcess::Quit () {
  if (!Command ("quit", NULL)) return false;
  if (!process.waitForFinished (1000)) {
    process.close();
  }
  return true;
}
