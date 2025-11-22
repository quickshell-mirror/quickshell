#include "webengine.hpp"

#include <qtest.h>

#include "../webengine.hpp"

void WebEngineInitTest::initDoesNotCrash() {
	QVERIFY(qs::web_engine::init());
}

QTEST_MAIN(WebEngineInitTest)