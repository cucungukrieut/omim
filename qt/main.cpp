#include "qt/info_dialog.hpp"
#include "qt/mainwindow.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include "std/cstdio.hpp"
#include "std/cstdlib.hpp"

#include "3party/Alohalytics/src/alohalytics.h"

#include <QSurfaceFormat>
#include <QtCore/QDir>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QApplication>
#else
  #include <QtWidgets/QApplication>
#endif

namespace
{
  class FinalizeBase
  {
  public:
    ~FinalizeBase()
    {
      // optional - clean allocated data in protobuf library
      // useful when using memory and resource leak utilites
      //google::protobuf::ShutdownProtobufLibrary();
    }
  };

#if defined(OMIM_OS_WINDOWS) //&& defined(PROFILER_COMMON)
  class InitializeFinalize : public FinalizeBase
  {
    FILE * m_errFile;
    my::ScopedLogLevelChanger const m_debugLog;
  public:
    InitializeFinalize() : m_debugLog(LDEBUG)
    {
      // App runs without error console under win32.
      m_errFile = ::freopen(".\\mapsme.log", "w", stderr);

      //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF);
      //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
    ~InitializeFinalize()
    {
      ::fclose(m_errFile);
    }
  };
#else
  typedef FinalizeBase InitializeFinalize;
#endif
}

int main(int argc, char * argv[])
{
  Q_INIT_RESOURCE(resources_common);

  // Our double parsing code (base/string_utils.hpp) needs dots as a floating point delimiters, not commas.
  // TODO: Refactor our doubles parsing code to use locale-independent delimiters.
  // For example, https://github.com/google/double-conversion can be used.
  // See http://dbaron.org/log/20121222-locale for more details.
  (void)::setenv("LC_NUMERIC", "C", 1);

  InitializeFinalize mainGuard;
  UNUSED_VALUE(mainGuard);

  QApplication a(argc, argv);

#ifdef DEBUG
  alohalytics::Stats::Instance().SetDebugMode(true);
#endif

  GetPlatform().SetupMeasurementSystem();

  // display EULA if needed
  char const * settingsEULA = "EulaAccepted";
  bool eulaAccepted = false;
  if (!settings::Get(settingsEULA, eulaAccepted) || !eulaAccepted)
  {
    QStringList buttons;
    buttons << "Accept" << "Decline";

    string buffer;
    {
      ReaderPtr<Reader> reader = GetPlatform().GetReader("eula.html");
      reader.ReadAsString(buffer);
    }
    qt::InfoDialog eulaDialog("MAPS.ME End User Licensing Agreement", buffer.c_str(), NULL, buttons);
    eulaAccepted = (eulaDialog.exec() == 1);
    settings::Set(settingsEULA, eulaAccepted);
  }

  int returnCode = -1;
  if (eulaAccepted)   // User has accepted EULA
  {
    bool useOpenGL3 = false;
#if defined(OMIM_OS_MAC)
    useOpenGL3 = a.arguments().contains("gl3");
#endif
    qt::MainWindow::SetDefaultSurfaceFormat(useOpenGL3);

    Framework framework;
    qt::MainWindow w(framework, useOpenGL3);
    w.show();
    returnCode = a.exec();
  }

  LOG_SHORT(LINFO, ("MapsWithMe finished with code", returnCode));
  return returnCode;
}
