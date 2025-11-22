#include "webengine.hpp"

#include <qtest.h>

#include "../webengine.hpp"

void WebEngineInitTest::init() {
	QVERIFY(qs::web_engine::init());
}

QTEST_MAIN(WebEngineInitTest)