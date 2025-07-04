#include "process.hpp"

#include <qlist.h>
#include <qsignalspy.h>
#include <qtest.h>
#include <qtestcase.h>

#include "../process.hpp"

void TestProcess::startAfterReload() {
	auto process = Process();
	auto startedSpy = QSignalSpy(&process, &Process::started);
	auto exitedSpy = QSignalSpy(&process, &Process::exited);

	process.setCommand({"true"});
	process.setRunning(true);

	QVERIFY(!process.isRunning());
	QCOMPARE(startedSpy.count(), 0);

	process.postReload();

	QVERIFY(process.isRunning());
	QVERIFY(startedSpy.wait(100));
}

void TestProcess::testExec() {
	auto process = Process();
	auto startedSpy = QSignalSpy(&process, &Process::started);
	auto exitedSpy = QSignalSpy(&process, &Process::exited);

	process.postReload();

	process.setCommand({"sleep", "30"});
	process.setRunning(true);

	QVERIFY(process.isRunning());
	QVERIFY(startedSpy.wait(100));

	process.exec({"true"});

	QVERIFY(exitedSpy.wait(100));
	QVERIFY(startedSpy.wait(100));
	QVERIFY(process.isRunning());
}

QTEST_MAIN(TestProcess);
