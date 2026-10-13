"""CUI website wording; repository provenance and source URLs stay intact."""
import re


def website_text(value):
    # Preserve navigable source URLs and immutable repository filenames.
    parts = re.split(r'(\]\([^\)]+\)|https?://[^\s<>"\)]+)', value)
    for index in range(0, len(parts), 2):
        parts[index] = re.sub(r'(?i)gpui-parity\.json', 'component manifest', parts[index])
        parts[index] = re.sub(r'(?i)\bgpui[ -]kit\b', 'upstream kit', parts[index])
        parts[index] = re.sub(r'(?i)\bgpui\b', 'upstream', parts[index])
    return ''.join(parts)


def website_data(value):
    if isinstance(value, dict):
        return {key: website_data(item) for key, item in value.items()}
    if isinstance(value, list):
        return [website_data(item) for item in value]
    return website_text(value) if isinstance(value, str) else value
