#include <QTextStream>
#include <QString>

void warning(const QString& s);
void debug(const QString& s);
void error(const QString& s);
void message(const QString& s);

void warning(QString& s);
void debug(QString& s);
void error(QString& s);
void message(QString& s);

void warning(char *s);
void debug(char *s);
void error(char *s);
void message(char *s);
