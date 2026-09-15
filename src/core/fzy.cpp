#include "fzy.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <qlist.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringview.h>
#include <qtypes.h>
#include <qvariant.h>
#include <span>

namespace {
constexpr qsizetype MATCH_MAX_LEN = 1024;

constexpr double SCORE_MAX = std::numeric_limits<double>::infinity();
constexpr double SCORE_MIN = -std::numeric_limits<double>::infinity();

constexpr double SCORE_GAP_LEADING = -0.005;
constexpr double SCORE_GAP_TRAILING = -0.005;
constexpr double SCORE_GAP_INNER = -0.01;
constexpr double SCORE_MATCH_CONSECUTIVE = 1.0;
constexpr double SCORE_MATCH_SLASH = 0.9;
constexpr double SCORE_MATCH_WORD = 0.8;
constexpr double SCORE_MATCH_CAPITAL = 0.7;
constexpr double SCORE_MATCH_DOT = 0.6;

struct ScoredResult {
	double score {};
	QString str;
	QObject* obj = nullptr;
};

bool hasMatch(QStringView needle, QStringView haystack) {
	qsizetype index = 0;
	for (auto needleChar: needle) {
		index = haystack.indexOf(needleChar, index, Qt::CaseInsensitive);
		if (index == -1) {
			return false;
		}
		index++;
	}
	return true;
}

struct MatchStruct {
	QString lowerNeedle;
	QString lowerHaystack;

	std::array<double, MATCH_MAX_LEN> matchBonus {};
};

double getBonus(QChar ch, QChar lastCh) {
	if (!lastCh.isLetterOrNumber()) {
		return 0.0;
	}
	switch (ch.unicode()) {
	case '/': return SCORE_MATCH_SLASH;
	case '-':
	case '_':
	case ' ': return SCORE_MATCH_WORD;
	case '.': return SCORE_MATCH_DOT;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z': return lastCh.isUpper() ? SCORE_MATCH_CAPITAL : 0.0;
	default: return 0.0;
	}
}

void precomputeBonus(QStringView haystack, std::span<double> matchBonus) {
	/* Which positions are beginning of words */
	QChar lastCh = '/';
	for (qsizetype index = 0; index < haystack.size(); index++) {
		const QChar ch = haystack[index];
		matchBonus[index] = getBonus(lastCh, ch);
		lastCh = ch;
	}
}

MatchStruct setupMatchStruct(QStringView needle, QStringView haystack) {
	MatchStruct match {};

	for (const auto nch: needle) {
		match.lowerNeedle.push_back(nch.toLower());
	}
	for (const auto hch: haystack) {
		match.lowerHaystack.push_back(hch.toLower());
	}

	precomputeBonus(haystack, match.matchBonus);

	return match;
}

void matchRow(
    const MatchStruct& match,
    qsizetype row,
    std::span<double> currD,
    std::span<double> currM,
    std::span<const double> lastD,
    std::span<const double> lastM
) {
	const qsizetype needleLen = match.lowerNeedle.size();
	const qsizetype haystackLen = match.lowerHaystack.size();

	const QStringView lowerNeedle = match.lowerNeedle;
	const QStringView lowerHaystack = match.lowerHaystack;
	const std::span<const double> matchBonus = match.matchBonus;

	double prevScore = SCORE_MIN;
	const double gapScore = row == needleLen - 1 ? SCORE_GAP_TRAILING : SCORE_GAP_INNER;

	/* These will not be used with this value, but not all compilers see it */
	double prevM = SCORE_MIN;
	double prevD = SCORE_MIN;

	for (qsizetype index = 0; index < haystackLen; index++) {
		if (lowerNeedle[row] == lowerHaystack[index]) {
			double score = SCORE_MIN;
			if (!row) {
				score = (static_cast<double>(index) * SCORE_GAP_LEADING) + matchBonus[index];
			} else if (index) { /* row > 0 && index > 0*/
				score = fmax(
				    prevM + matchBonus[index],

				    /* consecutive match, doesn't stack with match_bonus */
				    prevD + SCORE_MATCH_CONSECUTIVE
				);
			}
			prevD = lastD[index];
			prevM = lastM[index];
			currD[index] = score;
			currM[index] = prevScore = fmax(score, prevScore + gapScore);
		} else {
			prevD = lastD[index];
			prevM = lastM[index];
			currD[index] = SCORE_MIN;
			currM[index] = prevScore = prevScore + gapScore;
		}
	}
}

double match(QStringView needle, QStringView haystack) {
	if (needle.empty()) return SCORE_MIN;

	if (haystack.size() > MATCH_MAX_LEN || needle.size() > haystack.size()) {
		return SCORE_MIN;
	} else if (haystack.size() == needle.size()) {
		return SCORE_MAX;
	}

	const MatchStruct match = setupMatchStruct(needle, haystack);

	/*
	 * D Stores the best score for this position ending with a match.
	 * M Stores the best possible score at this position.
	 */
	std::array<double, MATCH_MAX_LEN> d {};
	std::array<double, MATCH_MAX_LEN> m {};

	for (qsizetype index = 0; index < needle.size(); index++) {
		matchRow(match, index, d, m, d, m);
	}

	return m[haystack.size() - 1];
}

} // namespace

namespace qs {

QList<QObject*>
FzyFinder::filter(const QString& needle, const QList<QObject*>& haystacks, const QString& name) {
	QList<ScoredResult> list;
	for (auto* haystack: haystacks) {
		const auto h = haystack->property(name.toUtf8()).toString();
		if (hasMatch(needle, h)) {
			list.emplace_back(match(needle, h), h, haystack);
		}
	}
	std::ranges::stable_sort(list, std::ranges::greater(), &ScoredResult::score);
	auto out = QList<QObject*>(list.size());
	std::ranges::transform(list, out.begin(), [](const ScoredResult& result) -> QObject* {
		return result.obj;
	});
	return out;
}

} // namespace qs
