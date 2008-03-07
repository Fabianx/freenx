#include <QCoreApplication>
#include <QString>

#include "nxclientlib.h"

int main(int argc, char **argv)
{
	QCoreApplication *qApp = new QCoreApplication(argc, argv);
	NXClientLib lib(qApp);

	lib.invokeNXSSH("default" ,argv[1], true);
	lib.setUsername(argv[2]);
	lib.setPassword(argv[3]);
	lib.setResolution(640,480);
	lib.setDepth(24);
	lib.setRender(true);

	NXSessionData session;

	// HARDCODED TEST CASE
	session.sessionName = "TEST";
	session.sessionType = "unix-kde";
	session.cache = 8;
	session.images = 32;
	session.linkType = "adsl";
	session.render = true;
	session.backingstore = "when_requested";
	session.imageCompressionMethod = 2;
	// session.imageCompressionLevel;
	session.geometry = "800x600+0+0";
	session.keyboard = "defkeymap";
	session.kbtype = "pc102/defkeymap";
	session.media = false;
	session.agentServer = "";
	session.agentUser = "";
	session.agentPass = "";
	session.cups = 0;
	session.suspended = false;

	lib.setSession(&session);
	return qApp->exec();
}
