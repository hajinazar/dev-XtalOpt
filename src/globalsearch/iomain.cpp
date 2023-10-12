#include <globalsearch/iomain.h>

inline QTextStream& qStdout()
{
    static QTextStream r{stdout};
    return r;
}

/*
void warning(const QString& s) 
{
  qStdout() << "Warning: " << s << "\n" << flush;
}
void debug(const QString& s)
{
  qStdout() << "Debug: " << s << "\n" << flush;
}
void error(const QString& s)
{
  qStdout() << "Error: " << s << "\n" << flush;
}
void message(const QString& s)
{
  qStdout() << s << "\n" << flush;
}
*/

void warning(const QString& s)   { qStdout() << "Warning: " << s << "\n" << flush; }
void debug(const QString& s)     { qStdout() << "Debug: " << s << "\n" << flush;   }
void error(const QString& s)     { qStdout() << "Error: " << s << "\n" << flush;   }
void message(const QString& s)   { qStdout() << s << "\n" << flush;                }

void warning(QString& s)   { qStdout() << "Warning: " << s << "\n" << flush; }
void debug(QString& s)     { qStdout() << "Debug: " << s << "\n" << flush;   }
void error(QString& s)     { qStdout() << "Error: " << s << "\n" << flush;   }
void message(QString& s)   { qStdout() << s << "\n" << flush;                }

void warning(char* s)      { qStdout() << "Warning: " << s << "\n" << flush; }
void debug(char* s)        { qStdout() << "Debug: " << s << "\n" << flush;   }
void error(char* s)        { qStdout() << "Error: " << s << "\n" << flush;   }
void message(char* s)      { qStdout() << s << "\n" << flush;                }
