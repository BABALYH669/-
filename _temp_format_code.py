with open('FreeRTOS/FreeRTOS软件定时器.md', 'r', encoding='utf-8') as f:
    content = f.read()

# Remove all zero-width spaces (U+200B)
content = content.replace('​', '')

def wrap_code_block(text, anchor, end_marker):
    """Find anchor, extract code from its line to end_marker's line, wrap with fences."""
    idx = text.find(anchor)
    if idx < 0:
        return text

    # Find start of the anchor's line
    line_start = text.rfind('\n', 0, idx) + 1

    # Find end_marker after anchor
    end_idx = text.find(end_marker, idx)
    if end_idx < 0:
        return text

    # Find end of the line containing end_marker
    line_end = text.find('\n', end_idx)
    if line_end < 0:
        line_end = len(text)

    # Extract the code text
    code = text[line_start:line_end]

    # Verify this looks like code
    first_line = code.split('\n')[0].strip()
    code_keywords = ['typedef', 'BaseType_t', 'TimerHandle_t', 'void',
                     'uint8_t', 'TickType_t', 'UBaseType_t', 'List']
    is_code = any(first_line.startswith(kw) for kw in code_keywords)

    if not is_code:
        return text

    # Wrap with code fence
    wrapped = '```c\n' + code + '\n```'
    result = text[:line_start] + wrapped + text[line_end:]
    return result

# Process from last in file to first in file
for anchor, end_marker in [
    ('xTimerChangePeriod', ');'),
    ('xTimerReset', ');'),
    ('xTimerStop', ');'),
    ('xTimerStart', ');'),
    ('xTimerCreate', ');'),
    ('typedef', 'xTIMER;'),
]:
    content = wrap_code_block(content, anchor, end_marker)

# Clean up extra blank lines
import re
# Remove `  ` (two spaces) line before code blocks
content = re.sub(r'\n  \n```c', r'\n```c', content)
# Also handle case where there's a blank line then the two-spaces line
content = re.sub(r'\n\n  \n```c', r'\n\n```c', content)

# Write result
with open('FreeRTOS/FreeRTOS软件定时器.md', 'w', encoding='utf-8') as f:
    f.write(content)

print("File written successfully.\n")

# Show all code blocks for verification
idx = content.find('```c')
count = 0
while idx >= 0:
    end_idx = content.find('```', idx + 3)
    if end_idx < 0:
        break
    count += 1
    print(f"--- Code block #{count} ---")
    block = content[idx:end_idx+3]
    print(block)
    print()
    idx = content.find('```c', end_idx + 3)
