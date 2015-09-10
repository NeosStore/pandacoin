#include "pandastyles.h"
#include <QLocale>



// Font sizes - NB! We specifically use 'px' and not 'pt' for all font sizes, as qt scales 'pt' dynamically in a way that makes our fonts unacceptably small on OSX etc. it doesn't do this with px so we use px instead.
QString CURRENCY_DECIMAL_FONT_SIZE = "11px"; // For .00 in currency and PND text.
QString BODY_FONT_SIZE = "12px"; // Standard body font size used in 'most places'.
QString CURRENCY_FONT_SIZE = "13px"; // For currency
QString TOTAL_FONT_SIZE = "15px"; // For totals and account names
QString HEADER_FONT_SIZE = "16px"; // For large headings



// Fonts

// We 'abuse' the translation system here to allow different 'font stacks' for different languages.
QString MAIN_FONTSTACK = QObject::tr("Arial, 'Helvetica Neue', Helvetica, sans-serif");
