#include "qml.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qobject.h>

#include "connection.hpp"

Greetd::Greetd(QObject* parent): QObject(parent) {
	auto* connection = GreetdConnection::instance();

	QObject::connect(connection, &GreetdConnection::authMessage, this, &Greetd::authMessage);
	QObject::connect(connection, &GreetdConnection::authFailure, this, &Greetd::authFailure);
	QObject::connect(connection, &GreetdConnection::readyToLaunch, this, &Greetd::readyToLaunch);
	QObject::connect(connection, &GreetdConnection::launched, this, &Greetd::launched);
	QObject::connect(connection, &GreetdConnection::error, this, &Greetd::error);

	QObject::connect(connection, &GreetdConnection::stateChanged, this, &Greetd::stateChanged);
	QObject::connect(connection, &GreetdConnection::userChanged, this, &Greetd::userChanged);
}

void Greetd::createSession(QString user) {
	GreetdConnection::instance()->createSession(std::move(user));
}

void Greetd::cancelSession() { GreetdConnection::instance()->cancelSession(); }

void Greetd::respond(QString response) {
	GreetdConnection::instance()->respond(std::move(response));
}

void Greetd::launch(const QList<QString>& command) {
	GreetdConnection::instance()->launch(command, {}, true);
}

void Greetd::launch(const QList<QString>& command, const QList<QString>& environment) {
	GreetdConnection::instance()->launch(command, environment, true);
}

void Greetd::launch(const QList<QString>& command, const QList<QString>& environment, bool quit) {
	GreetdConnection::instance()->launch(command, environment, quit);
}

bool Greetd::isAvailable() { return GreetdConnection::instance()->isAvailable(); }
GreetdState::Enum Greetd::state() { return GreetdConnection::instance()->state(); }
QString Greetd::user() { return GreetdConnection::instance()->user(); }
