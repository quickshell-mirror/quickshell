#pragma once

#include <qqmlintegration.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "qml.hpp"

namespace qs::wayland::input_method {

class KeyboardTextEdit: public Keyboard {
	Q_OBJECT;
	Q_PROPERTY(QJSValue transform READ transform WRITE setTransform NOTIFY transformChanged FINAL)
	Q_PROPERTY(int cursor MEMBER mCursor READ cursor WRITE setCursor NOTIFY cursorChanged);
	Q_PROPERTY(
	    QString editText MEMBER mEditText READ editText WRITE setEditText NOTIFY editTextChanged
	);
	QML_ELEMENT;

public:
	KeyboardTextEdit(QObject* parent = nullptr);

	Q_INVOKABLE [[nodiscard]] QJSValue transform() const;
	Q_INVOKABLE void setTransform(const QJSValue& callback);

	Q_INVOKABLE [[nodiscard]] int cursor() const;
	Q_INVOKABLE void setCursor(int value);

	Q_INVOKABLE [[nodiscard]] QString editText() const;
	Q_INVOKABLE void setEditText(const QString& value);

signals:
	void transformChanged();
	void cursorChanged();
	void editTextChanged();

private slots:
	void onKeyPress(QChar character);
	void onReturnPress();
	void onDirectionPress(KeyboardDirectionKey::Enum direction);
	void onBackspacePress();
	void onDeletePress();

private:
	void updatePreedit();

	QJSValue mTransform;
	int mCursor = 0;
	QString mEditText = "";
};

} // namespace qs::wayland::input_method
