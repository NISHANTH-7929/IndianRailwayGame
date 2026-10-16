# Localization and accessibility foundation

Production/localization.json is a UTF-8 catalog with stable keys, en/hi samples and English
fallback intent. Use keys in future widgets and convert to localized FText/String Tables
at integration; do not localize simulation IDs or compare translated text for objective
truth. Only four sample strings exist. Current S7 widgets are intentionally preserved;
this task does not claim they are already fully localized.

Future presentation configuration exposes font scale 1/1.25/1.5, standard/large touch
layout, control labels, pause, result and warning keys. Signal meaning must include text
and symbol alongside color. Validate DPI/safe areas and actual Devanagari shaping/glyph
coverage on Android before accepting any font. These sizes are configurable samples,
not a validated accessibility or human comprehension result. S7 human gates stay pending.

Graphics Low/Medium/High records contain only presentation cost fields. Null means use
the existing measured fixture configuration until a tier override has evidence. Unknown
fields (including physics/AI/scoring settings) fail host validation. No runtime CVar or
logical-state changes are made when these records load.
