#include "logging.h"

std::ostream& operator<<(std::ostream& stream, const severity_level& level) {
	switch (level) {
		case  trace: return stream << "TRAC";
		case debug: return stream << "DEBG";
		case info: return stream << "INFO";
		case warning: return stream << "WARN";
		case error: return stream << "ERRR";
		case critical: return stream << "CRIT";
		default: return stream << (unsigned int)level;
	}
}
