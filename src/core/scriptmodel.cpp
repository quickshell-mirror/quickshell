#include "scriptmodel.hpp"
#include <algorithm>
#include <iterator>

#include <qabstractitemmodel.h>
#include <qcontainerfwd.h>
#include <qjsvalue.h>
#include <qjsvalueiterator.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtversionchecks.h>
#include <qtypes.h>
#include <qvariant.h>

namespace {
struct RecursionPair {
	const QJSValue* a;
	const QJSValue* b;
	const RecursionPair* previous;
};

bool qjsValueStructuralEq(
    const QJSValue& a,
    const QJSValue& b,
    const RecursionPair* activePair = nullptr
) {
	if (a.strictlyEquals(b)) return true;
	if (!a.isObject() || !b.isObject() || a.isArray() != b.isArray()) return false;

	for (const auto* pair = activePair; pair != nullptr; pair = pair->previous) {
		if (a.strictlyEquals(*pair->a) || b.strictlyEquals(*pair->b)) return false;
	}

	const auto nextActivePair = RecursionPair {.a = &a, .b = &b, .previous = activePair};

	auto aIter = QJSValueIterator(a);
	auto bIter = QJSValueIterator(b);

	while (aIter.hasNext()) {
		if (!bIter.hasNext()) return false;

		aIter.next();
		bIter.next();

		const auto aName = aIter.name();
		const auto bName = bIter.name();
		const auto aValue = aIter.value();

		if (aName == bName) {
			const auto bValue = bIter.value();
			if (!qjsValueStructuralEq(aValue, bValue, &nextActivePair)) return false;
		} else {
			if (!b.hasOwnProperty(aName)) return false;

			const auto bValue = b.property(aName);
			if (!qjsValueStructuralEq(aValue, bValue, &nextActivePair)) return false;
		}
	}

	return !bIter.hasNext();
}
} // namespace

bool ScriptModel::updateValuesUnique(const QList<QJSValue>& newValues) {
	auto anyChanges = false;

	this->isModifying = true;
	this->mValues.reserve(newValues.size());

	auto iter = this->mValues.begin();
	auto newIter = newValues.begin();

	auto comparisonValue = [this](const QJSValue& value) {
		if (value.hasProperty(this->cmpKey)) return value.property(this->cmpKey);

		return value;
	};

	auto valueCmp = [&, this](const QJSValue& a, const QJSValue& b) {
		if (a.strictlyEquals(b)) return true;

		const auto aValue = this->cmpKey.isEmpty() ? a : comparisonValue(a);
		const auto bValue = this->cmpKey.isEmpty() ? b : comparisonValue(b);

		if (this->mComparisonMode == ObjectComparison::Identity) {
			return aValue.strictlyEquals(bValue);
		} else {
			return qjsValueStructuralEq(aValue, bValue);
		}
	};

	auto eqPredicate = [&](const QJSValue& b) {
		return [&](const QJSValue& a) { return valueCmp(a, b); };
	};

	while (true) {
		if (newIter == newValues.end()) {
			if (iter == this->mValues.end()) break;

			auto startIndex = static_cast<qint32>(newValues.length());
			auto endIndex = static_cast<qint32>(this->mValues.length() - 1);

			this->beginRemoveRows(QModelIndex(), startIndex, endIndex);
			this->mValues.erase(iter, this->mValues.end());
			this->endRemoveRows();
			anyChanges = true;

			break;
		} else if (iter == this->mValues.end()) {
			// Prior branch ensures length is at least 1.
			auto startIndex = static_cast<qint32>(this->mValues.length());
			auto endIndex = static_cast<qint32>(newValues.length() - 1);

			this->beginInsertRows(QModelIndex(), startIndex, endIndex);
			this->mValues.append(newValues.sliced(startIndex));
			this->endInsertRows();
			anyChanges = true;

			break;
		} else if (!valueCmp(*newIter, *iter)) {
			auto oldIter = std::find_if(iter, this->mValues.end(), eqPredicate(*newIter));

			if (oldIter != this->mValues.end()) {
				if (std::find_if(newIter, newValues.end(), eqPredicate(*iter)) == newValues.end()) {
					// Remove any entries we would otherwise move around that aren't in the new list.
					auto startIter = iter;

					do {
						++iter;
					} while (iter != this->mValues.end()
					         && std::find_if(newIter, newValues.end(), eqPredicate(*iter))
					                == newValues.end());

					auto index = static_cast<qint32>(std::distance(this->mValues.begin(), iter));
					auto startIndex = static_cast<qint32>(std::distance(this->mValues.begin(), startIter));

					this->beginRemoveRows(QModelIndex(), startIndex, index - 1);
					iter = this->mValues.erase(startIter, iter);
					this->endRemoveRows();
					anyChanges = true;
				} else {
					// Advance iters to capture a whole move sequence as a single operation if possible.
					auto oldStartIter = oldIter;
					do {
						++oldIter;
						++newIter;
					} while (oldIter != this->mValues.end() && newIter != newValues.end()
					         && valueCmp(*oldIter, *newIter));

					auto index = static_cast<qint32>(std::distance(this->mValues.begin(), iter));
					auto oldStartIndex =
					    static_cast<qint32>(std::distance(this->mValues.begin(), oldStartIter));
					auto oldIndex = static_cast<qint32>(std::distance(this->mValues.begin(), oldIter));
					auto len = oldIndex - oldStartIndex;

					this->beginMoveRows(QModelIndex(), oldStartIndex, oldIndex - 1, QModelIndex(), index);

					// While it is possible to optimize this further, it is currently not worth the time.
					for (auto i = 0; i != len; i++) {
						this->mValues.move(oldStartIndex + i, index + i);
					}

					iter = this->mValues.begin() + (index + len);
					this->endMoveRows();
					anyChanges = true;
				}
			} else {
				auto startNewIter = newIter;

				do {
					newIter++;
				} while (newIter != newValues.end()
				         && std::find_if(iter, this->mValues.end(), eqPredicate(*newIter))
				                == this->mValues.end());

				auto index = static_cast<qint32>(std::distance(this->mValues.begin(), iter));
				auto newIndex = static_cast<qint32>(std::distance(newValues.begin(), newIter));
				auto startNewIndex = static_cast<qint32>(std::distance(newValues.begin(), startNewIter));
				auto len = newIndex - startNewIndex;

				this->beginInsertRows(QModelIndex(), index, index + len - 1);
#if QT_VERSION <= QT_VERSION_CHECK(6, 8, 0)
				this->mValues.resize(this->mValues.length() + len);
#else
				this->mValues.resizeForOverwrite(this->mValues.length() + len);
#endif
				iter = this->mValues.begin() + index; // invalidated
				std::move_backward(iter, this->mValues.end() - len, this->mValues.end());
				iter = std::copy(startNewIter, newIter, iter);
				this->endInsertRows();
				anyChanges = true;
			}
		} else if (!newIter->strictlyEquals(*iter)) {
			auto first = static_cast<qint32>(std::distance(this->mValues.begin(), iter));
			auto index = first;

			do {
				this->mValues.replace(index, *newIter);
				++iter;
				++newIter;
				++index;
			} while (iter != this->mValues.end() && newIter != newValues.end()
			         && !newIter->strictlyEquals(*iter));

			this->dataChanged(
			    this->index(first, 0, QModelIndex()),
			    this->index(index - 1, 0, QModelIndex()),
			    {Qt::UserRole}
			);

			anyChanges = true;
		} else {
			++iter;
			++newIter;
		}
	}

	this->isModifying = false;

	if (this->stagedValues.has_value()) {
		auto values = *this->stagedValues;
		this->stagedValues.reset();
		anyChanges |= this->updateValuesUnique(values);
	}

	return anyChanges;
}

void ScriptModel::setValues(const QList<QJSValue>& newValues) {
	// Re-entrant modification waits for the original modification to complete to avoid mishandled duplicates.
	if (this->isModifying) {
		this->stagedValues = newValues;
		return;
	}

	auto changed = this->updateValuesUnique(newValues);
	if (changed) emit this->valuesChanged();
}

void ScriptModel::setObjectProp(const QString& objectProp) {
	if (objectProp == this->cmpKey) return;
	this->cmpKey = objectProp;
	this->updateValuesUnique(this->mValues);
	emit this->objectPropChanged();
}

void ScriptModel::setComparisonMode(ObjectComparison::Enum comparisonMode) {
	if (comparisonMode == this->mComparisonMode) return;
	this->mComparisonMode = comparisonMode;
	emit this->comparisonModeChanged();
}

qint32 ScriptModel::rowCount(const QModelIndex& parent) const {
	if (parent != QModelIndex()) return 0;
	return static_cast<qint32>(this->mValues.length());
}

QVariant ScriptModel::data(const QModelIndex& index, qint32 role) const {
	if (role != Qt::UserRole) return QVariant();
	return this->mValues.at(index.row()).toVariant(QJSValue::RetainJSObjects);
}

QHash<int, QByteArray> ScriptModel::roleNames() const { return {{Qt::UserRole, "modelData"}}; }
